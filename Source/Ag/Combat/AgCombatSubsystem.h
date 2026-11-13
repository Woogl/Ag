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

	/** 전투 중인 적을 전투에 들어간 순서로 돌려줍니다. */
	void GetCombatants(TArray<AAgEnemyCharacter*>& OutEnemies) const;

	AAgBossCharacter* FindBoss() const;

	/** 플레이어가 처형하는 동안 참입니다. 이때는 어떤 적도 패턴을 시작하지 않고, 실행 중인 패턴도 멈춥니다. */
	bool ArePatternsBlocked() const;

	/**
	 * 그 적이 지금 패턴을 시작해도 되는지입니다. 패턴을 시작하기 전에만 묻습니다(공격 중에 물으면 앞 순서의 적 때문에 거짓일 수
	 * 있습니다). 쿨다운·거리처럼 패턴마다의 조건은 보지 않습니다. 반응형 패턴은 패턴 후딜레이 중에 나오고 차례를 차지하지 않으므로
	 * 패턴 금지만 봅니다.
	 */
	bool CanStartPattern(const AAgEnemyCharacter& Enemy, bool bResponse) const;

	void StartAttack(const AAgEnemyCharacter& Enemy);

	/** 재공격 대기를 시작합니다. */
	void EndAttack(const AAgEnemyCharacter& Enemy);

	FSimpleMulticastDelegate OnCombatantsChanged;

private:
	struct FAgCombatant
	{
		TWeakObjectPtr<AAgEnemyCharacter> Enemy;
		bool bAttacking = false;
		double ReattackWaitEndTime = 0.0;
	};

	/** 차례를 기다리는 적이 물어야 합니다. 다른 적이 차지한 차례가 동시 공격 수보다 적으면 참입니다. */
	bool HasAttackTurn(const AAgEnemyCharacter& Enemy) const;

	FAgCombatant* FindCombatant(const AAgEnemyCharacter& Enemy);
	const FAgCombatant* FindCombatant(const AAgEnemyCharacter& Enemy) const;

	TArray<FAgCombatant> Combatants;
};
