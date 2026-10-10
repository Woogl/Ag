// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "AgHUDWidget.generated.h"

class AAgBossCharacter;
class AAgPlayerCharacter;
class UAbilitySystemComponent;
class UImage;
class UProgressBar;
class UTextBlock;
struct FGameplayEventData;
struct FGameplayTag;

/**
 * Combat HUD (전투 HUD). Parent of WBP_HUD: the layout lives in the widget, this class only feeds it values.
 * The parts shown in the world (lock-on marker, execution prompt, damage numbers) are on the boss.
 * While it is active, input goes to the game only and the cursor is hidden (게임 플로우 'Boss Stage').
 */
UCLASS(Abstract)
class UAgHUDWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** 플레이어 상태: SP 바, shown only while SP is below MaxSP. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> PlayerSPBar;

	/** 플레이어 상태: HP 바 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> PlayerHPBar;

	/** 플레이어 상태: the HP bar's 감소 잔상, behind it. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> PlayerHPTrailBar;

	/** 플레이어 상태: HP 수치 (현재값 / 최대값) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PlayerHPText;

	/** 플레이어 상태: MP 바 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> PlayerMPBar;

	/** 플레이어 상태: MP 수치 (현재값 / 최대값) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PlayerMPText;

	/** 스킬 슬롯, dimmed while the skill can't be used. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> SkillSlot;

	/** 스킬 슬롯: the skill's MP cost. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SkillCostText;

	/** 궁극기 슬롯, dimmed while the ultimate can't be used. The UP gauge is outside it and never dims. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> UltimateSlot;

	/** 궁극기 슬롯: UP 게이지. Its material fills the slot border by the UP ratio. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> UltimateGauge;

	/** 보스 상태: 보스 이름 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BossNameText;

	/** 보스 상태: HP 바 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> BossHPBar;

	/** 보스 상태: the HP bar's 감소 잔상, behind it. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> BossHPTrailBar;

	/** 보스 상태: PP 바. Its fill color in the designer is the normal color. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> BossPPBar;

	/** 화면 효과: 패리 성공, a faint flash over the whole screen. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> ParryFlash;

	/** 화면 효과: 극한 회피, a flash around the screen edges. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> PerfectDodgeFlash;

	/** 보스 상태: the PP bar color during groggy, while the bar refills over the groggy time. */
	UPROPERTY(EditAnywhere, Category = "HUD")
	FLinearColor GroggyPPColor = FLinearColor::White;

	/** 스킬·궁극기 슬롯: opacity of a slot that can't be used. */
	UPROPERTY(EditAnywhere, Category = "HUD", meta = (ClampMin = 0, ClampMax = 1))
	float DisabledSlotOpacity = 1.f;

	/** 감소 잔상: the trail waits this long after HP drops, ... */
	UPROPERTY(EditAnywhere, Category = "HUD|HP Trail", meta = (Units = "s", ClampMin = 0))
	float TrailDelay = 0.f;

	/** 감소 잔상: ... then shrinks to the current HP over this time. */
	UPROPERTY(EditAnywhere, Category = "HUD|HP Trail", meta = (Units = "s", ClampMin = 0))
	float TrailShrinkTime = 0.f;

	/** 화면 효과 '패리 성공': flash time; it fades out from its strongest. */
	UPROPERTY(EditAnywhere, Category = "HUD|Screen Effects", meta = (Units = "s", ClampMin = 0))
	float ParryFlashTime = 0.f;

	/** 화면 효과 '패리 성공': opacity at the start, faint enough not to hide the screen. */
	UPROPERTY(EditAnywhere, Category = "HUD|Screen Effects", meta = (ClampMin = 0, ClampMax = 1))
	float ParryFlashOpacity = 0.f;

	/** 화면 효과 '극한 회피': flash time; it fades out from its strongest. */
	UPROPERTY(EditAnywhere, Category = "HUD|Screen Effects", meta = (Units = "s", ClampMin = 0))
	float PerfectDodgeFlashTime = 0.f;

	/** 화면 효과 '극한 회피': opacity at the start. */
	UPROPERTY(EditAnywhere, Category = "HUD|Screen Effects", meta = (ClampMin = 0, ClampMax = 1))
	float PerfectDodgeFlashOpacity = 0.f;

private:
	/** 감소 잔상 of one HP bar. */
	struct FHPTrail
	{
		/** The trail's current length (0 to 1). */
		float Ratio = 1.f;
		/** The HP ratio last frame, negative before the first. */
		float LastHPRatio = -1.f;
		/** Time left before the trail shrinks. */
		float Wait = 0.f;
		/** Length when the shrink started. */
		float ShrinkFrom = 1.f;
		/** Time into the shrink, negative while not shrinking. */
		float ShrinkElapsed = -1.f;
	};

	void UpdatePlayer(const AAgPlayerCharacter& Player, float GameDeltaTime);
	void UpdateBoss(const AAgBossCharacter& BossCharacter, float GameDeltaTime);

	/** Whether the ability with AbilityTag passes its own cost and cooldown checks (사용할 수 없는 슬롯: MP·UP 부족, 쿨다운 중). */
	static bool CanUseAbility(const UAbilitySystemComponent& ASC, const FGameplayTag& AbilityTag);

	static void SetBarRatio(UProgressBar* Bar, float Value, float MaxValue);
	void SetSlotUsable(UWidget* SlotWidget, bool bUsable) const;

	/** Sets '현재값 / 최대값' when the whole numbers change. */
	static void SetValueText(UTextBlock* Text, float Value, float MaxValue, FIntPoint& ShownValues);

	/** HP dropped: the trail stays (even mid-shrink) and waits again; then it shrinks to the current HP. */
	void UpdateTrail(FHPTrail& Trail, UProgressBar* TrailBar, float HPRatio, float DeltaTime) const;

	/** Fades a screen effect out over its time; a negative age is idle. */
	static void UpdateFlash(UWidget* Flash, float& Age, float Duration, float StartOpacity, float DeltaTime);

	/** Listens to the player's parry and 극한 회피 for the screen effects. */
	void BindPlayerEvents(UAbilitySystemComponent* ASC);
	void UnbindPlayerEvents();
	void HandleParry(const FGameplayEventData* Payload);
	void HandlePerfectDodge(const FGameplayEventData* Payload);

	TWeakObjectPtr<AAgBossCharacter> Boss;
	TWeakObjectPtr<UAbilitySystemComponent> PlayerASC;
	FDelegateHandle ParryHandle;
	FDelegateHandle PerfectDodgeHandle;

	/** The PP bar's fill color from the designer. */
	FLinearColor PPColor = FLinearColor::White;

	int32 ShownSkillCost = INDEX_NONE;
	FIntPoint ShownHP = FIntPoint(INDEX_NONE, INDEX_NONE);
	FIntPoint ShownMP = FIntPoint(INDEX_NONE, INDEX_NONE);
	FHPTrail PlayerTrail;
	FHPTrail BossTrail;
	float ParryFlashAge = -1.f;
	float PerfectDodgeFlashAge = -1.f;
};
