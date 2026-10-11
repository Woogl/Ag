// Copyright Woogle. All Rights Reserved.

#include "AI/AgBossAIConditions.h"

#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AI/AgBossAIController.h"
#include "Character/AgBossCharacter.h"
#include "Combat/AgCombatLibrary.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "StateTreeExecutionContext.h"

bool FAgBossAICondition_BossDead::TestCondition(FStateTreeExecutionContext& Context) const
{
	const AAgBossAIController* Controller = Context.GetInstanceData(*this).Controller;
	const AAgBossCharacter* Boss = Controller ? Controller->GetBoss() : nullptr;
	return Boss && Boss->IsDead();
}

bool FAgBossAICondition_PlayerDead::TestCondition(FStateTreeExecutionContext& Context) const
{
	const AAgBossAIController* Controller = Context.GetInstanceData(*this).Controller;
	const AAgCharacterBase* Target = Controller ? Controller->GetTarget() : nullptr;
	return Target && Target->IsDead();
}

bool FAgBossAICondition_Reacting::TestCondition(FStateTreeExecutionContext& Context) const
{
	const AAgBossAIController* Controller = Context.GetInstanceData(*this).Controller;
	return Controller && Controller->IsBossReacting();
}

bool FAgBossAICondition_PhaseTransitionReady::TestCondition(FStateTreeExecutionContext& Context) const
{
	const AAgBossAIController* Controller = Context.GetInstanceData(*this).Controller;
	const AAgBossCharacter* Boss = Controller ? Controller->GetBoss() : nullptr;
	const UAgBossData* Data = Controller ? Controller->GetBossData() : nullptr;
	if (!Boss || !Data || Boss->IsDead() || Controller->HasStartedPhaseTransition() || Controller->IsBossReacting())
	{
		return false;
	}

	const UAgAttributeSet* Stats = Boss->GetAttributeSet();
	return Stats && Stats->GetHP() <= Stats->GetMaxHP() * Data->Phase2HPRatio;
}

bool FAgBossAICondition_BackstepReady::TestCondition(FStateTreeExecutionContext& Context) const
{
	const AAgBossAIController* Controller = Context.GetInstanceData(*this).Controller;
	const AAgBossCharacter* Boss = Controller ? Controller->GetBoss() : nullptr;
	const UAgBossData* Data = Controller ? Controller->GetBossData() : nullptr;
	if (!Boss || !Data || UAgCombatLibrary::GetHorizontalDistance(Boss, Controller->GetTarget()) > Data->BackstepRange)
	{
		return false;
	}

	// 쿨다운이 끝났을 때만, 그리고 확률로
	const FGameplayAbilitySpec* Spec = Boss->FindPatternSpec(AgGameplayTags::Ability_Boss_Pattern_B1);
	const FGameplayAbilityActorInfo* ActorInfo = Boss->GetAbilitySystemComponent()->AbilityActorInfo.Get();
	return Spec && Spec->Ability && Spec->Ability->CanActivateAbility(Spec->Handle, ActorInfo) && FMath::FRand() < Data->BackstepChance;
}
