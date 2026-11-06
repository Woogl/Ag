// Copyright Woogle. All Rights Reserved.

#include "Combat/AgCombatSubsystem.h"

#include "AI/AgEnemyAIController.h"
#include "Character/AgBossCharacter.h"
#include "Character/AgEnemyCharacter.h"
#include "Core/AgAssetManager.h"
#include "Data/AgCombatRules.h"
#include "Engine/World.h"

namespace
{
	bool IsWaitingForTurn(const AAgEnemyCharacter& Enemy)
	{
		const AAgEnemyAIController* Controller = Cast<AAgEnemyAIController>(Enemy.GetController());
		if (!Controller || !Controller->GetTarget() || Enemy.IsDead() || Controller->IsInPostPatternDelay() || Controller->IsEnemyReacting()
			|| Enemy.IsPhaseTransitionActive())
		{
			return false;
		}
		TArray<FFloatInterval> ReadyRanges;
		Enemy.GatherReadyPatternRanges(ReadyRanges);
		return !ReadyRanges.IsEmpty();
	}

	float GetTargetDistance(const AAgEnemyCharacter& Enemy)
	{
		const AAgEnemyAIController* Controller = Cast<AAgEnemyAIController>(Enemy.GetController());
		const AAgCharacterBase* Target = Controller ? Controller->GetTarget() : nullptr;
		return Target ? Enemy.GetHorizontalDistanceTo(Target) : TNumericLimits<float>::Max();
	}
} // namespace

void UAgCombatSubsystem::AddCombatant(AAgEnemyCharacter& Enemy)
{
	Combatants.AddDefaulted_GetRef().Enemy = &Enemy;
	OnCombatantsChanged.Broadcast();
}

void UAgCombatSubsystem::RemoveCombatant(const AAgEnemyCharacter& Enemy)
{
	Combatants.RemoveAll([&Enemy](const FCombatant& Combatant)
	{ return Combatant.Enemy.Get() == &Enemy; });
	OnCombatantsChanged.Broadcast();
}

AAgBossCharacter* UAgCombatSubsystem::FindBoss() const
{
	for (const FCombatant& Combatant : Combatants)
	{
		if (AAgBossCharacter* Boss = Cast<AAgBossCharacter>(Combatant.Enemy.Get()))
		{
			return Boss;
		}
	}
	return nullptr;
}

bool UAgCombatSubsystem::HasAttackTurn(const AAgEnemyCharacter& Enemy) const
{
	const FCombatant* Self = FindCombatant(Enemy);
	if (!Self)
	{
		return false;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	const bool bSelfReattackWaiting = Self->ReattackWaitEndTime > Now;
	const float SelfDistance = GetTargetDistance(Enemy);

	// 앞 순서의 적은 이미 패턴을 시작했든 아직 기다리든 한 자리로 세므로, 같은 프레임에서 적들이 묻는 순서와 관계없이 결과가
	// 같습니다.
	int32 TakenTurns = 0;
	for (const FCombatant& Other : Combatants)
	{
		const AAgEnemyCharacter* OtherEnemy = Other.Enemy.Get();
		if (!OtherEnemy || OtherEnemy == &Enemy)
		{
			continue;
		}
		if (Other.bAttacking)
		{
			++TakenTurns;
		}
		else if (IsWaitingForTurn(*OtherEnemy))
		{
			const bool bOtherReattackWaiting = Other.ReattackWaitEndTime > Now;
			const bool bAhead = bOtherReattackWaiting != bSelfReattackWaiting ? !bOtherReattackWaiting : GetTargetDistance(*OtherEnemy) < SelfDistance;
			TakenTurns += bAhead ? 1 : 0;
		}
	}
	return TakenTurns < UAgAssetManager::Get().GetCombatRules().MaxAttackers;
}

void UAgCombatSubsystem::StartAttack(const AAgEnemyCharacter& Enemy)
{
	if (FCombatant* Combatant = FindCombatant(Enemy))
	{
		Combatant->bAttacking = true;
	}
}

void UAgCombatSubsystem::EndAttack(const AAgEnemyCharacter& Enemy)
{
	if (FCombatant* Combatant = FindCombatant(Enemy))
	{
		Combatant->bAttacking = false;
		Combatant->ReattackWaitEndTime = GetWorld()->GetTimeSeconds() + UAgAssetManager::Get().GetCombatRules().ReattackWaitTime;
	}
}

UAgCombatSubsystem::FCombatant* UAgCombatSubsystem::FindCombatant(const AAgEnemyCharacter& Enemy)
{
	return Combatants.FindByPredicate([&Enemy](const FCombatant& Combatant)
	{ return Combatant.Enemy.Get() == &Enemy; });
}

const UAgCombatSubsystem::FCombatant* UAgCombatSubsystem::FindCombatant(const AAgEnemyCharacter& Enemy) const
{
	return Combatants.FindByPredicate([&Enemy](const FCombatant& Combatant)
	{ return Combatant.Enemy.Get() == &Enemy; });
}
