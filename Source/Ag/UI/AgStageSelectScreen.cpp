// Copyright Woogle. All Rights Reserved.

#include "UI/AgStageSelectScreen.h"

#include "Core/AgGameplayTags.h"
#include "Game/AgGameMode.h"
#include "Input/CommonUIInputTypes.h"
#include "UI/AgButton.h"
#include "UI/AgDropdown.h"

namespace AgStageSelectScreenPrivate
{
	FName MakeOption(int32 StageIndex)
	{
		return FName(*FString::FromInt(StageIndex));
	}

	int32 GetStageIndex(FName Option)
	{
		return Option.IsNone() ? INDEX_NONE : FCString::Atoi(*Option.ToString());
	}
} // namespace AgStageSelectScreenPrivate

void UAgStageSelectScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	RegisterUIActionBinding(FBindUIActionArgs(FUIActionTag::ConvertChecked(AgGameplayTags::UI_Action_Escape), false,
		FSimpleDelegate::CreateUObject(this, &ThisClass::DeactivateWidget)));
	StartButton->OnClicked().AddUObject(this, &ThisClass::OpenSelectedStage);
	BackButton->OnClicked().AddUObject(this, &ThisClass::DeactivateWidget);
}

void UAgStageSelectScreen::NativeOnActivated()
{
	Super::NativeOnActivated();

	// CommonUI 스택은 닫은 화면을 다시 쓰므로, 열 때마다 목록을 다시 채우고 첫 스테이지를 고릅니다.
	TArray<FAgDropdownOption> Options;
	if (const AAgGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgGameMode>())
	{
		for (int32 Index = 0; Index < GameMode->GetStages().Num(); ++Index)
		{
			if (GameMode->IsStageAvailable(Index))
			{
				Options.Add({AgStageSelectScreenPrivate::MakeOption(Index), GameMode->GetStages()[Index].DisplayName});
			}
		}
	}
	StageDropdown->SetOptions(Options);
}

UWidget* UAgStageSelectScreen::NativeGetDesiredFocusTarget() const
{
	return StageDropdown->GetDesiredFocusWidget();
}

TOptional<FUIInputConfig> UAgStageSelectScreen::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

void UAgStageSelectScreen::OpenSelectedStage()
{
	if (AAgGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgGameMode>())
	{
		GameMode->OpenStage(AgStageSelectScreenPrivate::GetStageIndex(StageDropdown->GetSelectedOption()));
	}
}
