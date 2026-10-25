// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "AgHUDWidget.generated.h"

class AAgBossCharacter;
class UAbilitySystemComponent;
class UAgPauseMenu;
class UAgStatBar;
class UGameplayAbility;
class UImage;
class UTextBlock;
struct FGameplayAbilitySpecHandle;
struct FGameplayAttribute;
struct FGameplayTag;
struct FOnAttributeChangeData;

UCLASS(Abstract, meta = (DisableNativeTick))
class UAgHUDWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	void HideStatus();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> StatusPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgStatBar> PlayerSPBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgStatBar> PlayerHPBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgStatBar> PlayerMPBar;

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
	TObjectPtr<UWidget> BossStatusPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BossNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgStatBar> BossHPBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgStatBar> BossPPBar;

	UPROPERTY(EditAnywhere, Category = "HUD", meta = (ClampMin = 0, ClampMax = 1))
	float DisabledSlotOpacity = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgPauseMenu> PauseMenuClass;

private:
	void OpenPauseMenu();
	void HandleBossChanged();

	void BindPlayer(UAbilitySystemComponent& ASC);
	void BindBoss(UAbilitySystemComponent& ASC);

	void UpdateStatusPanel();
	void UpdatePlayer();
	void UpdateSkillSlots();
	void UpdateSkillCooldown();
	void UpdateBoss();

	static void SetBarValue(UAgStatBar& Bar, const UAbilitySystemComponent& ASC, const FGameplayAttribute& Value, const FGameplayAttribute& MaxValue, bool bShowDrop = true);

	static const UGameplayAbility* FindAbility(const UAbilitySystemComponent& ASC, const FGameplayTag& AbilityTag, FGameplayAbilitySpecHandle& OutHandle);

	static bool CanPayCost(const UAbilitySystemComponent& ASC, const FGameplayTag& AbilityTag);

	void SetSlotUsable(UWidget& SlotWidget, bool bUsable) const;

	void HandlePlayerAttributeChanged(const FOnAttributeChangeData& Data);

	TWeakObjectPtr<AAgBossCharacter> Boss;

	TWeakObjectPtr<UAbilitySystemComponent> PlayerASC;

	bool bStatusHidden = false;

	ESlateVisibility StatusPanelVisibility = ESlateVisibility::SelfHitTestInvisible;

	int32 ShownSkillCost = INDEX_NONE;
	int32 ShownCooldownKey = INDEX_NONE;

	FTimerHandle CooldownTimer;
	FTimerHandle BossTimer;
};
