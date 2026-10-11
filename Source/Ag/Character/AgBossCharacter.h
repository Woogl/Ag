// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/AgCharacterBase.h"
#include "GameplayTagContainer.h"
#include "AgBossCharacter.generated.h"

class UAgDamageFloaterWidget;
class UWidgetComponent;
struct FGameplayAbilitySpec;

/**
 * 보스 커스터(보스 사양). AI의 대상을 향해 스스로 돌고, AI가 고른 패턴을 실행합니다.
 * 월드에 표시하는 HUD(전투 HUD '락온 마커', '처형 안내', '대미지 숫자')를 들고 있으며, 위젯은 BP_AgBoss에서 정합니다.
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

	/** 대미지 숫자: 플레이어 공격으로 보스가 잃은 HP를 Location에 띄웁니다. */
	void ShowDamageFloater(int32 Amount, const FVector& Location, bool bLarge) const;

	/** 보스 AI의 현재 대상(구현 계획서 'AI 대상'). AI가 없거나 대상이 아직 없으면 null입니다. */
	AAgCharacterBase* GetTarget() const;

	/**
	 * 보스 사양 '회전': 회전하지 않는 동안 참입니다. 판정 구간과 그 앞의 회전 정지 구간, 회전하지 않는 상태(피격 반응, 그로기,
	 * 처형 피격, 회전 고정, 비전투)가 해당합니다.
	 */
	virtual bool IsFacingFixed() const override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GrantAbilities() override;
	virtual float GetMoveSpeedMultiplier() const override;

private:
	/** 회전: 바라보는 방향이 고정되지 않은 동안 회전 속도로 AI의 대상을 향해 돕니다. */
	void UpdateRotation(float DeltaSeconds);

	/**
	 * True from the rotation stop lead before an attack window of a playing montage until the window ends.
	 * A6's sword wave release counts as an attack window of no length.
	 */
	bool IsInRotationStopWindow() const;

	/** Attaches the lock-on marker and the execution prompt to their bones in the character data. */
	void AttachWorldWidgets();

	/** Shows or hides the lock-on marker and the execution prompt (전투 HUD '상황별 표시 규칙'). */
	void UpdateWorldWidgets();

	/** 락온 마커: at the lock-on point while the player is locked on to this boss. */
	UPROPERTY(VisibleAnywhere, Category = "UI")
	TObjectPtr<UWidgetComponent> LockOnMarker;

	/** 처형 안내: above the head while the player can execute this boss. */
	UPROPERTY(VisibleAnywhere, Category = "UI")
	TObjectPtr<UWidgetComponent> ExecutionPrompt;

	/** 대미지 숫자 위젯(WBP_DamageFloater). */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgDamageFloaterWidget> DamageFloaterWidget;

	int32 Phase = 1;

	/** World time the groggy time started counting, or negative when it isn't counting. */
	double GroggyStartTime = -1.0;
};
