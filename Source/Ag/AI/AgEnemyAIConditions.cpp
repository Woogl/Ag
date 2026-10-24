// Copyright Woogle. All Rights Reserved.

#include "AI/AgEnemyAIConditions.h"

#include "Abilities/GameplayAbility.h"
#include "Data/AgAbilityData.h"
#include "AbilitySystemComponent.h"
#include "AI/AgEnemyAIController.h"
#include "Character/AgEnemyCharacter.h"
#include "Data/AgCharacterData.h"
#include "Kismet/GameplayStatics.h"
#include "StateTreeExecutionContext.h"

bool FAgEnemyAICondition_Dead::TestCondition(FStateTreeExecutionContext& Context) const
{
	const AAgEnemyAIController* Controller = Context.GetInstanceData(*this).Controller;
	return Controller->GetEnemy()->IsDead();
}

bool FAgEnemyAICondition_PlayerDead::TestCondition(FStateTreeExecutionContext& Context) const
{
	// 락온 대상은 죽으면 놓으므로 대상이 아니라 플레이어를 봅니다.
	const AAgEnemyAIController* Controller = Context.GetInstanceData(*this).Controller;
	const AAgCharacterBase* Player = Cast<AAgCharacterBase>(UGameplayStatics::GetPlayerCharacter(Controller, 0));
	return Player && Player->IsDead();
}

bool FAgEnemyAICondition_Reacting::TestCondition(FStateTreeExecutionContext& Context) const
{
	const AAgEnemyAIController* Controller = Context.GetInstanceData(*this).Controller;
	return Controller->IsEnemyReacting();
}

bool FAgEnemyAICondition_Flinching::TestCondition(FStateTreeExecutionContext& Context) const
{
	const AAgEnemyAIController* Controller = Context.GetInstanceData(*this).Controller;
	return Controller->IsEnemyFlinching() != bInvert;
}

bool FAgEnemyAICondition_PhaseTransitionReady::TestCondition(FStateTreeExecutionContext& Context) const
{
	const AAgEnemyAIController* Controller = Context.GetInstanceData(*this).Controller;
	const AAgEnemyCharacter* Enemy = Controller->GetEnemy();
	return !Enemy->IsDead() && !Controller->IsEnemyReacting() && Enemy->GetReachedPhase() > Controller->GetPhaseTransitionTarget();
}

bool FAgEnemyAICondition_ResponseReady::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const AAgEnemyAIController* Controller = InstanceData.Controller;
	const AAgEnemyCharacter* Enemy = Controller->GetEnemy();
	const FAgAbilityData* Pattern = Controller->GetEnemyData()->FindAbility(InstanceData.Pattern);
	const FAgPatternSelection* Params = Pattern ? Pattern->PatternSelection.GetPtrOrNull() : nullptr;
	const AAgCharacterBase* Target = Controller->GetTarget();
	if (!Params || !Target || !Params->IsAvailable(Enemy->GetHorizontalDistanceTo(Target), Enemy->GetPhase()))
	{
		return false;
	}

	const FGameplayAbilitySpec* Spec = Enemy->FindAbilitySpec(InstanceData.Pattern);
	const FGameplayAbilityActorInfo* ActorInfo = Enemy->GetAbilitySystemComponent()->AbilityActorInfo.Get();
	return Spec && Spec->Ability && Spec->GetPrimaryInstance()->CanActivateAbility(Spec->Handle, ActorInfo) &&
	       FMath::FRand() < Params->ResponseChance;
}

bool FAgEnemyAICondition_TargetNoticed::TestCondition(FStateTreeExecutionContext& Context) const
{
	return Context.GetInstanceData(*this).Controller->IsTargetNoticed();
}

bool FAgEnemyAICondition_TargetLost::TestCondition(FStateTreeExecutionContext& Context) const
{
	const AAgEnemyAIController* Controller = Context.GetInstanceData(*this).Controller;
	return Controller->GetEnemy()->CanReturnToNonCombat() && !Controller->IsTargetNoticed();
}
