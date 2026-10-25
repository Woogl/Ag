// Copyright Woogle. All Rights Reserved.

#include "UI/AgPauseMenu.h"

#include "Core/AgGameplayTags.h"
#include "Game/AgGameMode.h"
#include "GameFramework/PlayerController.h"
#include "Input/CommonUIInputTypes.h"
#include "UI/AgButton.h"

void UAgPauseMenu::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	RegisterUIActionBinding(FBindUIActionArgs(FUIActionTag::ConvertChecked(AgGameplayTags::UI_Action_Escape), false,
		FSimpleDelegate::CreateUObject(this, &ThisClass::DeactivateWidget)));
	ResumeButton->OnClicked().AddUObject(this, &ThisClass::DeactivateWidget);

	if (AAgGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgGameMode>())
	{
		RestartButton->OnClicked().AddUObject(GameMode, &AAgGameMode::RestartStage);
		ReturnToTitleButton->OnClicked().AddUObject(GameMode, &AAgGameMode::OpenFrontEnd);
		QuitButton->OnClicked().AddUObject(GameMode, &AAgGameMode::QuitGame);
	}
}

void UAgPauseMenu::NativeOnActivated()
{
	Super::NativeOnActivated();

	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		PlayerController->SetPause(true);
	}
}

void UAgPauseMenu::NativeOnDeactivated()
{
	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		PlayerController->SetPause(false);
	}

	Super::NativeOnDeactivated();
}

UWidget* UAgPauseMenu::NativeGetDesiredFocusTarget() const
{
	return ResumeButton;
}

TOptional<FUIInputConfig> UAgPauseMenu::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}
