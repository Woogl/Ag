// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Data/AgAttackData.h"
#include "GameplayTagContainer.h"
#include "AgBossAIController.generated.h"

class AAgBossCharacter;
class AAgCharacterBase;
class UAbilitySystemComponent;
class UAgBossData;
struct FAbilityEndedData;
struct FGameplayEventData;

/**
 * Boss AI (보스 사양 'AI 행동 규칙', '전투 시작과 행동 전환'):
 * wait → pick a pattern → run it → pattern recovery wait → pick again.
 * During the wait the boss walks sideways around the player. With no candidate it walks to the player and retries.
 * Groggy, execution, phase transition and death are state abilities. After the player dies the boss stops (비전투).
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
		/** 패턴 후딜레이: waits while walking sideways. */
		Waiting,
		Approaching,
		Pattern,
		PhaseTransition,
		/** 비전투: the player died; the boss holds its idle pose and only its own death still applies. */
		NonCombat,
		Stopped,
	};

	/** 패턴 후딜레이: waits, walking left or right (picked at random each time), then picks a pattern. */
	void Wait(float Seconds);

	/** 패턴 선택 */
	void SelectPattern();

	/** Runs a pattern now. False if it couldn't start. */
	bool StartPattern(const FGameplayTag& Pattern);

	void HandleAbilityEnded(const FAbilityEndedData& EndedData);
	void HandleBossDied(AAgCharacterBase* DeadBoss);

	/** 그로기 stops the AI; when it ends (with or without an execution) the boss waits and picks again. */
	void HandleGroggyChanged(const FGameplayTag Tag, int32 NewCount);

	/** 전투 시작과 행동 전환 5: starts the phase transition once phase 2 HP is reached outside a pattern and groggy. */
	void TryStartPhaseTransition();

	/** 전투 시작과 행동 전환 2: the running pattern stops at once and nothing but the boss's own death applies anymore. */
	void EnterNonCombat();

	/** Listens to the player's basic attack hits once the player exists. */
	void BindPlayerEvents();

	/** B1 백스텝: checked every time the player starts a basic attack hit. */
	void HandlePlayerBasicAttack(const FGameplayEventData* Payload);

	const UAgBossData* GetBossData() const;

	/** The current phase (1 or 2). */
	int32 GetPhase() const;

	TWeakObjectPtr<AAgBossCharacter> Boss;
	TWeakObjectPtr<UAbilitySystemComponent> PlayerASC;
	FDelegateHandle BasicAttackHandle;
	EState State = EState::Stopped;
	bool bPhaseTransitionStarted = false;
	FGameplayTag RunningPattern;

	/** 좌우 걷기: +1 walks right, -1 left. */
	float StrafeDirection = 1.f;

	FTimerHandle WaitTimer;
	FTimerHandle RetryTimer;
};
