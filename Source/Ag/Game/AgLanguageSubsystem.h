// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AgLanguageSubsystem.generated.h"

/**
 * 게임 문구의 언어. 번역은 문구 표의 언어 칼럼에서 엔진에 등록합니다. 처음에는 윈도우 언어를 따르고, 고른 언어는 엔진 방식
 * (GameUserSettings의 Internationalization)으로 저장합니다.
 */
UCLASS()
class UAgLanguageSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** 고를 수 있는 언어(문화권 이름)입니다. 원문 언어가 먼저입니다. */
	static TArray<FString> GetLanguages();

	FString GetLanguage() const;

	void SetLanguage(const FString& Language);

private:
#if WITH_EDITOR
	/** 에디터에서 미리 보기로 바꾼 언어입니다. */
	FString PreviewLanguage;
#endif
};
