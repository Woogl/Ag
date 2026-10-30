// Copyright Woogle. All Rights Reserved.

#include "AI/AgEnemyAIConditions.h"

#include "AI/AgEnemyAIController.h"
#include "Character/AgEnemyCharacter.h"
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
	return !Enemy->IsDead() && !Controller->IsEnemyReacting() && Enemy->IsPhaseTransitionPending();
}

bool FAgEnemyAICondition_ResponseReady::TestCondition(FStateTreeExecutionContext& Context) const
{
	const AAgEnemyAIController* Controller = Context.GetInstanceData(*this).Controller;
	const AAgEnemyCharacter* Enemy = Controller->GetEnemy();
	const AAgCharacterBase* Target = Controller->GetTarget();
	if (!Target)
	{
		return false;
	}

	TArray<TPair<FName, float>> Responses;
	Enemy->GatherPatterns(Enemy->GetHorizontalDistanceTo(Target), true, Responses);
	// 반응 태스크가 확률을 가중치로 하나를 다시 고르므로, 각 반응 패턴이 자기 확률로 나오도록 합으로 한 번만 굴립니다.
	float TotalChance = 0.f;
	for (const TPair<FName, float>& Response : Responses)
	{
		TotalChance += Response.Value;
	}
	return FMath::FRand() < TotalChance;
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
