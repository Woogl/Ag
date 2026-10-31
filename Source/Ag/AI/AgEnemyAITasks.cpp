// Copyright Woogle. All Rights Reserved.

#include "AI/AgEnemyAITasks.h"

#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "AI/AgEnemyAIController.h"
#include "Algo/SelectRandomWeighted.h"
#include "Character/AgEnemyCharacter.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "Engine/TargetPoint.h"
#include "Navigation/PathFollowingComponent.h"
#include "StateTreeExecutionContext.h"

namespace
{
	constexpr float RetryInterval = 0.5f;

	constexpr float HomeFacingDistance = 100.f;

	FVector GetPatrolDestination(const AAgEnemyCharacter& Enemy, int32 PointIndex)
	{
		const TArray<TObjectPtr<ATargetPoint>>& Points = Enemy.GetPatrolPoints();
		const ATargetPoint* Point = Points.IsValidIndex(PointIndex) ? Points[PointIndex].Get() : nullptr;
		return Point ? Point->GetActorLocation() : Enemy.GetHomeTransform().GetLocation();
	}

	void RequestPatrolMove(AAgEnemyAIController& Controller, const FVector& Destination)
	{
		Controller.ClearFocus(EAIFocusPriority::Gameplay);
		Controller.MoveToLocation(Destination, -1.f, true, true, true, false);
	}

} // namespace

EStateTreeRunStatus FAgEnemyAITask_PostPatternDelay::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const FFloatInterval& RecoveryTime = InstanceData.Controller->GetEnemy()->GetCharacterData()->PostPatternDelayTime;
	InstanceData.Duration = FMath::FRandRange(RecoveryTime.Min, RecoveryTime.Max);
	InstanceData.Elapsed = 0.f;
	InstanceData.StrafeDirection = FMath::RandBool() ? 1.f : -1.f;
	return InstanceData.Duration > 0.f ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

EStateTreeRunStatus FAgEnemyAITask_PostPatternDelay::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AAgEnemyCharacter* Enemy = InstanceData.Controller->GetEnemy();

	// 적은 스스로 플레이어를 향해 돌므로, 옆으로 걸으면 플레이어 주위를 돕니다.
	Enemy->AddMovementInput(Enemy->GetActorRightVector(), InstanceData.StrafeDirection);
	InstanceData.Elapsed += DeltaTime;
	return InstanceData.Elapsed >= InstanceData.Duration ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FAgEnemyAITask_Approach::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.Elapsed = 0.f;
	if (AAgCharacterBase* Target = InstanceData.Controller->GetTarget())
	{
		InstanceData.Controller->MoveToActor(Target);
	}
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FAgEnemyAITask_Approach::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.Elapsed += DeltaTime;
	return InstanceData.Elapsed >= RetryInterval ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

void FAgEnemyAITask_Approach::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	Context.GetInstanceData(*this).Controller->StopMovement();
}

EStateTreeRunStatus FAgEnemyAITask_RunPattern::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.RunningPattern = NAME_None;
	InstanceData.Handle = FGameplayAbilitySpecHandle();

	AAgEnemyCharacter* Enemy = InstanceData.Controller->GetEnemy();
	UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent();
	const AAgCharacterBase* Target = InstanceData.Controller->GetTarget();
	if (!Target)
	{
		return EStateTreeRunStatus::Failed;
	}
	TArray<TPair<FName, float>> Candidates;
	Enemy->GatherPatterns(Enemy->GetHorizontalDistanceTo(Target), InstanceData.bResponse, Candidates);

	// StateTree의 가중치 선택은 패턴마다 상태가 필요해, 패턴 목록을 적 데이터에만 두려고 여기서 고릅니다.
	const TPair<FName, float>* Picked =
		Algo::SelectRandomWeightedBy(Candidates, [](const TPair<FName, float>& Candidate)
	{ return Candidate.Value; });
	if (!Picked)
	{
		return EStateTreeRunStatus::Failed;
	}
	const FName Pattern = Picked->Key;

	const FGameplayAbilitySpec* Spec = Enemy->FindAbilitySpec(Pattern);
	const FGameplayAbilitySpecHandle Handle = Spec ? Spec->Handle : FGameplayAbilitySpecHandle();
	if (!Handle.IsValid() || !ASC->TryActivateAbility(Handle))
	{
		return EStateTreeRunStatus::Failed;
	}
	InstanceData.Handle = Handle;
	InstanceData.RunningPattern = Pattern;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FAgEnemyAITask_RunPattern::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	// 패턴이 넉다운이나 그로기로 끊겨도 여기서는 끝난 것으로 봅니다. 그때는 우선순위가 높은 피격 반응 전환이
	// 먼저 적용됩니다.
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const UAbilitySystemComponent* ASC = InstanceData.Controller->GetEnemy()->GetAbilitySystemComponent();
	const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(InstanceData.Handle);
	return Spec && Spec->IsActive() ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

EStateTreeRunStatus FAgEnemyAITask_PhaseTransition::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	return InstanceData.Controller->GetEnemy()->StartPhaseTransition() ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FAgEnemyAITask_PhaseTransition::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	return InstanceData.Controller->GetEnemy()->IsPhaseTransitionActive() ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

EStateTreeRunStatus FAgEnemyAITask_WaitReaction::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	return InstanceData.Controller->IsEnemyReacting() ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

EStateTreeRunStatus FAgEnemyAITask_WaitReaction::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	return InstanceData.Controller->IsEnemyReacting() ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

FAgEnemyAITask_NonCombat::FAgEnemyAITask_NonCombat()
{
	bShouldCallTick = false;
	bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FAgEnemyAITask_NonCombat::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	UAgAbilitySystemComponent* ASC = InstanceData.Controller->GetEnemy()->GetAbilitySystemComponent();
	ASC->AddLooseGameplayTag(AgGameplayTags::State_NonCombat);
	ASC->CancelAbilitiesWithSpecTags(FGameplayTagContainer(AgGameplayTags::Ability_Enemy_Pattern));
	return EStateTreeRunStatus::Running;
}

void FAgEnemyAITask_NonCombat::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.Controller->GetEnemy()->GetAbilitySystemComponent()->RemoveLooseGameplayTag(AgGameplayTags::State_NonCombat);
}

FAgEnemyAITask_Combat::FAgEnemyAITask_Combat()
{
	// 전투 아래의 상태끼리 옮겨 다닐 때 락온을 풀었다 다시 걸지 않습니다.
	bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FAgEnemyAITask_Combat::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	UpdateTarget(*Context.GetInstanceData(*this).Controller);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FAgEnemyAITask_Combat::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	UpdateTarget(*Context.GetInstanceData(*this).Controller);
	return EStateTreeRunStatus::Running;
}

void FAgEnemyAITask_Combat::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	AAgEnemyAIController& Controller = *Context.GetInstanceData(*this).Controller;
	Controller.ClearFocus(EAIFocusPriority::Gameplay);
	UAgAbilitySystemComponent* ASC = Controller.GetEnemy()->GetAbilitySystemComponent();
	if (const FGameplayAbilitySpec* LockOn = ASC->FindSpecByTag(AgGameplayTags::Ability_LockOn))
	{
		ASC->CancelAbilityHandle(LockOn->Handle);
	}
}

void FAgEnemyAITask_Combat::UpdateTarget(AAgEnemyAIController& Controller)
{
	UAgAbilitySystemComponent* ASC = Controller.GetEnemy()->GetAbilitySystemComponent();
	const FGameplayAbilitySpec* LockOn = ASC->FindSpecByTag(AgGameplayTags::Ability_LockOn);
	if (LockOn && !LockOn->IsActive())
	{
		ASC->TryActivateAbility(LockOn->Handle);
	}
	if (AAgCharacterBase* Target = Controller.GetTarget())
	{
		Controller.SetFocus(Target);
	}
	else
	{
		Controller.ClearFocus(EAIFocusPriority::Gameplay);
	}
}

EStateTreeRunStatus FAgEnemyAITask_Patrol::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const AAgEnemyCharacter* Enemy = InstanceData.Controller->GetEnemy();
	const TArray<TObjectPtr<ATargetPoint>>& Points = Enemy->GetPatrolPoints();

	InstanceData.PointIndex = INDEX_NONE;
	double NearestDistanceSq = TNumericLimits<double>::Max();
	for (int32 Index = 0; Index < Points.Num(); ++Index)
	{
		const double DistanceSq = Points[Index] ? FVector::DistSquared2D(Points[Index]->GetActorLocation(), Enemy->GetActorLocation()) : NearestDistanceSq;
		if (DistanceSq < NearestDistanceSq)
		{
			NearestDistanceSq = DistanceSq;
			InstanceData.PointIndex = Index;
		}
	}
	// StateTree는 빙의할 때(BeginPlay 전) 시작하므로, 이동 요청은 첫 틱에서 보냅니다.
	InstanceData.bMoveRequested = false;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FAgEnemyAITask_Patrol::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AAgEnemyAIController& Controller = *InstanceData.Controller;
	if (Controller.GetMoveStatus() != EPathFollowingStatus::Idle)
	{
		return EStateTreeRunStatus::Running;
	}

	const AAgEnemyCharacter* Enemy = Controller.GetEnemy();
	if (!InstanceData.bMoveRequested)
	{
		InstanceData.bMoveRequested = true;
		RequestPatrolMove(Controller, GetPatrolDestination(*Enemy, InstanceData.PointIndex));
		return EStateTreeRunStatus::Running;
	}

	const int32 PointCount = Enemy->GetPatrolPoints().Num();
	if (InstanceData.PointIndex == INDEX_NONE)
	{
		const FTransform& Home = Enemy->GetHomeTransform();
		Controller.SetFocalPoint(Enemy->GetActorLocation() + Home.GetRotation().GetForwardVector() * HomeFacingDistance);
		return EStateTreeRunStatus::Running;
	}
	InstanceData.PointIndex = (InstanceData.PointIndex + 1) % PointCount;
	RequestPatrolMove(Controller, GetPatrolDestination(*Enemy, InstanceData.PointIndex));
	return EStateTreeRunStatus::Running;
}

void FAgEnemyAITask_Patrol::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	AAgEnemyAIController& Controller = *Context.GetInstanceData(*this).Controller;
	Controller.StopMovement();
	Controller.ClearFocus(EAIFocusPriority::Gameplay);
}
