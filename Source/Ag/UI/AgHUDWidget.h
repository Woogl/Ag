// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "GameplayTagContainer.h"
#include "AgHUDWidget.generated.h"

class UAgAbilitySystemComponent;
class UAgInventoryComponent;
class UAgNameplateWidget;
class UAgPauseMenu;
class UAgQuickslotButton;
class UAgSkillButton;
class UAgStatBar;
class UImage;
class UTextBlock;
struct FOnAttributeChangeData;

UCLASS(Abstract, meta = (DisableNativeTick))
class UAgHUDWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	/** 궁극기 컷신과 스테이지 결과 동안 한꺼번에 숨기는 묶음이라, 플레이어 상태·스킬 버튼·보스 상태를 모두 이 안에 둡니다. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> StatusPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgStatBar> PlayerHPBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgStatBar> PlayerMPBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgSkillButton> Skill1Button;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgSkillButton> Skill2Button;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgSkillButton> Skill3Button;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgSkillButton> Skill4Button;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgSkillButton> UltimateButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> UPGauge;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgQuickslotButton> PotionButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> BossStatusPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BossNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgNameplateWidget> BossStatus;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgPauseMenu> PauseMenuClass;

private:
	void OpenPauseMenu();

	void BindPlayer();
	void UnbindPlayer();

	void UpdateStatusPanel();
	void UpdatePlayer();
	void UpdateHPBar();
	void UpdateMPBar();
	void UpdatePotion();
	void UpdateUPGauge();
	void UpdateSkillButtons();
	void UpdateCooldowns();
	void UpdateBoss();

	/** 스킬·궁극기 버튼과 그 버튼의 어빌리티를 발동하는 입력 태그입니다. */
	TArray<TPair<UAgSkillButton*, FGameplayTag>, TInlineAllocator<5>> GetSkillButtons() const;

	void HandlePlayerAttributeChanged(const FOnAttributeChangeData& Data);

	TWeakObjectPtr<UAgAbilitySystemComponent> PlayerASC;

	TWeakObjectPtr<UAgInventoryComponent> PlayerInventory;
};
