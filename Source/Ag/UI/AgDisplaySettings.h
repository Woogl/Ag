// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GameFramework/GameUserSettings.h"
#include "AgDisplaySettings.generated.h"

class UAgDropdown;

/**
 * Settings의 Display 분류입니다. 창 모드와 해상도의 주인은 엔진 게임 사용자 설정이며, 고르면 엔진이 적용하고 저장합니다.
 * 다음 실행에서는 엔진이 시작할 때 저장한 값으로 창을 열고, 고른 적이 없을 때의 값은 DefaultGameUserSettings.ini에 있습니다.
 */
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgDisplaySettings : public UCommonUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

	/** 고를 수 있는 창 모드와 그 항목 글자입니다. 목록은 엔진의 창 모드 순서입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TMap<TEnumAsByte<EWindowMode::Type>, FText> WindowModeNames;

	/** 고를 수 있는 해상도입니다. 창 모드마다 모니터에 맞는 것만 보입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TArray<FIntPoint> StandardResolutions;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgDropdown> WindowModeDropdown;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgDropdown> ResolutionDropdown;

private:
	/** 지금 창 모드와 해상도로 두 드롭다운을 다시 채웁니다. */
	void Refresh();

	void ApplyWindowMode();
	void ApplyResolution();

	/** 해상도 드롭다운에 채운 해상도입니다. */
	TArray<FIntPoint> Resolutions;
};
