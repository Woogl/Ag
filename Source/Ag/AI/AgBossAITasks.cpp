// Copyright Woogle. All Rights Reserved.

#include "AI/AgBossAITasks.h"

#include "Abilities/GameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "AI/AgBossAIController.h"
#include "Algo/SelectRandomWeighted.h"
#include "Character/AgBossCharacter.h"
#include "Core/AgAssetManager.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "StateTreeExecutionContext.h"

namespace
{
	constexpr float RetryInterval = 0.5f;

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
	const FFloatInterval& RecoveryTime = InstanceData.Controller->GetBossData()->PostPatternDelayTime;
	InstanceData.Duration = FMath::FRandRange(RecoveryTime.Min, RecoveryTime.Max);
	InstanceData.Elapsed = 0.f;
	InstanceData.StrafeDirection = FMath::RandBool() ? 1.f : -1.f;
	return InstanceData.Duration > 0.f ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

EStateTreeRunStatus FAgBossAITask_Wait::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AAgBossCharacter* Boss = InstanceData.Controller->GetBoss();

	// 보스는 스스로 플레이어를 향해 돌므로, 옆으로 걸으면 플레이어 주위를 돕니다.
	Boss->AddMovementInput(Boss->GetActorRightVector(), InstanceData.StrafeDirection);
	InstanceData.Elapsed += DeltaTime;
	return InstanceData.Elapsed >= InstanceData.Duration ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FAgBossAITask_Approach::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.Elapsed = 0.f;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FAgBossAITask_Approach::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AAgBossCharacter* Boss = InstanceData.Controller->GetBoss();
	if (const AAgCharacterBase* Target = InstanceData.Controller->GetTarget())
	{
		Boss->AddMovementInput((Target->GetActorLocation() - Boss->GetActorLocation()).GetSafeNormal2D(), 1.f);
	}
	InstanceData.Elapsed += DeltaTime;
	return InstanceData.Elapsed >= RetryInterval ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FAgBossAITask_RunPattern::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
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
		const AAgCharacterBase* Target = InstanceData.Controller->GetTarget();
		if (!Target)
		{
			return EStateTreeRunStatus::Failed;
		}
		const float Distance = Boss->GetHorizontalDistanceTo(Target);
		const FGameplayAbilityActorInfo* ActorInfo = ASC->AbilityActorInfo.Get();
		const auto IsReady = [Boss, ActorInfo](FName Candidate)
		{
			const FGameplayAbilitySpec* Spec = Boss->FindAbilitySpec(Candidate);
			return Spec && Spec->Ability && Spec->GetPrimaryInstance()->CanActivateAbility(Spec->Handle, ActorInfo);
		};
		TArray<TPair<FName, float>> Candidates;
		GatherCandidates(Data->Abilities, Distance, Boss->GetPhase(), IsReady, Candidates);

		const TPair<FName, float>* Picked =
			Algo::SelectRandomWeightedBy(Candidates, [](const TPair<FName, float>& Candidate) { return Candidate.Value; });
		if (!Picked)
		{
			return EStateTreeRunStatus::Failed;
		}
		Pattern = Picked->Key;
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
	// 패턴이 넉다운이나 그로기로 끊겨도 여기서는 끝난 것으로 봅니다. 그때는 우선순위가 높은 피격 반응 전환이
	// 먼저 적용됩니다.
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const UAbilitySystemComponent* ASC = InstanceData.Controller->GetBoss()->GetAbilitySystemComponent();
	const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(InstanceData.Handle);
	return Spec && Spec->IsActive() ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

void FAgBossAITask_RunPattern::GatherCandidates(TConstArrayView<FName> AbilityIds, float Distance, int32 Phase, TFunctionRef<bool(FName)> IsReady, TArray<TPair<FName, float>>& OutCandidates)
{
	OutCandidates.Reset();
	for (FName Id : AbilityIds)
	{
		const FAgAbilityData* Ability = UAgAssetManager::Get().FindAbility(Id);
		const FAgPatternSelection* Pattern =
			Ability && Ability->Tags.HasTag(AgGameplayTags::Ability_Boss_Pattern) ? Ability->PatternSelection.GetPtrOrNull() : nullptr;
		if (!Pattern)
		{
			continue;
		}

		if (Pattern->Weight > 0.f && Pattern->IsAvailable(Distance, Phase) && IsReady(Id))
		{
			OutCandidates.Emplace(Id, Pattern->Weight);
		}
	}
}

EStateTreeRunStatus FAgBossAITask_PhaseTransition::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AAgBossCharacter* Boss = InstanceData.Controller->GetBoss();
	const UAbilitySystemComponent* ASC = Boss->GetAbilitySystemComponent();

	InstanceData.Controller->MarkPhaseTransitionStarted(Boss->GetReachedPhase());
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

EStateTreeRunStatus FAgBossAITask_WaitReaction::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	return InstanceData.Controller->IsBossReacting() ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

EStateTreeRunStatus FAgBossAITask_WaitReaction::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	return InstanceData.Controller->IsBossReacting() ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

FAgBossAITask_NonCombat::FAgBossAITask_NonCombat()
{
	bShouldCallTick = false;
}

EStateTreeRunStatus FAgBossAITask_NonCombat::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	UAbilitySystemComponent* ASC = InstanceData.Controller->GetBoss()->GetAbilitySystemComponent();
	ASC->AddLooseGameplayTag(AgGameplayTags::State_NonCombat);
	const FGameplayTagContainer PatternTags(AgGameplayTags::Ability_Boss_Pattern);
	CastChecked<UAgAbilitySystemComponent>(ASC)->CancelAbilitiesWithSpecTags(PatternTags);
	return EStateTreeRunStatus::Running;
}
