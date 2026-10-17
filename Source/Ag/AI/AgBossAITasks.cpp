// Copyright Woogle. All Rights Reserved.

#include "AI/AgBossAITasks.h"

#include "Abilities/GameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "AI/AgBossAIController.h"
#include "Character/AgBossCharacter.h"
#include "Core/AgAssetManager.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "StateTreeExecutionContext.h"

namespace
{
	bool IsPhaseTransitionActive(const UAbilitySystemComponent& ASC)
	{
		for (const FGameplayAbilitySpec& Spec : ASC.GetActivatableAbilities())
		{
			if (Spec.IsActive() && Spec.Ability && Spec.GetDynamicSpecSourceTags().HasTag(AgGameplayTags::Ability_Reaction_PhaseTransition))
			{
				return true;
			}
		}
		return false;
	}

}

EStateTreeRunStatus FAgBossAITask_Wait::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const AAgBossCharacter* Boss = InstanceData.Controller->GetBoss();
	const UAgBossData* Data = InstanceData.Controller->GetBossData();
	if (InstanceData.Wait == EAgBossWait::Fixed)
	{
		InstanceData.Duration = Data->FixedRecoveryWait;
	}
	else
	{
		const FFloatInterval& Range = Boss->GetPhase() == 1 ? Data->PatternRecoveryWaitPhase1 : Data->PatternRecoveryWaitPhase2;
		InstanceData.Duration = FMath::FRandRange(Range.Min, Range.Max);
	}
	InstanceData.Elapsed = 0.f;
	InstanceData.StrafeDirection = FMath::RandBool() ? 1.f : -1.f;
	return InstanceData.Duration > 0.f ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

EStateTreeRunStatus FAgBossAITask_Wait::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AAgBossCharacter* Boss = InstanceData.Controller->GetBoss();

	// 보스는 스스로 플레이어를 향해 돌므로, 옆으로 걸으면 플레이어 주위를 돕니다. 길찾기 없이 이동 입력을 직접 줍니다.
	Boss->AddMovementInput(Boss->GetActorRightVector(), InstanceData.StrafeDirection);
	InstanceData.Elapsed += DeltaTime;
	return InstanceData.Elapsed >= InstanceData.Duration ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FAgBossAITask_Approach::EnterState(
    FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.Elapsed = 0.f;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FAgBossAITask_Approach::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AAgBossCharacter* Boss = InstanceData.Controller->GetBoss();
	const UAgBossData* Data = InstanceData.Controller->GetBossData();
	if (const AAgCharacterBase* Target = InstanceData.Controller->GetTarget())
	{
		Boss->AddMovementInput((Target->GetActorLocation() - Boss->GetActorLocation()).GetSafeNormal2D(), 1.f);
	}
	InstanceData.Elapsed += DeltaTime;
	return InstanceData.Elapsed >= Data->RetryInterval ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FAgBossAITask_RunPattern::EnterState(
    FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.RunningPattern = NAME_None;
	InstanceData.Handle = FGameplayAbilitySpecHandle();

	AAgBossCharacter* Boss = InstanceData.Controller->GetBoss();
	const UAgBossData* Data = InstanceData.Controller->GetBossData();
	UAbilitySystemComponent* ASC = Boss->GetAbilitySystemComponent();

	FName Pattern = InstanceData.Pattern;
	if (Pattern.IsNone())
	{
		// 1. 후보: 지금 거리 구간과 페이즈에서 쓸 수 있고 쿨다운이 끝난 패턴. 대상이 없으면 고르지 않습니다.
		const AAgCharacterBase* Target = InstanceData.Controller->GetTarget();
		if (!Target)
		{
			return EStateTreeRunStatus::Failed;
		}
		const float Distance = Boss->GetHorizontalDistanceTo(Target);
		const FGameplayAbilityActorInfo* ActorInfo = ASC->AbilityActorInfo.Get();
		TArray<FName> Candidates;
		TArray<float> Weights;
		GatherCandidates(
		    Data->Abilities, Distance, Data->NearDistance, Data->FarDistance, Boss->GetPhase(),
		    [Boss, ActorInfo](FName Candidate)
		    {
			    const FGameplayAbilitySpec* Spec = Boss->FindAbilitySpec(Candidate);
			    return Spec && Spec->Ability && Spec->GetPrimaryInstance()->CanActivateAbility(Spec->Handle, ActorInfo);
		    },
		    Candidates, Weights);

		// 2. 가중치로 하나를 고릅니다. 3. 후보가 없으면 실패해 다가가기로 넘어갑니다.
		const int32 Picked = PickWeighted(Weights, FMath::FRand());
		if (Picked == INDEX_NONE)
		{
			return EStateTreeRunStatus::Failed;
		}
		Pattern = Candidates[Picked];
	}

	const FGameplayAbilitySpec* Spec = Boss->FindAbilitySpec(Pattern);
	const FGameplayAbilitySpecHandle Handle = Spec ? Spec->Handle : FGameplayAbilitySpecHandle();
	if (!Handle.IsValid() || !ASC->TryActivateAbility(Handle))
	{
		return EStateTreeRunStatus::Failed;
	}
	InstanceData.Handle = Handle;
	InstanceData.RunningPattern = Pattern;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FAgBossAITask_RunPattern::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	// 패턴이 넉백이나 그로기로 끊겨도 여기서는 끝난 것으로 봅니다. 그때는 우선순위가 높은 피격 반응 전환이 먼저 적용됩니다.
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const UAbilitySystemComponent* ASC = InstanceData.Controller->GetBoss()->GetAbilitySystemComponent();
	const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(InstanceData.Handle);
	return Spec && Spec->IsActive() ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

int32 FAgBossAITask_RunPattern::PickWeighted(TConstArrayView<float> Weights, float Random01)
{
	float Total = 0.f;
	for (const float Weight : Weights)
	{
		Total += FMath::Max(0.f, Weight);
	}
	if (Total <= 0.f)
	{
		return INDEX_NONE;
	}

	// 뽑힐 확률 = 그 패턴의 가중치 ÷ 후보 가중치의 합
	float Threshold = FMath::Clamp(Random01, 0.f, 1.f) * Total;
	int32 LastPositive = INDEX_NONE;
	for (int32 Index = 0; Index < Weights.Num(); ++Index)
	{
		const float Weight = FMath::Max(0.f, Weights[Index]);
		if (Weight <= 0.f)
		{
			continue;
		}
		LastPositive = Index;
		if (Threshold < Weight)
		{
			return Index;
		}
		Threshold -= Weight;
	}
	return LastPositive;
}

void FAgBossAITask_RunPattern::GatherCandidates(TConstArrayView<FName> AbilityIds, float Distance, float NearDistance,
    float FarDistance, int32 Phase, TFunctionRef<bool(FName)> IsReady, TArray<FName>& OutIds, TArray<float>& OutWeights)
{
	OutIds.Reset();
	OutWeights.Reset();
	for (FName Id : AbilityIds)
	{
		const FAgAbilityData* Ability = UAgAssetManager::Get().FindAbility(Id);
		// 패턴이 아닌 어빌리티(피격 반응, 사망 등)는 전용 값이 패턴 공통 값이 아닙니다.
		const FAgPatternSelection* Pattern = Ability && Ability->bHasPatternSelection && Ability->Tags.HasTag(AgGameplayTags::Ability_Boss_Pattern)
		                                         ? &Ability->PatternSelection
		                                         : nullptr;
		if (!Pattern)
		{
			continue;
		}

		// 거리 구간: NearDistance까지 근거리, FarDistance까지 중거리, 그보다 멀면 원거리
		const float Weight =
		    Distance <= NearDistance ? Pattern->WeightNear : (Distance <= FarDistance ? Pattern->WeightMid : Pattern->WeightFar);
		if (Weight > 0.f && Pattern->Phases.Contains(Phase) && IsReady(Id))
		{
			OutIds.Add(Id);
			OutWeights.Add(Weight);
		}
	}
}

EStateTreeRunStatus FAgBossAITask_PhaseTransition::EnterState(
    FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AAgBossCharacter* Boss = InstanceData.Controller->GetBoss();
	const UAbilitySystemComponent* ASC = Boss->GetAbilitySystemComponent();

	// 시작하지 못해도 다시 시도하지 않습니다. 보스전에서 한 번만 시작합니다.
	InstanceData.Controller->MarkPhaseTransitionStarted();
	FGameplayEventData Payload;
	Payload.EventTag = AgGameplayTags::Event_PhaseTransition;
	Payload.Target = Boss;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Boss, AgGameplayTags::Event_PhaseTransition, Payload);
	return IsPhaseTransitionActive(*ASC) ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FAgBossAITask_PhaseTransition::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	return IsPhaseTransitionActive(*InstanceData.Controller->GetBoss()->GetAbilitySystemComponent()) ? EStateTreeRunStatus::Running
	                                                                                                 : EStateTreeRunStatus::Succeeded;
}

EStateTreeRunStatus FAgBossAITask_WaitReaction::EnterState(
    FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	return InstanceData.Controller->IsBossReacting() ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

EStateTreeRunStatus FAgBossAITask_WaitReaction::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	// 넉백 중에 그로기가 되면 그로기가 이어받으므로, 둘 다 끝날 때까지 기다립니다.
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	return InstanceData.Controller->IsBossReacting() ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

FAgBossAITask_NonCombat::FAgBossAITask_NonCombat()
{
	bShouldCallTick = false;
}

EStateTreeRunStatus FAgBossAITask_NonCombat::EnterState(
    FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	UAbilitySystemComponent* ASC = InstanceData.Controller->GetBoss()->GetAbilitySystemComponent();
	ASC->AddLooseGameplayTag(AgGameplayTags::State_NonCombat);
	const FGameplayTagContainer PatternTags(AgGameplayTags::Ability_Boss_Pattern);
	CastChecked<UAgAbilitySystemComponent>(ASC)->CancelAbilitiesWithSpecTags(PatternTags);
	return EStateTreeRunStatus::Running;
}
