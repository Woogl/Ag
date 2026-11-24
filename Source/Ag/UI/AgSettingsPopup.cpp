// Copyright Woogle. All Rights Reserved.

#include "UI/AgSettingsPopup.h"

#include "Engine/GameInstance.h"
#include "Game/AgLanguageSubsystem.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "UI/AgButton.h"
#include "UI/AgDropdown.h"

UAgSettingsPopup::UAgSettingsPopup(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;
}

void UAgSettingsPopup::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 뒤의 타이틀 버튼이 켜진 채 남아 있어, 막지 않으면 방향키가 팝업 밖으로 포커스를 옮깁니다.
	LanguageDropdown->SetAllNavigationRules(EUINavigationRule::Stop, NAME_None);
	LanguageDropdown->SetNavigationRuleExplicit(EUINavigation::Down, ConfirmButton);
	ConfirmButton->SetAllNavigationRules(EUINavigationRule::Stop, NAME_None);
	ConfirmButton->SetNavigationRuleExplicit(EUINavigation::Up, LanguageDropdown);

	ConfirmButton->OnClicked().AddUObject(this, &ThisClass::DeactivateWidget);
	LanguageDropdown->OnSelectionChanged.AddUObject(this, &ThisClass::ApplyLanguage);
}

void UAgSettingsPopup::NativeOnActivated()
{
	Super::NativeOnActivated();

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

UWidget* UAgSettingsPopup::NativeGetDesiredFocusTarget() const
{
	return LanguageDropdown->GetDesiredFocusWidget();
}

void UAgSettingsPopup::ApplyLanguage()
{
	if (UAgLanguageSubsystem* Languages = GetGameInstance()->GetSubsystem<UAgLanguageSubsystem>())
	{
		Languages->SetLanguage(LanguageDropdown->GetSelectedOption().ToString());
	}
}
