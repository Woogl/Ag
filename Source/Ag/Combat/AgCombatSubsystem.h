// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AgCombatSubsystem.generated.h"

class AAgBossCharacter;
class AAgEnemyCharacter;

UCLASS()
class UAgCombatSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void AddCombatant(AAgEnemyCharacter& Enemy);
	void RemoveCombatant(const AAgEnemyCharacter& Enemy);

	AAgBossCharacter* FindBoss() const;

	/** 차례를 기다리는 적이 물어야 합니다. 다른 적이 차지한 차례가 동시 공격 수보다 적으면 참입니다. */
	bool HasAttackTurn(const AAgEnemyCharacter& Enemy) const;

	void StartAttack(const AAgEnemyCharacter& Enemy);

	/** 재공격 대기를 시작합니다. */
	void EndAttack(const AAgEnemyCharacter& Enemy);

	FSimpleMulticastDelegate OnCombatantsChanged;

private:
	struct FCombatant
	{
		TWeakObjectPtr<AAgEnemyCharacter> Enemy;
		bool bAttacking = false;
		double ReattackWaitEndTime = 0.0;
	};

	FCombatant* FindCombatant(const AAgEnemyCharacter& Enemy);
	const FCombatant* FindCombatant(const AAgEnemyCharacter& Enemy) const;

	TArray<FCombatant> Combatants;
};
