// Copyright Woogle. All Rights Reserved.

#include "Combat/AgCombatSubsystem.h"

#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "AI/AgEnemyAIController.h"
#include "Character/AgBossCharacter.h"
#include "Character/AgEnemyCharacter.h"
#include "Core/AgAssetManager.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCombatRules.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

namespace AgCombatSubsystemPrivate
{
	bool IsWaitingForTurn(const AAgEnemyCharacter& Enemy)
	{
		const AAgEnemyAIController* Controller = Cast<AAgEnemyAIController>(Enemy.GetController());
		if (!Controller || !Controller->GetTarget() || Enemy.IsDead() || Controller->IsInPostPatternDelay() || Controller->IsEnemyReacting() || Enemy.IsPhaseTransitionActive())
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
} // namespace AgCombatSubsystemPrivate

void UAgCombatSubsystem::AddCombatant(AAgEnemyCharacter& Enemy)
{
	Combatants.AddDefaulted_GetRef().Enemy = &Enemy;
	OnCombatantsChanged.Broadcast();
}

void UAgCombatSubsystem::RemoveCombatant(const AAgEnemyCharacter& Enemy)
{
	Combatants.RemoveAll([&Enemy](const FAgCombatant& Combatant)
	{ return Combatant.Enemy.Get() == &Enemy; });
	OnCombatantsChanged.Broadcast();
}

void UAgCombatSubsystem::GetCombatants(TArray<AAgEnemyCharacter*>& OutEnemies) const
{
	OutEnemies.Reset();
	for (const FAgCombatant& Combatant : Combatants)
	{
		if (AAgEnemyCharacter* Enemy = Combatant.Enemy.Get())
		{
			OutEnemies.Add(Enemy);
		}
	}
}

AAgBossCharacter* UAgCombatSubsystem::FindBoss() const
{
	for (const FAgCombatant& Combatant : Combatants)
	{
		if (AAgBossCharacter* Boss = Cast<AAgBossCharacter>(Combatant.Enemy.Get()))
		{
			return Boss;
		}
	}
	return nullptr;
}

bool UAgCombatSubsystem::ArePatternsBlocked() const
{
	// 플레이어는 처형당하지 않으므로, 플레이어의 처형 상태는 처형하는 중입니다.
	const AAgCharacterBase* Player = Cast<AAgCharacterBase>(UGameplayStatics::GetPlayerCharacter(this, 0));
	return Player && Player->GetAbilitySystemComponent()->HasMatchingGameplayTag(AgGameplayTags::State_Execution);
}

bool UAgCombatSubsystem::CanStartPattern(const AAgEnemyCharacter& Enemy, bool bResponse) const
{
	if (ArePatternsBlocked())
	{
		return false;
	}
	if (bResponse)
	{
		return true;
	}
	const AAgEnemyAIController* Controller = Cast<AAgEnemyAIController>(Enemy.GetController());
	return Controller && !Controller->IsInPostPatternDelay() && HasAttackTurn(Enemy);
}

void UAgCombatSubsystem::StartAttack(const AAgEnemyCharacter& Enemy)
{
	if (FAgCombatant* Combatant = FindCombatant(Enemy))
	{
		Combatant->bAttacking = true;
	}
}

void UAgCombatSubsystem::EndAttack(const AAgEnemyCharacter& Enemy)
{
	if (FAgCombatant* Combatant = FindCombatant(Enemy))
	{
		Combatant->bAttacking = false;
		Combatant->ReattackWaitEndTime = GetWorld()->GetTimeSeconds() + UAgAssetManager::Get().GetCombatRules().ReattackWaitTime;
	}
}

bool UAgCombatSubsystem::HasAttackTurn(const AAgEnemyCharacter& Enemy) const
{
	const FAgCombatant* Self = FindCombatant(Enemy);
	if (!Self)
	{
		return false;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	const bool bSelfReattackWaiting = Self->ReattackWaitEndTime > Now;
	const float SelfDistance = AgCombatSubsystemPrivate::GetTargetDistance(Enemy);

	// 앞 순서의 적은 이미 패턴을 시작했든 아직 기다리든 한 자리로 세므로, 같은 프레임에서 적들이 묻는 순서와 관계없이 결과가
	// 같습니다.
	int32 TakenTurns = 0;
	for (const FAgCombatant& Other : Combatants)
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
		else if (AgCombatSubsystemPrivate::IsWaitingForTurn(*OtherEnemy))
		{
			const bool bOtherReattackWaiting = Other.ReattackWaitEndTime > Now;
			const bool bAhead = bOtherReattackWaiting != bSelfReattackWaiting ? !bOtherReattackWaiting : AgCombatSubsystemPrivate::GetTargetDistance(*OtherEnemy) < SelfDistance;
			TakenTurns += bAhead ? 1 : 0;
		}
	}
	return TakenTurns < UAgAssetManager::Get().GetCombatRules().MaxAttackers;
}

UAgCombatSubsystem::FAgCombatant* UAgCombatSubsystem::FindCombatant(const AAgEnemyCharacter& Enemy)
{
	return Combatants.FindByPredicate([&Enemy](const FAgCombatant& Combatant)
	{ return Combatant.Enemy.Get() == &Enemy; });
}

const UAgCombatSubsystem::FAgCombatant* UAgCombatSubsystem::FindCombatant(const AAgEnemyCharacter& Enemy) const
{
	return Combatants.FindByPredicate([&Enemy](const FAgCombatant& Combatant)
	{ return Combatant.Enemy.Get() == &Enemy; });
}
