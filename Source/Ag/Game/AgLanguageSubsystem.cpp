// Copyright Woogle. All Rights Reserved.

#include "Game/AgLanguageSubsystem.h"

#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/PolyglotTextData.h"
#include "Internationalization/StringTableCore.h"
#include "Internationalization/StringTableRegistry.h"
#include "Internationalization/TextLocalizationManager.h"
#include "Kismet/KismetInternationalizationLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Parse.h"

namespace AgLanguageSubsystemPrivate
{
	/** 문구 표의 SourceString 언어입니다. */
	const TCHAR* NativeLanguage = TEXT("ko");

	/** 원하는 언어의 번역이 없을 때 쓰는 언어입니다. */
	const TCHAR* FallbackLanguage = TEXT("en");

	const FName StringTableId(TEXT("StringTable"));

	/** 엔진은 SourceString 말고 다른 칼럼을 항목의 메타데이터로 읽으므로, 이름이 언어인 칼럼을 그 언어의 번역으로 등록합니다. */
	void RegisterTranslations()
	{
		const FStringTableConstPtr Table = FStringTableRegistry::Get().FindStringTable(StringTableId);
		if (!Table)
		{
			return;
		}
		FInternationalization& I18N = FInternationalization::Get();
		TArray<FPolyglotTextData> Translations;
		Table->EnumerateSourceStrings([&Table, &I18N, &Translations](const FString& Key, const FString& SourceString)
		{
			FPolyglotTextData& Translation = Translations.Emplace_GetRef(ELocalizedTextSourceCategory::Game, Table->GetNamespace(), Key, SourceString, NativeLanguage);
			Table->EnumerateMetaData(FTextKey(Key), [&I18N, &Translation](FName Column, const FString& Text)
			{
				if (I18N.GetCulture(Column.ToString()))
				{
					Translation.AddLocalizedString(Column.ToString(), Text);
				}
				return true;
			});
			return true;
		});
		FTextLocalizationManager::Get().RegisterPolyglotTextData(Translations);
	}

	/** 엔진이 시작 언어를 찾는 순서(TextLocalizationManager.cpp의 GetRequestedCulture)대로 명령줄, 게임·엔진 설정, 윈도우 언어를 봅니다. */
	FString GetRequestedLanguage()
	{
		FString Language;
		if (FParse::Value(FCommandLine::Get(), TEXT("LANGUAGE="), Language) || FParse::Value(FCommandLine::Get(), TEXT("CULTURE="), Language))
		{
			return Language;
		}
		for (const FString& Ini : {GGameUserSettingsIni, GGameIni, GEngineIni})
		{
			if (GConfig->GetString(TEXT("Internationalization"), TEXT("Language"), Language, Ini) || GConfig->GetString(TEXT("Internationalization"), TEXT("Culture"), Language, Ini))
			{
				return Language;
			}
		}
		return FInternationalization::Get().GetDefaultLanguage()->GetName();
	}
} // namespace AgLanguageSubsystemPrivate

void UAgLanguageSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	AgLanguageSubsystemPrivate::RegisterTranslations();
	if (GIsEditor)
	{
		return;
	}

	// 엔진은 번역을 등록하기 전에 시작 언어를 골라 번역이 없는 언어로 시작하므로, 등록한 뒤 다시 고릅니다. 원하는 언어의 번역이
	// 없으면 엔진은 원문 언어를 쓰지만, 기획은 그 경우 영어입니다.
	const TArray<FString> Languages = GetLanguages();
	FString StartLanguage = AgLanguageSubsystemPrivate::FallbackLanguage;
	for (const FString& Language : FInternationalization::Get().GetPrioritizedCultureNames(AgLanguageSubsystemPrivate::GetRequestedLanguage()))
	{
		if (Languages.Contains(Language))
		{
			StartLanguage = Language;
			break;
		}
	}
	FInternationalization::Get().SetCurrentLanguage(StartLanguage);
}

void UAgLanguageSubsystem::Deinitialize()
{
#if WITH_EDITOR
	if (!PreviewLanguage.IsEmpty())
	{
		FTextLocalizationManager& Localization = FTextLocalizationManager::Get();
		if (Localization.ShouldGameLocalizationPreviewAutoEnable())
		{
			Localization.EnableGameLocalizationPreview();
		}
		else
		{
			Localization.DisableGameLocalizationPreview();
		}
	}
#endif

	Super::Deinitialize();
}

TArray<FString> UAgLanguageSubsystem::GetLanguages()
{
	TArray<FString> Languages = FTextLocalizationManager::Get().GetLocalizedCultureNames(ELocalizationLoadFlags::Game);
	const FString NativeLanguage = AgLanguageSubsystemPrivate::NativeLanguage;
	Languages.Sort([&NativeLanguage](const FString& A, const FString& B)
	{ return (A == NativeLanguage) != (B == NativeLanguage) ? A == NativeLanguage : A < B; });
	return Languages;
}

FString UAgLanguageSubsystem::GetLanguage() const
{
#if WITH_EDITOR
	if (GIsEditor)
	{
		if (!PreviewLanguage.IsEmpty())
		{
			return PreviewLanguage;
		}
		const FTextLocalizationManager& Localization = FTextLocalizationManager::Get();
		const FString Configured = Localization.GetConfiguredGameLocalizationPreviewLanguage();
		return Localization.IsGameLocalizationPreviewEnabled() && !Configured.IsEmpty() ? Configured : AgLanguageSubsystemPrivate::NativeLanguage;
	}
#endif
	return FInternationalization::Get().GetCurrentLanguage()->GetName();
}

void UAgLanguageSubsystem::SetLanguage(const FString& Language)
{
#if WITH_EDITOR
	if (GIsEditor)
	{
		// 에디터에서 엔진 언어를 바꾸면 에디터 UI 언어까지 바뀌므로, 게임 문구만 엔진의 번역 미리 보기로 바꾸고 저장하지 않습니다.
		PreviewLanguage = Language;
		FTextLocalizationManager::Get().EnableGameLocalizationPreview(Language);
		return;
	}
#endif
	UKismetInternationalizationLibrary::SetCurrentLanguage(Language, true);
}
