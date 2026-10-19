// Copyright Woogle. All Rights Reserved.

#include "AI/AgBossAIController.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Character/AgBossCharacter.h"
#include "Components/StateTreeAIComponent.h"
#include "Core/AgAssetManager.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "Data/AgEffectData.h"
#include "Kismet/GameplayStatics.h"

AAgBossAIController::AAgBossAIController()
{
	PrimaryActorTick.bCanEverTick = true;

	StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTree"));
	// 실행할 StateTree는 빙의할 때 보스 데이터에서 정하므로 BeginPlay에 자동으로 시작하지 않습니다.
	StateTreeComponent->SetStartLogicAutomatically(false);
}

AAgBossCharacter* AAgBossAIController::GetBoss() const
{
	return GetPawn<AAgBossCharacter>();
}

const UAgBossData* AAgBossAIController::GetBossData() const
{
	const AAgBossCharacter* Boss = GetBoss();
	return Boss ? Cast<UAgBossData>(Boss->GetCharacterData()) : nullptr;
}

AAgCharacterBase* AAgBossAIController::GetTarget() const
{
	// 지금은 플레이어 한 명과만 싸우므로 대상은 플레이어입니다.
	return Cast<AAgCharacterBase>(UGameplayStatics::GetPlayerPawn(this, 0));
}

bool AAgBossAIController::IsBossReacting() const
{
	const AAgBossCharacter* Boss = GetBoss();
	const UAbilitySystemComponent* ASC = Boss ? Boss->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return false;
	}

	FGameplayTagContainer ReactionTags;
	ReactionTags.AddTag(AgGameplayTags::State_HitReaction);
	ReactionTags.AddTag(AgGameplayTags::State_Groggy);
	return ASC->HasAnyMatchingGameplayTags(ReactionTags);
}

void AAgBossAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (const AAgBossCharacter* Boss = GetBoss())
	{
		BossASC = Boss->GetAbilitySystemComponent();
		AbilityEndedHandle = BossASC->OnAbilityEnded.AddUObject(this, &ThisClass::HandleBossAbilityEnded);
	}
	const UAgBossData* Data = GetBossData();
	if (!Data || !Data->AIStateTree)
	{
		return;
	}

	StateTreeComponent->SetStateTree(Data->AIStateTree);
	StateTreeComponent->StartLogic();
}

void AAgBossAIController::OnUnPossess()
{
	StateTreeComponent->StopLogic(TEXT("UnPossess"));
	if (TargetASC.IsValid())
	{
		TargetASC->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_BasicAttackStarted).Remove(BasicAttackHandle);
	}
	TargetASC.Reset();
	BasicAttackHandle.Reset();
	if (BossASC.IsValid())
	{
		BossASC->OnAbilityEnded.Remove(AbilityEndedHandle);
	}
	BossASC.Reset();
	AbilityEndedHandle.Reset();

	Super::OnUnPossess();
}

void AAgBossAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	BindTargetEvents();
}

void AAgBossAIController::BindTargetEvents()
{
	// 보스는 플레이어가 스폰되기 전에 빙의하므로, 빙의할 때가 아니라 대상이 생긴 뒤의 틱에서 한 번 구독합니다.
	if (TargetASC.IsValid())
	{
		return;
	}
	if (const AAgCharacterBase* Target = GetTarget())
	{
		TargetASC = Target->GetAbilitySystemComponent();
		BasicAttackHandle = TargetASC->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_BasicAttackStarted)
		                        .AddUObject(this, &ThisClass::HandleTargetBasicAttack);
	}
}

void AAgBossAIController::HandleTargetBasicAttack(const FGameplayEventData* Payload)
{
	const AAgBossCharacter* Boss = GetBoss();
	if (Payload && Boss && Payload->Target.Get() == Boss)
	{
		StateTreeComponent->SendStateTreeEvent(AgGameplayTags::Event_BasicAttackStarted);
	}
}

void AAgBossAIController::HandleBossAbilityEnded(const FAbilityEndedData& Data)
{
	const UAgGameplayAbility* Ability = Cast<UAgGameplayAbility>(Data.AbilityThatEnded);
	AAgBossCharacter* Boss = GetBoss();
	if (Data.bWasCancelled || !Ability || !Boss || Boss->IsDead() ||
		!Ability->GetDefinitionTags().HasTagExact(AgGameplayTags::Ability_Reaction_PhaseTransition))
	{
		return;
	}
	UAbilitySystemComponent* ASC = Boss->GetAbilitySystemComponent();
	ASC->ApplyGameplayEffectToSelf(UAgAssetManager::Get().GetEffect(EAgEffect::PPRefill), 1.f, ASC->MakeEffectContext());
	Boss->EnterPhase2();
}
