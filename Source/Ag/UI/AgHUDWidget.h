// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "GameplayTagContainer.h"
#include "AgHUDWidget.generated.h"

class UAbilitySystemComponent;
class UAgAbilitySystemComponent;
class UAgNameplateWidget;
class UAgPauseMenu;
class UAgSkillSlot;
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

	/**
	 * 궁극기 컷신과 스테이지 결과 동안 한꺼번에 숨기는 묶음이라, 플레이어 상태·스킬 슬롯·보스 상태를 모두 이 안에
	 * 둡니다.
	 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> StatusPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgStatBar> PlayerHPBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgStatBar> PlayerMPBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgSkillSlot> Skill1Slot;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgSkillSlot> Skill2Slot;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgSkillSlot> Skill3Slot;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgSkillSlot> Skill4Slot;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgSkillSlot> UltimateSlot;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> UPGauge;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgSkillSlot> PotionSlot;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PotionCountText;

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
	void UpdateSkillSlots();
	void UpdateCooldowns();
	void UpdateBoss();

	/** 스킬·궁극기·회복병 칸과 그 칸을 발동하는 입력 태그입니다. */
	TArray<TPair<UAgSkillSlot*, FGameplayTag>, TInlineAllocator<6>> GetSkillSlots() const;

	void HandlePlayerAttributeChanged(const FOnAttributeChangeData& Data);

	TWeakObjectPtr<UAgAbilitySystemComponent> PlayerASC;
};
