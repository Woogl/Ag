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
struct FOnAttributeChangeData;

UCLASS(Abstract)
class UAgHUDWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	void HideStatus();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> StatusPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PlayerSPBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PlayerSPTrailBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PlayerHPBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PlayerHPTrailBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PlayerHPText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PlayerMPBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PlayerMPText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> SkillSlot;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> SkillIconImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SkillCostText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> SkillCooldownSweep;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SkillCooldownText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> UltimateSlot;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> UltimateIconImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> UPGauge;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BossNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> BossHPBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> BossHPTrailBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> BossPPBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> BossPPTrailBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ControlsGuideKeys;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ControlsGuideActions;

	UPROPERTY(EditAnywhere, Category = "HUD|Controls Guide")
	TMap<FKey, FText> KeyDisplayNames;

	UPROPERTY(EditAnywhere, Category = "HUD", meta = (ClampMin = 0, ClampMax = 1))
	float DisabledSlotOpacity = 1.f;

private:
	struct FAgTrail
	{
		float Ratio = 1.f;
		float LastBarRatio = -1.f;
		float Wait = 0.f;
		float ShrinkFrom = 1.f;
		float ShrinkElapsed = -1.f;
	};

	void ShowControlsGuide(const AAgPlayerCharacter& Player);

	void UpdatePlayer(const AAgPlayerCharacter& Player, float GameDeltaTime);
	void UpdateBoss(const AAgBossCharacter& BossCharacter, float GameDeltaTime);

	static const UGameplayAbility* FindAbility(const UAbilitySystemComponent& ASC, const FGameplayTag& AbilityTag, FGameplayAbilitySpecHandle& OutHandle);

	static bool CanPayCost(const UAbilitySystemComponent& ASC, const FGameplayTag& AbilityTag);

	void UpdateCooldown(const UAbilitySystemComponent& ASC, const FGameplayTag& AbilityTag, UImage& Sweep, UTextBlock& Text);

	static void SetBarRatio(UProgressBar& Bar, float Value, float MaxValue);
	void SetSlotUsable(UWidget& SlotWidget, bool bUsable) const;

	static void SetValueText(UTextBlock& Text, float Value, float MaxValue, FIntPoint& ShownValues);

	void UpdateTrail(FAgTrail& Trail, UProgressBar& TrailBar, float BarRatio, float DeltaTime, bool bShowDrop = true) const;

	void HandlePlayerSPChanged(const FOnAttributeChangeData& Data);

	TWeakObjectPtr<AAgBossCharacter> Boss;

	TWeakObjectPtr<UAbilitySystemComponent> PlayerASC;
	FDelegateHandle PlayerSPChangedHandle;

	bool bPlayerSPGuardDrop = false;

	bool bStatusHidden = false;

	ESlateVisibility StatusPanelVisibility = ESlateVisibility::SelfHitTestInvisible;

	int32 ShownSkillCost = INDEX_NONE;
	int32 ShownCooldownKey = INDEX_NONE;
	FIntPoint ShownHP = FIntPoint(INDEX_NONE, INDEX_NONE);
	FIntPoint ShownMP = FIntPoint(INDEX_NONE, INDEX_NONE);
	FAgTrail PlayerHPTrail;
	FAgTrail PlayerSPTrail;
	FAgTrail BossHPTrail;
	FAgTrail BossPPTrail;
};
