// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/AgCharacterBase.h"
#include "GameplayTagContainer.h"
#include "AgBossCharacter.generated.h"

class UAgDamageNumberWidget;
class UWidgetComponent;
struct FGameplayAbilitySpec;

/**
 * The boss 커스터 (보스 사양). Turns toward the player on its own and runs the patterns its AI picks.
 * Carries the HUD shown on it in the world (전투 HUD '락온 마커', '처형 안내', '대미지 숫자'); the widgets are set in BP_AgBoss.
 */
UCLASS(Abstract)
class AAgBossCharacter : public AAgCharacterBase
{
	GENERATED_BODY()

public:
	AAgBossCharacter();

	/** The granted spec of a pattern, or null. */
	FGameplayAbilitySpec* FindPatternSpec(const FGameplayTag& Pattern) const;

	/** 페이즈: 1, or 2 after the phase transition ends. */
	int32 GetPhase() const { return Phase; }

	/** 페이즈 전환이 끝남: the phase 2 changes apply from now on. */
	void EnterPhase2();

	/** 2페이즈 변화 '패턴 진행 속도': 1 in phase 1. Scales the pattern motions and the movement inside patterns. */
	float GetPatternSpeed() const;

	/** 그로기: the groggy time starts counting (on landing when broken in the air) or groggy ends. */
	void SetGroggyTimerRunning(bool bRunning);

	/** Game time since the groggy time started counting, or a negative value when it isn't counting. */
	float GetGroggyElapsedTime() const;

	/** 대미지 숫자: shows the HP this boss lost from a player attack at Location. */
	void ShowDamageNumber(int32 Amount, const FVector& Location, bool bLarge) const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GrantAbilities() override;
	virtual float GetMoveSpeedMultiplier() const override;

private:
	/** 회전: turns toward the player at the rotation speed, except during the rotation stop windows. */
	void UpdateRotation(float DeltaSeconds);

	/** True from the rotation stop lead before an attack window of a playing montage until the window ends. */
	bool IsInRotationStopWindow() const;

	/** Shows or hides the lock-on marker and the execution prompt (전투 HUD '상황별 표시 규칙'). */
	void UpdateWorldWidgets();

	/** 락온 마커: at the lock-on point while the player is locked on to this boss. */
	UPROPERTY(VisibleAnywhere, Category = "UI")
	TObjectPtr<UWidgetComponent> LockOnMarker;

	/** 처형 안내: above the head while the player can execute this boss. */
	UPROPERTY(VisibleAnywhere, Category = "UI")
	TObjectPtr<UWidgetComponent> ExecutionPrompt;

	/** 대미지 숫자 widget (WBP_DamageNumber). */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgDamageNumberWidget> DamageNumberWidget;

	int32 Phase = 1;

	/** World time the groggy time started counting, or negative when it isn't counting. */
	double GroggyStartTime = -1.0;
};
