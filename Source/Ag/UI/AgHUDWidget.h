// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "InputCoreTypes.h"
#include "AgHUDWidget.generated.h"

class AAgBossCharacter;
class AAgPlayerCharacter;
class UAbilitySystemComponent;
class UGameplayAbility;
class UImage;
class UProgressBar;
class UTextBlock;
struct FGameplayAbilitySpecHandle;
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

public:
	/** 상황별 표시 규칙 '사망 연출': 플레이어 상태, 슬롯, 보스 상태, 조작 안내를 숨깁니다. */
	void HideStatus();

protected:
	virtual void NativeConstruct() override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** 사망 연출에 숨기는 요소(플레이어 상태, 슬롯, 보스 상태, 조작 안내)를 모두 담습니다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> StatusPanel;

	/** 플레이어 상태: SP 바 */
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

	/** 스킬 슬롯, dimmed while the skill's MP cost can't be paid. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> SkillSlot;

	/** 스킬 슬롯: 스킬 아이콘. A texture or material goes into its brush in the designer; without one it stays hidden. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> SkillIconImage;

	/** 스킬 슬롯: the skill's MP cost. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SkillCostText;

	/** 스킬 슬롯: 쿨다운 표시의 원형 표시. Its material covers the slot by the remaining share of the cooldown. Outside SkillSlot, so it never dims. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> SkillCooldownSweep;

	/** 스킬 슬롯: 쿨다운 표시의 숫자 (remaining cooldown). Outside SkillSlot, so it never dims. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SkillCooldownText;

	/** 궁극기 슬롯, dimmed while UP isn't full. The UP gauge is outside it and never dims. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> UltimateSlot;

	/** 궁극기 슬롯: 궁극기 아이콘, set up like SkillIconImage. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> UltimateIconImage;

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

	/** 보스 상태: PP 바 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> BossPPBar;

	/** 조작 안내: 키 열. 한 줄에 행동 하나의 키를 표시합니다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ControlsGuideKeys;

	/** 조작 안내: 행동 이름 열. 키 열과 같은 줄 순서입니다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ControlsGuideActions;

	/** UI 스타일 '입력 키 표시': 엔진 키 이름과 다르게 표시하는 키(마우스 입력 등)의 표시 이름 */
	UPROPERTY(EditAnywhere, Category = "HUD|Controls Guide")
	TMap<FKey, FText> KeyDisplayNames;

	/** 스킬·궁극기 슬롯: opacity of a slot that can't be used. */
	UPROPERTY(EditAnywhere, Category = "HUD", meta = (ClampMin = 0, ClampMax = 1))
	float DisabledSlotOpacity = 1.f;

	/** 감소 잔상: the trail waits this long after HP drops, ... */
	UPROPERTY(EditAnywhere, Category = "HUD|HP Trail", meta = (Units = "s", ClampMin = 0))
	float TrailDelay = 0.f;

	/** 감소 잔상: ... then shrinks to the current HP over this time. */
	UPROPERTY(EditAnywhere, Category = "HUD|HP Trail", meta = (Units = "s", ClampMin = 0))
	float TrailShrinkTime = 0.f;

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

	/** 조작 안내: 플레이어 입력 매핑의 행동마다 키와 행동 이름(입력 액션의 설명)을 매핑 순서대로 한 줄씩 채웁니다. */
	void ShowControlsGuide(const AAgPlayerCharacter& Player);

	void UpdatePlayer(const AAgPlayerCharacter& Player, float GameDeltaTime);
	void UpdateBoss(const AAgBossCharacter& BossCharacter, float GameDeltaTime);

	/** The granted ability with AbilityTag and its spec handle, or null. */
	static const UGameplayAbility* FindAbility(const UAbilitySystemComponent& ASC, const FGameplayTag& AbilityTag, FGameplayAbilitySpecHandle& OutHandle);

	/** Whether the ability passes its own cost check (흐린 슬롯: MP·UP 부족). */
	static bool CanPayCost(const UAbilitySystemComponent& ASC, const FGameplayTag& AbilityTag);

	/** 쿨다운 표시: the remaining share and time of the ability's cooldown, hidden while it is ready. */
	void UpdateCooldown(const UAbilitySystemComponent& ASC, const FGameplayTag& AbilityTag, UImage* Sweep, UTextBlock* Text);

	static void SetBarRatio(UProgressBar* Bar, float Value, float MaxValue);
	void SetSlotUsable(UWidget* SlotWidget, bool bUsable) const;

	/** Sets '현재값 / 최대값' when the whole numbers change. */
	static void SetValueText(UTextBlock* Text, float Value, float MaxValue, FIntPoint& ShownValues);

	/** HP dropped: the trail stays (even mid-shrink) and waits again; then it shrinks to the current HP. */
	void UpdateTrail(FHPTrail& Trail, UProgressBar* TrailBar, float HPRatio, float DeltaTime) const;

	TWeakObjectPtr<AAgBossCharacter> Boss;

	int32 ShownSkillCost = INDEX_NONE;
	int32 ShownCooldownKey = INDEX_NONE;
	FIntPoint ShownHP = FIntPoint(INDEX_NONE, INDEX_NONE);
	FIntPoint ShownMP = FIntPoint(INDEX_NONE, INDEX_NONE);
	FHPTrail PlayerTrail;
	FHPTrail BossTrail;
};
