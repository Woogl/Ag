// Copyright Woogle. All Rights Reserved.

#include "UI/AgDropdown.h"

#include "Components/ComboBoxKey.h"
#include "UI/AgDropdownOptionWidget.h"

void UAgDropdown::SetOptions(const TArray<FAgDropdownOption>& Options, FName SelectedKey)
{
	ComboBox->ClearOptions();
	Labels.Reset();
	for (const FAgDropdownOption& Option : Options)
	{
		Labels.Add(Option.Key, Option.Label);
		ComboBox->AddOption(Option.Key);
	}
	if (!Options.IsEmpty())
	{
		ComboBox->SetSelectedOption(Labels.Contains(SelectedKey) ? SelectedKey : Options[0].Key);
	}
}

FName UAgDropdown::GetSelectedOption() const
{
	return ComboBox->GetSelectedOption();
}

void UAgDropdown::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SetDesiredFocusWidget(ComboBox);
	ComboBox->OnGenerateContentWidget.BindDynamic(this, &ThisClass::GenerateSelectedWidget);
	ComboBox->OnGenerateItemWidget.BindDynamic(this, &ThisClass::GenerateOptionWidget);
	ComboBox->OnSelectionChanged.AddDynamic(this, &ThisClass::HandleSelectionChanged);
}

UWidget* UAgDropdown::GenerateSelectedWidget(FName Option)
{
	return CreateOptionWidget(SelectedWidgetClass ? SelectedWidgetClass : OptionWidgetClass, Option);
}

UWidget* UAgDropdown::GenerateOptionWidget(FName Option)
{
	return CreateOptionWidget(OptionWidgetClass, Option);
}

UAgDropdownOptionWidget* UAgDropdown::CreateOptionWidget(TSubclassOf<UAgDropdownOptionWidget> WidgetClass, FName Option)
{
	UAgDropdownOptionWidget* OptionWidget = CreateWidget<UAgDropdownOptionWidget>(this, WidgetClass);
	if (OptionWidget)
	{
		OptionWidget->SetOption(Option, Labels.FindRef(Option));
	}
	return OptionWidget;
}

void UAgDropdown::HandleSelectionChanged(FName SelectedItem, ESelectInfo::Type SelectionType)
{
	// 화면이 목록을 다시 채우며 선택을 되돌려 놓는 변경까지 알리면 갱신이 되풀이됩니다.
	if (SelectionType != ESelectInfo::Direct)
	{
		OnSelectionChanged.Broadcast();
	}
}
