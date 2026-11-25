// Copyright Woogle. All Rights Reserved.

#include "UI/AgDisplaySettings.h"

#include "Kismet/KismetSystemLibrary.h"
#include "UI/AgDropdown.h"

namespace AgDisplaySettingsPrivate
{
	/**
	 * 그 창 모드에서 고를 수 있는 해상도를 넓이가 큰 것부터 돌려줍니다. Fullscreen은 후보 가운데 모니터가 지원하는 것과 모니터
	 * 해상도, Windowed는 후보 가운데 모니터보다 작은 것(없으면 모니터 해상도), 그 밖의 창 모드는 모니터 해상도입니다.
	 */
	TArray<FIntPoint> GetResolutions(const UGameUserSettings& Settings, EWindowMode::Type WindowMode, const TArray<FIntPoint>& Candidates)
	{
		const FIntPoint Desktop = Settings.GetDesktopResolution();
		TArray<FIntPoint> Resolutions;
		if (WindowMode == EWindowMode::Fullscreen)
		{
			TArray<FIntPoint> Supported;
			UKismetSystemLibrary::GetSupportedFullscreenResolutions(Supported);
			Resolutions = Candidates.FilterByPredicate([&Supported](const FIntPoint& Resolution)
			{ return Supported.Contains(Resolution); });
			Resolutions.AddUnique(Desktop);
		}
		else if (WindowMode == EWindowMode::Windowed)
		{
			Resolutions = Candidates.FilterByPredicate([&Desktop](const FIntPoint& Resolution)
			{ return Resolution.X < Desktop.X && Resolution.Y < Desktop.Y; });
		}
		if (Resolutions.IsEmpty())
		{
			Resolutions.Add(Desktop);
		}
		Resolutions.Sort([](const FIntPoint& A, const FIntPoint& B)
		{ return A.X * A.Y != B.X * B.Y ? A.X * A.Y > B.X * B.Y : A.X > B.X; });
		return Resolutions;
	}

	FName GetResolutionKey(const FIntPoint& Resolution)
	{
		return FName(FString::Printf(TEXT("%dx%d"), Resolution.X, Resolution.Y));
	}
}

void UAgDisplaySettings::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	WindowModeDropdown->OnSelectionChanged.AddUObject(this, &ThisClass::ApplyWindowMode);
	ResolutionDropdown->OnSelectionChanged.AddUObject(this, &ThisClass::ApplyResolution);
}

void UAgDisplaySettings::NativeConstruct()
{
	Super::NativeConstruct();

	Refresh();
}

void UAgDisplaySettings::Refresh()
{
	const UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	const EWindowMode::Type WindowMode = Settings->GetFullscreenMode();
	const UEnum* WindowModes = StaticEnum<EWindowMode::Type>();

	TArray<FAgDropdownOption> WindowModeOptions;
	for (int32 Mode = 0; Mode < EWindowMode::NumWindowModes; ++Mode)
	{
		if (const FText* Name = WindowModeNames.Find(EWindowMode::ConvertIntToWindowMode(Mode)))
		{
			WindowModeOptions.Add({WindowModes->GetNameByValue(Mode), *Name});
		}
	}
	WindowModeDropdown->SetOptions(WindowModeOptions, WindowModes->GetNameByValue(WindowMode));

	Resolutions = AgDisplaySettingsPrivate::GetResolutions(*Settings, WindowMode, StandardResolutions);
	TArray<FAgDropdownOption> ResolutionOptions;
	for (const FIntPoint& Resolution : Resolutions)
	{
		const FString Label = FString::Printf(TEXT("%d × %d"), Resolution.X, Resolution.Y);
		ResolutionOptions.Add({AgDisplaySettingsPrivate::GetResolutionKey(Resolution), FText::AsCultureInvariant(Label)});
	}
	ResolutionDropdown->SetOptions(ResolutionOptions, AgDisplaySettingsPrivate::GetResolutionKey(Settings->GetScreenResolution()));
}

void UAgDisplaySettings::ApplyWindowMode()
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	const int64 Mode = StaticEnum<EWindowMode::Type>()->GetValueByName(WindowModeDropdown->GetSelectedOption());
	const EWindowMode::Type WindowMode = EWindowMode::ConvertIntToWindowMode(static_cast<int32>(Mode));
	Settings->SetFullscreenMode(WindowMode);

	const TArray<FIntPoint> ModeResolutions = AgDisplaySettingsPrivate::GetResolutions(*Settings, WindowMode, StandardResolutions);
	if (!ModeResolutions.Contains(Settings->GetScreenResolution()))
	{
		Settings->SetScreenResolution(ModeResolutions[0]);
	}
	Settings->ApplyResolutionSettings(false);
	Settings->SaveSettings();
	Refresh();
}

void UAgDisplaySettings::ApplyResolution()
{
	const FName Key = ResolutionDropdown->GetSelectedOption();
	for (const FIntPoint& Resolution : Resolutions)
	{
		if (AgDisplaySettingsPrivate::GetResolutionKey(Resolution) == Key)
		{
			UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
			Settings->SetScreenResolution(Resolution);
			Settings->ApplyResolutionSettings(false);
			Settings->SaveSettings();
			return;
		}
	}
}
