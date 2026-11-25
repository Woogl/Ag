// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "AgSettingsPanel.generated.h"

class UAgTabList;
class UCommonAnimatedSwitcher;

/**
 * Settings 내용입니다. 왼쪽 분류 목록으로 오른쪽 분류 내용을 바꾸며, 일시정지 메뉴의 Settings 탭과 타이틀의 Settings 화면이
 * 함께 씁니다. 고른 분류의 이름에 첫 포커스를 줍니다.
 */
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgSettingsPanel : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgTabList> CategoryList;

	/** 분류 내용은 활성화 위젯이 아니어야 합니다. 활성화 위젯이면 CommonUI가 첫 포커스를 분류 이름이 아닌 내용에 줍니다. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonAnimatedSwitcher> CategoryContent;

private:
	UFUNCTION()
	void HandleCategorySelected(FName TabId);
};
