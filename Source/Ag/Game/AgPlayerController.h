// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AgPlayerController.generated.h"

enum class EAgCameraShake : uint8;

class UCommonActivatableWidget;
class UAgHUDWidget;
class UAgResultScreen;

/** 플레이어 컨트롤러. 화면(타이틀, HUD, 결과 UI)과 테스트용 콘솔 명령을 맡습니다. */
UCLASS()
class AAgPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AAgPlayerController();

	/** JSON의 단계별 셰이크를 로컬 카메라에 재생합니다. None은 재생하지 않습니다. */
	void PlayCameraShake(EAgCameraShake Shake);

	/** ScreenClass 화면을 만들어 플레이어 화면에 붙이고 활성화합니다. */
	UCommonActivatableWidget* PushScreen(TSubclassOf<UCommonActivatableWidget> ScreenClass, int32 ZOrder = 0);

	void ShowHUD(TSubclassOf<UAgHUDWidget> HUDClass);
	void HideHUD();

	/** Stage Clear UI나 Game Over UI를 띄웁니다. */
	void ShowResult(TSubclassOf<UAgResultScreen> ResultScreenClass, bool bStageCleared);

private:
	UPROPERTY()
	TObjectPtr<UAgHUDWidget> HUDWidget;
};
