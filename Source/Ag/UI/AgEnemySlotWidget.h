// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Types/SlateEnums.h"
#include "AgEnemySlotWidget.generated.h"

class UAgEnemyOptionWidget;
class UComboBoxKey;

UCLASS(Abstract, meta = (DisableNativeTick))
class UAgEnemySlotWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	/** Empty와 그 적들로 목록을 다시 채우고, 고른 적이 목록에 있으면 그대로 둡니다. */
	void SetOptions(const TArray<FName>& CharacterDataIds);

	/** 비어 있으면 None입니다. */
	FName GetSelectedEnemy() const;

	/** 사람이 목록에서 고른 때만 알립니다. */
	FSimpleMulticastDelegate OnSelectedEnemyChanged;

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgEnemyOptionWidget> OptionWidgetClass;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxKey> EnemyComboBox;

private:
	UFUNCTION()
	UWidget* GenerateOptionWidget(FName Option);

	UFUNCTION()
	void HandleSelectionChanged(FName SelectedItem, ESelectInfo::Type SelectionType);
};
