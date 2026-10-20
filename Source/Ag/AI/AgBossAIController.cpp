// Copyright Woogle. All Rights Reserved.

#include "AI/AgBossAIController.h"

#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "Character/AgBossCharacter.h"
#include "Components/StateTreeAIComponent.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"

AAgBossAIController::AAgBossAIController()
{
	PrimaryActorTick.bCanEverTick = true;

	StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTree"));
	// 실행할 StateTree는 빙의할 때 보스 캐릭터에서 가져오므로 BeginPlay에 자동으로 시작하지 않습니다.
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
	const AAgBossCharacter* Boss = GetBoss();
	return Boss ? Boss->GetLockOnTarget() : nullptr;
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
	ReactionTags.AddTag(AgGameplayTags::State_Execution);
	return ASC->HasAnyMatchingGameplayTags(ReactionTags);
}

void AAgBossAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	const AAgBossCharacter* Boss = GetBoss();
	if (!Boss || !Boss->AIStateTree)
	{
		return;
	}

	StateTreeComponent->SetStateTree(Boss->AIStateTree);
	StateTreeComponent->StartLogic();
}

void AAgBossAIController::OnUnPossess()
{
	ClearFocus(EAIFocusPriority::Gameplay);
	StateTreeComponent->StopLogic(TEXT("UnPossess"));
	if (TargetASC.IsValid())
	{
		TargetASC->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_BasicAttackStarted).Remove(BasicAttackHandle);
	}
	TargetASC.Reset();
	BasicAttackHandle.Reset();

	Super::OnUnPossess();
}

void AAgBossAIController::Tick(float DeltaSeconds)
{
	UpdateTarget();

	Super::Tick(DeltaSeconds);

	BindTargetEvents();
}

void AAgBossAIController::UpdateTarget()
{
	const AAgBossCharacter* Boss = GetBoss();
	UAgAbilitySystemComponent* ASC = Boss ? Cast<UAgAbilitySystemComponent>(Boss->GetAbilitySystemComponent()) : nullptr;
	const FGameplayAbilitySpec* LockOn = ASC ? ASC->FindSpecByTag(AgGameplayTags::Ability_LockOn) : nullptr;
	if (LockOn && !LockOn->IsActive())
	{
		ASC->TryActivateAbility(LockOn->Handle);
	}
	if (AAgCharacterBase* Target = GetTarget())
	{
		SetFocus(Target);
	}
	else
	{
		ClearFocus(EAIFocusPriority::Gameplay);
	}
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
