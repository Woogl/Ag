// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GameplayTagContainer.h"
#include "AgBossAIController.generated.h"

class AAgBossCharacter;
class AAgCharacterBase;
class UAgBossData;
struct FAbilityEndedData;

/**
 * Boss AI (보스 사양 'AI 행동 규칙', '전투 시작과 행동 전환'):
 * wait → pick a pattern → run it → pattern recovery wait → pick again.
 * With no candidate it walks to the player and retries. Groggy, execution, phase transition and death are state abilities.
 */
UCLASS()
class AAgBossAIController : public AAIController
{
	GENERATED_BODY()

public:
	AAgBossAIController();

	/** Picks an index by weight; Random01 in [0, 1). Returns INDEX_NONE when every weight is 0. */
	static int32 PickWeighted(TConstArrayView<float> Weights, float Random01);

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	enum class EState : uint8
	{
		Waiting,
		Approaching,
		Pattern,
		Stopped,
	};

	/** Waits, then picks a pattern. */
	void Wait(float Seconds);

	/** 패턴 선택 */
	void SelectPattern();

	void HandleAbilityEnded(const FAbilityEndedData& EndedData);
	void HandleBossDied(AAgCharacterBase* DeadBoss);

	const UAgBossData* GetBossData() const;

	/** The current phase (1 or 2). */
	int32 GetPhase() const { return 1; }

	TWeakObjectPtr<AAgBossCharacter> Boss;
	EState State = EState::Stopped;
	FGameplayTag RunningPattern;
	FTimerHandle WaitTimer;
	FTimerHandle RetryTimer;
};
