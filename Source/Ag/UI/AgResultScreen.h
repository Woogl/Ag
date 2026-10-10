// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "AgResultScreen.generated.h"

class UAgButton;
class UCommonTextBlock;

/**
 * Stage Clear UI / Game Over UI (게임 플로우). The game keeps running while it is shown.
 * Fades in with the screen darkening, then accepts button input.
 */
UCLASS(Abstract)
class UAgResultScreen : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	void SetResult(bool bStageCleared);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	/** Stage Clear UI의 제목 문구 */
	UPROPERTY(EditAnywhere, Category = "Result")
	FText StageClearTitle;

	/** Game Over UI의 제목 문구 */
	UPROPERTY(EditAnywhere, Category = "Result")
	FText GameOverTitle;

	/** 화면을 서서히 어둡게 하면서 결과 UI를 서서히 표시하는 시간 */
	UPROPERTY(EditAnywhere, Category = "Result", meta = (Units = "s", ClampMin = 0))
	float FadeInTime = 0.f;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> TitleText;

	/** Panel holding the buttons. Disabled until the fade-in ends. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> ButtonList;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> TryAgainButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> ReturnToTitleButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> QuitButton;

private:
	void FinishFadeIn();
	void HandleTryAgainClicked();
	void HandleReturnToTitleClicked();
	void HandleQuitClicked();

	float FadeElapsed = 0.f;
	bool bFadingIn = false;
};
