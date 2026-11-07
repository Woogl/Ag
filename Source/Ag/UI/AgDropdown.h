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
	void SetOptions(const TArray<FAgDropdownOption>& Options, FName SelectedKey = NAME_None);

	FName GetSelectedOption() const;

	FSimpleMulticastDelegate OnSelectionChanged;

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgDropdownOptionWidget> OptionWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgDropdownOptionWidget> SelectedWidgetClass;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxKey> ComboBox;

private:
	UFUNCTION()
	UWidget* GenerateSelectedWidget(FName Option);

	UFUNCTION()
	UWidget* GenerateOptionWidget(FName Option);

	UAgDropdownOptionWidget* CreateOptionWidget(TSubclassOf<UAgDropdownOptionWidget> WidgetClass, FName Option);

	UFUNCTION()
	void HandleSelectionChanged(FName SelectedItem, ESelectInfo::Type SelectionType);

	TMap<FName, FText> Labels;
};
