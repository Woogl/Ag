// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "AgLanguageSettings.generated.h"

class UAgDropdown;

/** Settings의 Language 분류입니다. 언어 서브시스템의 언어 목록과 지금 언어로 드롭다운을 채우고, 고르면 언어를 바꿉니다. */
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgLanguageSettings : public UCommonUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgDropdown> LanguageDropdown;

private:
	void ApplyLanguage();
};
