// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/AgMenuScreen.h"
#include "AgEnemySelectScreen.generated.h"

class AAgCharacterBase;
class UAgButton;
class UAgDropdown;
class UDynamicEntryBox;

UCLASS(Abstract, meta = (DisableNativeTick))
class UAgEnemySelectScreen : public UAgMenuScreen
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY(EditAnywhere, Category = "UI", meta = (ClampMin = 0, ClampMax = 1))
	float LockedButtonOpacity = 1.f;

	UPROPERTY(EditAnywhere, Category = "UI")
	FText EmptyText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UDynamicEntryBox> SlotList;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> StartButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> ReturnToTitleButton;

private:
	TArray<TSubclassOf<AAgCharacterBase>> GetLineup() const;
	void RefreshSlots();
	void StartLineup();

	UPROPERTY()
	TArray<TObjectPtr<UAgDropdown>> SlotDropdowns;
};
