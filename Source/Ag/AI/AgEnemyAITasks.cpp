// Copyright Woogle. All Rights Reserved.

#include "AI/AgEnemyAITasks.h"

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
#include "NavigationSystem.h"
#include "StateTreeExecutionContext.h"

namespace AgEnemyAITasksPrivate
{
	constexpr float RetryInterval = 0.5f;

	constexpr float HomeFacingDistance = 100.f;

	constexpr float WalkLookAhead = 100.f;

	constexpr float WalkAcceptanceRadius = 5.f;

	// 벽이나 내비메시 끝에 비스듬히 닿으면 레이캐스트가 닿는 점이 걸을 때마다 옆으로 조금씩 밀려나 끝을 따라 미끄러지므로,
	// 닿는 점이 이만큼 가까우면 걷지 않습니다.
	constexpr float WalkStopDistance = 10.f;

	void StopPathMove(AAgEnemyAIController& Controller)
	{
		// 간격 지키기에서는 걷기와 제자리가 번갈아 바뀌므로, StopMovement처럼 속도를 0으로 만들면 걸음이 그때마다 끊깁니다.
		// 경로만 끊고 남은 속도는 이동 컴포넌트의 감속에 맡깁니다.
		Controller.GetPathFollowingComponent()->AbortMove(Controller, FPathFollowingResultFlags::ForcedScript, FAIRequestID::CurrentRequest,
			EPathFollowingVelocityMode::Keep);
	}

	/**
	 * 물러나기·좌우 걷기의 한 걸음입니다. 군중 회피는 경로 이동에만 적용되므로 경로 이동으로 걷되, 목적지를 직선으로 열린 곳까지로
	 * 잘라 벽이나 내비메시 끝 앞에서 멈추고 돌아가지 않습니다.
	 */
	void WalkInsideNavMesh(AAgEnemyAIController& Controller, AAgEnemyCharacter& Enemy, const FVector& Direction)
	{
		// 엔진 레이캐스트는 시작점이 내비메시 밖이면 모든 방향을 막힘으로 답합니다. 멈추며 끝을 조금 넘었거나 넉다운 등으로
		// 벽에 붙거나 가장자리에 선 적도 안쪽이나 옆으로는 걷도록, 가장 가까운 내비메시 점에서 봅니다.
		UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Enemy.GetWorld());
		FNavLocation Start;
		if (!NavSys || !NavSys->ProjectPointToNavigation(Enemy.GetActorLocation(), Start, INVALID_NAVEXTENT, &Enemy.GetNavAgentPropertiesRef()))
		{
			StopPathMove(Controller);
			return;
		}
		FVector Destination = Start.Location + Direction * WalkLookAhead;
		FVector HitLocation;
		if (UNavigationSystemV1::NavigationRaycast(&Enemy, Start.Location, Destination, HitLocation, nullptr, &Controller))
		{
			if (FVector::Dist2D(Start.Location, HitLocation) <= WalkStopDistance)
			{
				StopPathMove(Controller);
				return;
			}
			Destination = HitLocation;
		}
		Controller.MoveToLocation(Destination, WalkAcceptanceRadius, false, true, false, true, {}, false);
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

	/**
	 * 지금 쓸 수 있는 패턴이 있으면 true입니다. 없으면 지킬 거리를 고르고, 그보다 멀면 다가가기를 요청합니다.
	 */
	bool ChooseTargetRange(AAgEnemyAIController& Controller, const AAgEnemyCharacter& Enemy, AAgCharacterBase& Target, float Distance, FAgEnemyAITask_KeepDistanceInstanceData& InstanceData)
	{
		InstanceData.TargetRange = Enemy.GetCharacterData()->AI.PreferredDistance;
		bool bAttackTurn = false;
		if (Enemy.GetWorld()->GetSubsystem<UAgCombatSubsystem>()->CanStartPattern(Enemy, false))
		{
			TArray<TPair<FName, float>> Usable;
			Enemy.GatherPatterns(Distance, false, Usable);
			if (!Usable.IsEmpty())
			{
				return true;
			}
			TArray<FFloatInterval> ReadyRanges;
			Enemy.GatherReadyPatternRanges(ReadyRanges);
			bAttackTurn = !ReadyRanges.IsEmpty();
			if (bAttackTurn)
			{
				InstanceData.TargetRange = PickNearestRange(ReadyRanges, Distance);
			}
		}

		// 공격할 차례인데 목표 거리 안이면 거리는 닿지만 시야 같은 조건이 맞지 않은 것이라, 보이는 곳을 찾아 다가갑니다.
		const bool bApproach = Distance > InstanceData.TargetRange.Max || (bAttackTurn && Distance >= InstanceData.TargetRange.Min);
		// 엔진의 막힘 감지나 부분 경로의 끝에 닿아 경로 이동이 도착 전에 끝날 수 있으므로, 다가가는 중에 멈춰 있으면 다시 요청합니다.
		if (bApproach && (!InstanceData.bApproaching || Controller.GetMoveStatus() == EPathFollowingStatus::Idle))
		{
			Controller.MoveToActor(&Target);
		}
		InstanceData.bApproaching = bApproach;
		return false;
	}

	/** 지킬 거리보다 가까우면 물러나고, 그 안이면 캐릭터 데이터에 따라 좌우로 걷거나 제자리에 섭니다. */
	void WalkAtTargetRange(AAgEnemyAIController& Controller, AAgEnemyCharacter& Enemy, const AAgCharacterBase& Target, float Distance, const FAgEnemyAITask_KeepDistanceInstanceData& InstanceData)
	{
		// 적은 스스로 대상을 향해 돌므로, 반대쪽으로 걸으면 바라본 채 물러나고 옆으로 걸으면 대상 주위를 돕니다.
		if (Distance < InstanceData.TargetRange.Min)
		{
			WalkInsideNavMesh(Controller, Enemy, (Enemy.GetActorLocation() - Target.GetActorLocation()).GetSafeNormal2D());
		}
		else if (Enemy.GetCharacterData()->AI.WaitMovement == EAgWaitMovement::Strafe)
		{
			WalkInsideNavMesh(Controller, Enemy, Enemy.GetActorRightVector() * InstanceData.StrafeDirection);
		}
		else
		{
			StopPathMove(Controller);
		}
	}

	bool RequestPatrolMove(AAgEnemyAIController& Controller, const FVector& Destination)
	{
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

	void UpdateCombatTarget(AAgEnemyAIController& Controller)
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
} // namespace AgEnemyAITasksPrivate

EStateTreeRunStatus FAgEnemyAITask_KeepDistance::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.StrafeDirection = FMath::RandBool() ? 1.f : -1.f;
	InstanceData.bApproaching = false;
	InstanceData.bWasDelaying = InstanceData.Controller->IsInPostPatternDelay();
	// 움찔이 끝나고 들어왔을 때처럼 쓸 수 있는 패턴이 이미 있으면 첫 틱에 바로 끝나도록 합니다.
	InstanceData.CheckElapsed = AgEnemyAITasksPrivate::RetryInterval;
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

	// 고른 거리와 다가가기는 다음에 다시 고를 때까지 인스턴스 데이터에 남아, 그 사이의 틱은 그 값으로 걷습니다.
	if (InstanceData.CheckElapsed >= AgEnemyAITasksPrivate::RetryInterval || bDelayEnded)
	{
		InstanceData.CheckElapsed = 0.f;
		if (AgEnemyAITasksPrivate::ChooseTargetRange(Controller, *Enemy, *Target, Distance, InstanceData))
		{
			return EStateTreeRunStatus::Succeeded;
		}
	}
	if (!InstanceData.bApproaching)
	{
		AgEnemyAITasksPrivate::WalkAtTargetRange(Controller, *Enemy, *Target, Distance, InstanceData);
	}
	return EStateTreeRunStatus::Running;
}

void FAgEnemyAITask_KeepDistance::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	AgEnemyAITasksPrivate::StopPathMove(*Context.GetInstanceData(*this).Controller);
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
	// 반응형 패턴이 끝나면 간격 지키기를 거치지 않고 이 상태로 오므로, 시작해도 되는지 여기서도 묻습니다.
	if (!Target || !CombatSubsystem->CanStartPattern(*Enemy, InstanceData.bResponse))
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

	// 후보는 발동할 수 있는지 스펙으로 확인한 패턴입니다.
	const FGameplayAbilitySpecHandle Handle = Enemy->FindAbilitySpec(Pattern)->Handle;
	if (!ASC->TryActivateAbility(Handle))
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
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const UAbilitySystemComponent* ASC = InstanceData.Controller->GetEnemy()->GetAbilitySystemComponent();
	const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(InstanceData.Handle);
	return Spec && Spec->IsActive() ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

void FAgEnemyAITask_RunPattern::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AAgEnemyAIController& Controller = *InstanceData.Controller;
	AAgEnemyCharacter* Enemy = Controller.GetEnemy();
	// 패턴을 시작하지 못하고 나가는 길(EnterState 실패)입니다.
	if (!InstanceData.Handle.IsValid())
	{
		return;
	}
	UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent();
	const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(InstanceData.Handle);
	// 패턴 금지나 플레이어 사망처럼 패턴이 끝나기 전에 다른 상태로 가면 패턴을 멈춥니다.
	if (Spec && Spec->IsActive())
	{
		ASC->CancelAbilityHandle(InstanceData.Handle);
	}
	// 피격 반응으로 끊겼으면 후딜레이를 건드리지 않고, 그 밖에는 멈춘 패턴도 끝까지 마친 것으로 봅니다.
	if (!Enemy->IsReacting())
	{
		// 반응형 패턴(Backstep)은 끝까지 마치면 남은 후딜레이 없이 바로 다음 패턴을 고릅니다.
		if (InstanceData.bResponse)
		{
			Controller.ClearPostPatternDelay();
		}
		else
		{
			Controller.StartPostPatternDelay();
		}
	}
	if (!InstanceData.bResponse)
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
	return InstanceData.Controller->GetEnemy()->IsPhaseTransitionActive() ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

EStateTreeRunStatus FAgEnemyAITask_WaitReaction::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	return Tick(Context, 0.f);
}

EStateTreeRunStatus FAgEnemyAITask_WaitReaction::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	return InstanceData.Controller->GetEnemy()->IsReacting() ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
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
	AgEnemyAITasksPrivate::UpdateCombatTarget(Controller);
	Controller.GetWorld()->GetSubsystem<UAgCombatSubsystem>()->AddCombatant(*Controller.GetEnemy());
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FAgEnemyAITask_Combat::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	AgEnemyAITasksPrivate::UpdateCombatTarget(*Context.GetInstanceData(*this).Controller);
	return EStateTreeRunStatus::Running;
}

void FAgEnemyAITask_Combat::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	AAgEnemyAIController& Controller = *Context.GetInstanceData(*this).Controller;
	Controller.ClearFocus(EAIFocusPriority::Gameplay);
	Controller.GetEnemy()->GetAbilitySystemComponent()->CancelAbilitiesWithSpecTags(FGameplayTagContainer(AgGameplayTags::Ability_LockOn));
	Controller.GetWorld()->GetSubsystem<UAgCombatSubsystem>()->RemoveCombatant(*Controller.GetEnemy());
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
				AgEnemyAITasksPrivate::RequestReachablePointMove(Controller, *Enemy, InstanceData);
			}
			else
			{
				AgEnemyAITasksPrivate::RequestPatrolMove(Controller, Enemy->GetHomeTransform().GetLocation());
			}
			return EStateTreeRunStatus::Running;
		}
		InstanceData.PointIndex = INDEX_NONE;
	}

	if (InstanceData.PointIndex == INDEX_NONE)
	{
		const FTransform& Home = Enemy->GetHomeTransform();
		Controller.SetFocalPoint(Enemy->GetActorLocation() + Home.GetRotation().GetForwardVector() * AgEnemyAITasksPrivate::HomeFacingDistance);
		return EStateTreeRunStatus::Running;
	}
	InstanceData.FailedCount = 0;
	InstanceData.PointIndex = (InstanceData.PointIndex + 1) % Enemy->GetPatrolRoute()->GetPointCount();
	AgEnemyAITasksPrivate::RequestReachablePointMove(Controller, *Enemy, InstanceData);
	return EStateTreeRunStatus::Running;
}

void FAgEnemyAITask_Patrol::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	AAgEnemyAIController& Controller = *Context.GetInstanceData(*this).Controller;
	Controller.StopMovement();
	Controller.ClearFocus(EAIFocusPriority::Gameplay);
}
