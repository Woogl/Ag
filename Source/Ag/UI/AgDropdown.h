// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Types/SlateEnums.h"
#include "AgDropdown.generated.h"

class UAgDropdownOptionWidget;
class UComboBoxKey;

struct FAgDropdownOption
{
	FName Key;
	FText Label;
};

UCLASS(Abstract, meta = (DisableNativeTick))
class UAgDropdown : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	/** 목록을 다시 채우고 SelectedKey를 고릅니다. 목록에 없으면 첫 항목을 고릅니다. */
	void SetOptions(const TArray<FAgDropdownOption>& Options, FName SelectedKey = NAME_None);

	/** 목록이 비어 있으면 None입니다. */
	FName GetSelectedOption() const;

	/** 사람이 목록에서 고른 때만 알립니다. */
	FSimpleMulticastDelegate OnSelectionChanged;

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgDropdownOptionWidget> OptionWidgetClass;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxKey> ComboBox;

private:
	UFUNCTION()
	UWidget* GenerateOptionWidget(FName Option);

	UFUNCTION()
	void HandleSelectionChanged(FName SelectedItem, ESelectInfo::Type SelectionType);

	TMap<FName, FText> Labels;
};
