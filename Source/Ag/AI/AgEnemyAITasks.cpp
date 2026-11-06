// Copyright Woogle. All Rights Reserved.

#include "AI/AgEnemyAITasks.h"

#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "Ag.h"
#include "AI/AgEnemyAIController.h"
#include "AI/AgPatrolRoute.h"
#include "Algo/SelectRandomWeighted.h"
#include "Character/AgEnemyCharacter.h"
#include "Combat/AgCombatSubsystem.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "StateTreeExecutionContext.h"

namespace
{
	constexpr float RetryInterval = 0.5f;

	constexpr float HomeFacingDistance = 100.f;

	void StopPathMove(AAgEnemyAIController& Controller)
	{
		// 간격 지키기에서는 다가가기와 물러나기·좌우 걷기가 번갈아 바뀌므로, StopMovement처럼 속도를 0으로 만들면 걸음이
		// 그때마다 끊깁니다. 경로만 끊고 남은 속도는 이동 컴포넌트의 감속에 맡깁니다.
		Controller.GetPathFollowingComponent()->AbortMove(Controller, FPathFollowingResultFlags::ForcedScript, FAIRequestID::CurrentRequest,
			EPathFollowingVelocityMode::Keep);
	}

	/** 거리가 들어 있는 범위가 있으면 그 범위, 없으면 가장 조금 움직여 닿는 범위입니다. 차이가 같으면 다가가는 쪽입니다. */
	FFloatInterval PickNearestRange(const TArray<FFloatInterval>& Ranges, float Distance)
	{
		FFloatInterval Picked = Ranges[0];
		float PickedGap = TNumericLimits<float>::Max();
		bool bPickedApproach = false;
		for (const FFloatInterval& Range : Ranges)
		{
			const bool bApproach = Distance > Range.Max;
			const float Gap = bApproach ? Distance - Range.Max : FMath::Max(Range.Min - Distance, 0.f);
			if (Gap < PickedGap || (Gap == PickedGap && bApproach && !bPickedApproach))
			{
				Picked = Range;
				PickedGap = Gap;
				bPickedApproach = bApproach;
			}
		}
		return Picked;
	}

	bool RequestPatrolMove(AAgEnemyAIController& Controller, const FVector& Destination)
	{
		Controller.ClearFocus(EAIFocusPriority::Gameplay);
		// 부분 경로를 허용하면 갈 수 없는 곳으로도 요청이 성공해 가까운 데까지 걸어가므로, 실패로 받아 건너뜁니다.
		return Controller.MoveToLocation(Destination, -1.f, true, true, true, false, {}, false) != EPathFollowingRequestResult::Failed;
	}

	/** 지금 순서부터 걸어서 갈 수 있는 정찰 지점으로 이동을 요청합니다. 연달아 모든 지점에 실패하면 처음 자리로 갑니다. */
	void RequestReachablePointMove(AAgEnemyAIController& Controller, const AAgEnemyCharacter& Enemy, FAgEnemyAITask_PatrolInstanceData& InstanceData)
	{
		const AAgPatrolRoute& Route = *Enemy.GetPatrolRoute();
		const int32 PointCount = Route.GetPointCount();
		while (InstanceData.FailedCount < PointCount)
		{
			if (RequestPatrolMove(Controller, Route.GetPointLocation(InstanceData.PointIndex)))
			{
				return;
			}
			UE_LOG(LogAg, Warning, TEXT("%s cannot walk to patrol point %d of %s."), *Enemy.GetName(), InstanceData.PointIndex + 1, *Route.GetName());
			++InstanceData.FailedCount;
			InstanceData.PointIndex = (InstanceData.PointIndex + 1) % PointCount;
		}
		InstanceData.PointIndex = INDEX_NONE;
		RequestPatrolMove(Controller, Enemy.GetHomeTransform().GetLocation());
	}

} // namespace

EStateTreeRunStatus FAgEnemyAITask_KeepDistance::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.StrafeDirection = FMath::RandBool() ? 1.f : -1.f;
	InstanceData.bApproaching = false;
	InstanceData.bWasDelaying = InstanceData.Controller->IsInPostPatternDelay();
	// 움찔이 끝나고 들어왔을 때처럼 쓸 수 있는 패턴이 이미 있으면 첫 틱에 바로 끝나도록 합니다.
	InstanceData.CheckElapsed = RetryInterval;
	InstanceData.TargetRange = InstanceData.Controller->GetEnemy()->GetCharacterData()->AI.PreferredDistance;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FAgEnemyAITask_KeepDistance::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AAgEnemyAIController& Controller = *InstanceData.Controller;
	AAgEnemyCharacter* Enemy = Controller.GetEnemy();
	AAgCharacterBase* Target = Controller.GetTarget();
	if (!Target)
	{
		return EStateTreeRunStatus::Running;
	}
	const float Distance = Enemy->GetHorizontalDistanceTo(Target);
	const bool bDelaying = Controller.IsInPostPatternDelay();
	const bool bDelayEnded = InstanceData.bWasDelaying && !bDelaying;
	InstanceData.bWasDelaying = bDelaying;
	InstanceData.CheckElapsed += DeltaTime;

	if (InstanceData.CheckElapsed >= RetryInterval || bDelayEnded)
	{
		InstanceData.CheckElapsed = 0.f;
		bool bAttackTurn = false;
		if (!bDelaying && Enemy->GetWorld()->GetSubsystem<UAgCombatSubsystem>()->HasAttackTurn(*Enemy))
		{
			TArray<TPair<FName, float>> Usable;
			Enemy->GatherPatterns(Distance, false, Usable);
			if (!Usable.IsEmpty())
			{
				return EStateTreeRunStatus::Succeeded;
			}
			TArray<FFloatInterval> ReadyRanges;
			Enemy->GatherReadyPatternRanges(ReadyRanges);
			bAttackTurn = !ReadyRanges.IsEmpty();
			InstanceData.TargetRange = bAttackTurn ? PickNearestRange(ReadyRanges, Distance) : Enemy->GetCharacterData()->AI.PreferredDistance;
		}
		else
		{
			InstanceData.TargetRange = Enemy->GetCharacterData()->AI.PreferredDistance;
		}

		// 공격할 차례인데 목표 거리 안이면 거리는 닿지만 시야 같은 조건이 맞지 않은 것이라, 보이는 곳을 찾아 다가갑니다.
		const bool bApproach = Distance > InstanceData.TargetRange.Max || (bAttackTurn && Distance >= InstanceData.TargetRange.Min);
		// 엔진의 막힘 감지나 부분 경로의 끝에 닿아 경로 이동이 도착 전에 끝날 수 있으므로, 멈춰 있으면 다시 요청합니다.
		if (bApproach && Controller.GetMoveStatus() == EPathFollowingStatus::Idle)
		{
			Controller.MoveToActor(Target);
		}
		else if (!bApproach && InstanceData.bApproaching)
		{
			StopPathMove(Controller);
		}
		InstanceData.bApproaching = bApproach;
	}

	if (InstanceData.bApproaching)
	{
		return EStateTreeRunStatus::Running;
	}
	// 적은 스스로 대상을 향해 돌므로, 반대쪽으로 걸으면 바라본 채 물러나고 옆으로 걸으면 대상 주위를 돕니다.
	if (Distance < InstanceData.TargetRange.Min)
	{
		Enemy->AddMovementInput((Enemy->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D());
	}
	else if (Enemy->GetCharacterData()->AI.WaitMovement == EAgWaitMovement::Strafe)
	{
		Enemy->AddMovementInput(Enemy->GetActorRightVector(), InstanceData.StrafeDirection);
	}
	return EStateTreeRunStatus::Running;
}

void FAgEnemyAITask_KeepDistance::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (InstanceData.bApproaching)
	{
		StopPathMove(*InstanceData.Controller);
	}
}

EStateTreeRunStatus FAgEnemyAITask_RunPattern::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.RunningPattern = NAME_None;
	InstanceData.Handle = FGameplayAbilitySpecHandle();

	AAgEnemyCharacter* Enemy = InstanceData.Controller->GetEnemy();
	UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent();
	const AAgCharacterBase* Target = InstanceData.Controller->GetTarget();
	UAgCombatSubsystem* CombatSubsystem = Enemy->GetWorld()->GetSubsystem<UAgCombatSubsystem>();
	// 반응형 패턴만 패턴 후딜레이 중에 나오고 차례와 관계없습니다. 반응형 패턴이 끝나면 간격 지키기를 거치지 않고 이 상태로
	// 오므로, 차례도 여기서 확인합니다.
	if (!Target || (!InstanceData.bResponse && (InstanceData.Controller->IsInPostPatternDelay() || !CombatSubsystem->HasAttackTurn(*Enemy))))
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
	if (!InstanceData.bResponse)
	{
		CombatSubsystem->StartAttack(*Enemy);
	}
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FAgEnemyAITask_RunPattern::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	// 패턴이 피격 반응으로 끊겨도 여기서는 끝난 것으로 봅니다. 그때는 우선순위가 높은 피격 반응 전환이 먼저 적용되고,
	// 후딜레이는 그 반응이 끝날 때 정합니다.
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AAgEnemyAIController& Controller = *InstanceData.Controller;
	const UAbilitySystemComponent* ASC = Controller.GetEnemy()->GetAbilitySystemComponent();
	const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(InstanceData.Handle);
	if (Spec && Spec->IsActive())
	{
		return EStateTreeRunStatus::Running;
	}
	if (Controller.IsEnemyReacting())
	{
		return EStateTreeRunStatus::Succeeded;
	}
	// 반응형 패턴(Backstep)은 끝까지 마치면 남은 후딜레이 없이 바로 다음 패턴을 고릅니다.
	if (InstanceData.bResponse)
	{
		Controller.ClearPostPatternDelay();
	}
	else
	{
		Controller.StartPostPatternDelay();
	}
	return EStateTreeRunStatus::Succeeded;
}

void FAgEnemyAITask_RunPattern::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AAgEnemyAIController& Controller = *InstanceData.Controller;
	AAgEnemyCharacter* Enemy = Controller.GetEnemy();
	UAbilitySystemComponent* ASC = Enemy ? Enemy->GetAbilitySystemComponent() : nullptr;
	const FGameplayAbilitySpec* Spec = ASC ? ASC->FindAbilitySpecFromHandle(InstanceData.Handle) : nullptr;
	// 조준 대상 놓침처럼 패턴이 끝나기 전에 다른 상태로 가면 패턴을 멈추고 후딜레이를 겁니다.
	if (Spec && Spec->IsActive())
	{
		ASC->CancelAbilityHandle(InstanceData.Handle);
		if (!InstanceData.bResponse)
		{
			Controller.StartPostPatternDelay();
		}
	}
	if (Enemy && !InstanceData.bResponse && InstanceData.Handle.IsValid())
	{
		Enemy->GetWorld()->GetSubsystem<UAgCombatSubsystem>()->EndAttack(*Enemy);
	}
}

EStateTreeRunStatus FAgEnemyAITask_PhaseTransition::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	return InstanceData.Controller->GetEnemy()->StartPhaseTransition() ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FAgEnemyAITask_PhaseTransition::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (InstanceData.Controller->GetEnemy()->IsPhaseTransitionActive())
	{
		return EStateTreeRunStatus::Running;
	}
	InstanceData.Controller->StartPostPatternDelay();
	return EStateTreeRunStatus::Succeeded;
}

EStateTreeRunStatus FAgEnemyAITask_WaitReaction::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.bHeavyReaction = false;
	return Tick(Context, 0.f);
}

EStateTreeRunStatus FAgEnemyAITask_WaitReaction::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const AAgEnemyAIController& Controller = *InstanceData.Controller;
	if (!Controller.IsEnemyReacting())
	{
		return EStateTreeRunStatus::Succeeded;
	}
	// 움찔 도중 넉다운으로 바뀌는 것처럼 반응이 이어질 수 있어, 머무는 동안 한 번이라도 있었는지 기록합니다.
	InstanceData.bHeavyReaction |= !Controller.IsEnemyFlinching();
	return EStateTreeRunStatus::Running;
}

void FAgEnemyAITask_WaitReaction::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (InstanceData.bHeavyReaction)
	{
		InstanceData.Controller->StartPostPatternDelay();
	}
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
	AAgEnemyAIController& Controller = *Context.GetInstanceData(*this).Controller;
	UpdateTarget(Controller);
	Controller.StartPostPatternDelay();
	Controller.GetWorld()->GetSubsystem<UAgCombatSubsystem>()->AddCombatant(*Controller.GetEnemy());
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
	Controller.GetWorld()->GetSubsystem<UAgCombatSubsystem>()->RemoveCombatant(*Controller.GetEnemy());
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
	const AAgPatrolRoute* Route = Enemy->GetPatrolRoute();

	InstanceData.PointIndex = Route ? Route->FindNearestPoint(Enemy->GetActorLocation()) : INDEX_NONE;
	InstanceData.FailedCount = 0;
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
		// MOV가 0이면 이동은 진행되지 않고 목적지 쪽으로 몸만 돌리므로, 요청하지 않고 처음 방향만 봅니다.
		if (Enemy->GetCharacterMovement()->MaxWalkSpeed > 0.f)
		{
			if (InstanceData.PointIndex != INDEX_NONE)
			{
				RequestReachablePointMove(Controller, *Enemy, InstanceData);
			}
			else
			{
				RequestPatrolMove(Controller, Enemy->GetHomeTransform().GetLocation());
			}
			return EStateTreeRunStatus::Running;
		}
		InstanceData.PointIndex = INDEX_NONE;
	}

	if (InstanceData.PointIndex == INDEX_NONE)
	{
		const FTransform& Home = Enemy->GetHomeTransform();
		Controller.SetFocalPoint(Enemy->GetActorLocation() + Home.GetRotation().GetForwardVector() * HomeFacingDistance);
		return EStateTreeRunStatus::Running;
	}
	InstanceData.FailedCount = 0;
	InstanceData.PointIndex = (InstanceData.PointIndex + 1) % Enemy->GetPatrolRoute()->GetPointCount();
	RequestReachablePointMove(Controller, *Enemy, InstanceData);
	return EStateTreeRunStatus::Running;
}

void FAgEnemyAITask_Patrol::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	AAgEnemyAIController& Controller = *Context.GetInstanceData(*this).Controller;
	Controller.StopMovement();
	Controller.ClearFocus(EAIFocusPriority::Gameplay);
}
