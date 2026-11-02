// Copyright Woogle. All Rights Reserved.

#include "UI/AgEnemySlotWidget.h"

#include "Components/ComboBoxKey.h"
#include "UI/AgEnemyOptionWidget.h"

namespace
{
	/** 엔진 목록은 None을 빈 항목으로 다뤄 고를 수 없으므로, 빈 자리에 따로 키를 둡니다. */
	const FName EmptyOption(TEXT("Empty"));
} // namespace

void UAgEnemySlotWidget::SetOptions(const TArray<FName>& CharacterDataIds)
{
	const FName Selected = EnemyComboBox->GetSelectedOption();
	EnemyComboBox->ClearOptions();
	EnemyComboBox->AddOption(EmptyOption);
	for (FName Id : CharacterDataIds)
	{
		EnemyComboBox->AddOption(Id);
	}
	EnemyComboBox->SetSelectedOption(CharacterDataIds.Contains(Selected) ? Selected : EmptyOption);
}

FName UAgEnemySlotWidget::GetSelectedEnemy() const
{
	const FName Selected = EnemyComboBox->GetSelectedOption();
	return Selected == EmptyOption ? NAME_None : Selected;
}

void UAgEnemySlotWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SetDesiredFocusWidget(EnemyComboBox);
	EnemyComboBox->OnGenerateContentWidget.BindDynamic(this, &ThisClass::GenerateOptionWidget);
	EnemyComboBox->OnGenerateItemWidget.BindDynamic(this, &ThisClass::GenerateOptionWidget);
	EnemyComboBox->OnSelectionChanged.AddDynamic(this, &ThisClass::HandleSelectionChanged);
}

UWidget* UAgEnemySlotWidget::GenerateOptionWidget(FName Option)
{
	UAgEnemyOptionWidget* OptionWidget = CreateWidget<UAgEnemyOptionWidget>(this, OptionWidgetClass);
	if (OptionWidget)
	{
		OptionWidget->SetEnemy(Option == EmptyOption ? NAME_None : Option);
	}
	return OptionWidget;
}

void UAgEnemySlotWidget::HandleSelectionChanged(FName SelectedItem, ESelectInfo::Type SelectionType)
{
	// 화면이 목록을 다시 채우며 선택을 되돌려 놓는 변경까지 알리면 갱신이 되풀이됩니다.
	if (SelectionType != ESelectInfo::Direct)
	{
		OnSelectedEnemyChanged.Broadcast();
	}
}
