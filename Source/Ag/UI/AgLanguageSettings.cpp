// Copyright Woogle. All Rights Reserved.

#include "UI/AgLanguageSettings.h"

#include "Engine/GameInstance.h"
#include "Game/AgLanguageSubsystem.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "UI/AgDropdown.h"

void UAgLanguageSettings::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	LanguageDropdown->OnSelectionChanged.AddUObject(this, &ThisClass::ApplyLanguage);
}

void UAgLanguageSettings::NativeConstruct()
{
	Super::NativeConstruct();

	// 언어 이름은 지금 언어와 관계없이 그 언어로 적으므로 번역하지 않는 엔진 문화권 이름을 씁니다.
	TArray<FAgDropdownOption> Options;
	for (const FString& Language : UAgLanguageSubsystem::GetLanguages())
	{
		const FCulturePtr Culture = FInternationalization::Get().GetCulture(Language);
		Options.Add({FName(Language), FText::AsCultureInvariant(Culture ? Culture->GetNativeLanguage() : Language)});
	}
	const UAgLanguageSubsystem* Languages = GetGameInstance()->GetSubsystem<UAgLanguageSubsystem>();
	LanguageDropdown->SetOptions(Options, Languages ? FName(Languages->GetLanguage()) : NAME_None);
}

void UAgLanguageSettings::ApplyLanguage()
{
	if (UAgLanguageSubsystem* Languages = GetGameInstance()->GetSubsystem<UAgLanguageSubsystem>())
	{
		Languages->SetLanguage(LanguageDropdown->GetSelectedOption().ToString());
	}
}
