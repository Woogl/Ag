// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/AgMenuScreen.h"
#include "AgSettingsScreen.generated.h"

class UAgButton;
class UAgSettingsPanel;

/**
 * 타이틀에서 여는 Settings 화면입니다. 안의 Settings 내용은 위젯 블루프린트의 인스턴스 값(bAutoActivate)으로 화면과 함께
 * 켜지고 꺼집니다.
 */
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgSettingsScreen : public UAgMenuScreen
{
	GENERATED_BODY()

public:
	UAgSettingsScreen(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeOnInitialized() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgSettingsPanel> SettingsPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> BackButton;
};
