// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Data/AgAttackData.h"
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

	/**
	 * 패턴 선택 1: the patterns usable now (the distance band's weight above 0, the phase allows it, IsReady says the
	 * cooldown is over) and their weights in that band. Static so the selection rules can be tested without a world.
	 */
	static void GatherCandidates(TConstArrayView<FAgBossPattern> Patterns, float Distance, float NearDistance, float FarDistance, int32 Phase,
		TFunctionRef<bool(const FAgBossPattern&)> IsReady, TArray<int32>& OutIndices, TArray<float>& OutWeights);

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
		PhaseTransition,
		Stopped,
	};

	/** Waits, then picks a pattern. */
	void Wait(float Seconds);

	/** 패턴 선택 */
	void SelectPattern();

	void HandleAbilityEnded(const FAbilityEndedData& EndedData);
	void HandleBossDied(AAgCharacterBase* DeadBoss);

	/** 그로기 stops the AI; when it ends (with or without an execution) the boss waits and picks again. */
	void HandleGroggyChanged(const FGameplayTag Tag, int32 NewCount);

	/** 전투 시작과 행동 전환 5: starts the phase transition once phase 2 HP is reached outside a pattern and groggy. */
	void TryStartPhaseTransition();

	const UAgBossData* GetBossData() const;

	/** The current phase (1 or 2). */
	int32 GetPhase() const;

	TWeakObjectPtr<AAgBossCharacter> Boss;
	EState State = EState::Stopped;
	bool bPhaseTransitionStarted = false;
	FGameplayTag RunningPattern;
	FTimerHandle WaitTimer;
	FTimerHandle RetryTimer;
};
