// Copyright Woogle. All Rights Reserved.

#include "UI/AgStageSelectScreen.h"

#include "Components/ComboBoxKey.h"
#include "Core/AgGameplayTags.h"
#include "Game/AgGameMode.h"
#include "Input/CommonUIInputTypes.h"
#include "UI/AgButton.h"
#include "UI/AgStageOptionWidget.h"

namespace
{
	FName MakeOption(int32 StageIndex)
	{
		return FName(*FString::FromInt(StageIndex));
	}

	int32 GetStageIndex(FName Option)
	{
		return Option.IsNone() ? INDEX_NONE : FCString::Atoi(*Option.ToString());
	}
} // namespace

void UAgStageSelectScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	RegisterUIActionBinding(FBindUIActionArgs(FUIActionTag::ConvertChecked(AgGameplayTags::UI_Action_Escape), false,
		FSimpleDelegate::CreateUObject(this, &ThisClass::DeactivateWidget)));
	StageComboBox->OnGenerateContentWidget.BindDynamic(this, &ThisClass::GenerateOptionWidget);
	StageComboBox->OnGenerateItemWidget.BindDynamic(this, &ThisClass::GenerateOptionWidget);
	StartButton->OnClicked().AddUObject(this, &ThisClass::OpenSelectedStage);
	BackButton->OnClicked().AddUObject(this, &ThisClass::DeactivateWidget);
}

void UAgStageSelectScreen::NativeOnActivated()
{
	Super::NativeOnActivated();

	// CommonUI 스택은 닫은 화면을 다시 쓰므로, 열 때마다 목록을 다시 채우고 첫 스테이지를 고릅니다.
	StageComboBox->ClearOptions();
	const AAgGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgGameMode>();
	if (!GameMode)
	{
		return;
	}
	for (int32 Index = 0; Index < GameMode->GetStages().Num(); ++Index)
	{
		if (GameMode->IsStageAvailable(Index))
		{
			StageComboBox->AddOption(MakeOption(Index));
			if (StageComboBox->GetSelectedOption().IsNone())
			{
				StageComboBox->SetSelectedOption(MakeOption(Index));
			}
		}
	}
}

UWidget* UAgStageSelectScreen::NativeGetDesiredFocusTarget() const
{
	return StageComboBox;
}

TOptional<FUIInputConfig> UAgStageSelectScreen::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

UWidget* UAgStageSelectScreen::GenerateOptionWidget(FName Option)
{
	UAgStageOptionWidget* OptionWidget = CreateWidget<UAgStageOptionWidget>(this, OptionWidgetClass);
	const AAgGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgGameMode>();
	const int32 StageIndex = GetStageIndex(Option);
	if (OptionWidget && GameMode && GameMode->GetStages().IsValidIndex(StageIndex))
	{
		OptionWidget->SetStage(GameMode->GetStages()[StageIndex]);
	}
	return OptionWidget;
}

void UAgStageSelectScreen::OpenSelectedStage()
{
	if (AAgGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgGameMode>())
	{
		GameMode->OpenStage(GetStageIndex(StageComboBox->GetSelectedOption()));
	}
}
