// Copyright Woogle. All Rights Reserved.

#include "AI/AgBossAIController.h"

#include "AbilitySystemComponent.h"
#include "Character/AgBossCharacter.h"
#include "Components/StateTreeAIComponent.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "Kismet/GameplayStatics.h"

AAgBossAIController::AAgBossAIController()
{
	PrimaryActorTick.bCanEverTick = true;

	StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTree"));
	// 실행할 StateTree는 빙의할 때 보스 데이터에서 정하므로 BeginPlay에 자동으로 시작하지 않습니다.
	StateTreeComponent->SetStartLogicAutomatically(false);
}

const UAgBossData* AAgBossAIController::GetBossData() const
{
	return Boss.IsValid() ? Cast<UAgBossData>(Boss->GetCharacterData()) : nullptr;
}

AAgCharacterBase* AAgBossAIController::GetTarget() const
{
	// 지금은 플레이어 한 명과만 싸우므로 대상은 플레이어입니다.
	return Cast<AAgCharacterBase>(UGameplayStatics::GetPlayerPawn(this, 0));
}

bool AAgBossAIController::IsBossReacting() const
{
	const UAbilitySystemComponent* ASC = Boss.IsValid() ? Boss->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return false;
	}

	// 넉백(상태 우선순위 4)과 그로기(3)
	FGameplayTagContainer ReactionTags;
	ReactionTags.AddTag(AgGameplayTags::State_HitReaction);
	ReactionTags.AddTag(AgGameplayTags::State_Groggy);
	return ASC->HasAnyMatchingGameplayTags(ReactionTags);
}

void AAgBossAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	Boss = Cast<AAgBossCharacter>(InPawn);
	const UAgBossData* Data = GetBossData();
	if (!Boss.IsValid() || !Data || !Data->AIStateTree)
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

	Super::OnUnPossess();
}

void AAgBossAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	BindTargetEvents();
}

void AAgBossAIController::BindTargetEvents()
{
	if (TargetASC.IsValid())
	{
		return;
	}
	if (const AAgCharacterBase* Target = GetTarget())
	{
		TargetASC = Target->GetAbilitySystemComponent();
		BasicAttackHandle = TargetASC->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_BasicAttackStarted).AddUObject(this, &ThisClass::HandleTargetBasicAttack);
	}
}

void AAgBossAIController::HandleTargetBasicAttack(const FGameplayEventData* Payload)
{
	// 다른 적을 친 평타는 넘기지 않습니다. B1 백스텝의 발동 조건과 확률은 StateTree의 패턴 후딜레이 전환이 검사합니다.
	if (Payload && Boss.IsValid() && Payload->Target.Get() == Boss.Get())
	{
		StateTreeComponent->SendStateTreeEvent(AgGameplayTags::Event_BasicAttackStarted);
	}
}
