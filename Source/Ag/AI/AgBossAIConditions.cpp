// Copyright Woogle. All Rights Reserved.

#include "AI/AgBossAIConditions.h"

#include "Abilities/GameplayAbility.h"
#include "Data/AgAbilityData.h"
#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AI/AgBossAIController.h"
#include "Character/AgBossCharacter.h"
#include "Data/AgCharacterData.h"
#include "StateTreeExecutionContext.h"

bool FAgBossAICondition_BossDead::TestCondition(FStateTreeExecutionContext& Context) const
{
	const AAgBossAIController* Controller = Context.GetInstanceData(*this).Controller;
	return Controller->GetBoss()->IsDead();
}

bool FAgBossAICondition_PlayerDead::TestCondition(FStateTreeExecutionContext& Context) const
{
	const AAgBossAIController* Controller = Context.GetInstanceData(*this).Controller;
	const AAgCharacterBase* Target = Controller->GetTarget();
	return Target && Target->IsDead();
}

bool FAgBossAICondition_Reacting::TestCondition(FStateTreeExecutionContext& Context) const
{
	const AAgBossAIController* Controller = Context.GetInstanceData(*this).Controller;
	return Controller->IsBossReacting();
}

bool FAgBossAICondition_PhaseTransitionReady::TestCondition(FStateTreeExecutionContext& Context) const
{
	const AAgBossAIController* Controller = Context.GetInstanceData(*this).Controller;
	const AAgBossCharacter* Boss = Controller->GetBoss();
	const UAgBossData* Data = Controller->GetBossData();
	if (Boss->IsDead() || Controller->HasStartedPhaseTransition() || Controller->IsBossReacting())
	{
		return false;
	}

	const UAgAttributeSet* Stats = Boss->GetAttributeSet();
	return Stats->GetHP() <= Stats->GetMaxHP() * Data->Phase2HPRatio;
}

bool FAgBossAICondition_BackstepReady::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const AAgBossAIController* Controller = InstanceData.Controller;
	const AAgBossCharacter* Boss = Controller->GetBoss();
	const FAgAbilityData* Pattern = Controller->GetBossData()->FindAbility(InstanceData.Pattern);
	const FAgPatternSelection* Params = Pattern && Pattern->bHasPatternSelection ? &Pattern->PatternSelection : nullptr;
	const AAgCharacterBase* Target = Controller->GetTarget();
	if (!Params || !Target || Boss->GetHorizontalDistanceTo(Target) > Params->ResponseRange)
	{
		return false;
	}

	const FGameplayAbilitySpec* Spec = Boss->FindAbilitySpec(InstanceData.Pattern);
	const FGameplayAbilityActorInfo* ActorInfo = Boss->GetAbilitySystemComponent()->AbilityActorInfo.Get();
	return Spec && Spec->Ability && Spec->GetPrimaryInstance()->CanActivateAbility(Spec->Handle, ActorInfo) &&
	       FMath::FRand() < Params->ResponseChance;
}
