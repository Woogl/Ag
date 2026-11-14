// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Game/AgCombatStatsSubsystem.h"
#include "GameplayEffectTypes.h"
#include "AgCombatStatsComponent.generated.h"

class AAgEnemyCharacter;
class UAbilitySystemComponent;
struct FGameplayEffectSpec;
struct FGameplayEventData;

/** 플레이어의 지금 전투를 기록하고, 끝나면 게임 인스턴스의 전투 통계에 넘겨 저장합니다. */
UCLASS()
class UAgCombatStatsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAgCombatStatsComponent();

	/** 스테이지 결과가 정해진 뒤에 끝난 기록입니다. 그때 기록 중인 전투가 없었으면 nullptr입니다. */
	const FAgCombatStats* GetStageResultStats() const
	{
		return StageResultStats.GetPtrOrNull();
	}

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleCombatantsChanged();
	void HandleStageResultChanged();
	void HandleParry(const FGameplayEventData* Payload);
	void HandlePerfectDodge(const FGameplayEventData* Payload);
	void HandleEffectApplied(UAbilitySystemComponent* Source, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle);

	void EndRecording(EAgCombatResult Result);

	TOptional<FAgCombatStats> Recording;

	double StartTime = 0.0;

	TArray<TWeakObjectPtr<const AAgEnemyCharacter>> Enemies;

	TOptional<FAgCombatStats> StageResultStats;

	FDelegateHandle ParryHandle;
	FDelegateHandle PerfectDodgeHandle;
};
