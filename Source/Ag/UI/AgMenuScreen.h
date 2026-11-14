// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "AgMenuScreen.generated.h"

/** 엔진 기본 화면은 입력 설정이 없어 열려도 입력 모드와 마우스 캡처가 그대로이므로, 메뉴·모달 화면은 이 클래스를 상속합니다. */
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgMenuScreen : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	void BindEscapeToClose();
};
