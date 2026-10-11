// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "AgResultScreen.generated.h"

class UAgButton;
class UCommonTextBlock;
class UWidgetAnimation;

/**
 * Stage Clear UI / Game Over UI (게임 플로우). 결과 UI가 떠 있는 동안에도 게임은 계속 흐릅니다.
 * 화면이 어두워지며 나타나는 페이드 인이 끝난 뒤부터 버튼 입력을 받습니다.
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
	virtual void OnAnimationFinished_Implementation(const UWidgetAnimation* Animation) override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	/** Stage Clear UI의 제목 문구 */
	UPROPERTY(EditAnywhere, Category = "Result")
	FText StageClearTitle;

	/** Game Over UI의 제목 문구 */
	UPROPERTY(EditAnywhere, Category = "Result")
	FText GameOverTitle;

	/** 화면을 서서히 어둡게 하면서 결과 UI를 서서히 표시하는 위젯 애니메이션. 모양과 길이는 애니메이션이 정합니다. */
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> FadeInAnimation;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> TitleText;

	/** 버튼을 담은 패널. 페이드 인이 끝날 때까지 꺼 둡니다. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> ButtonList;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> TryAgainButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> ReturnToTitleButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> QuitButton;
};
