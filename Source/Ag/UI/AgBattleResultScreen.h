// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/AgMenuScreen.h"
#include "AgBattleResultScreen.generated.h"

class UAgButton;
class UCommonTextBlock;
class UWidgetAnimation;

UCLASS(Abstract, meta = (DisableNativeTick))
class UAgBattleResultScreen : public UAgMenuScreen
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual void OnAnimationFinished_Implementation(const UWidgetAnimation* Animation) override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY(EditAnywhere, Category = "Result")
	FText StageClearTitle;

	UPROPERTY(EditAnywhere, Category = "Result")
	FText GameOverTitle;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> FadeInAnimation;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> TitleText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> CombatStatsTable;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> AttemptText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> DurationText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> EnemiesText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> ParryCountText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> PerfectDodgeCountText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> HitCountText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> RestartButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> ReturnToTitleButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> QuitButton;

private:
	/** 시뮬레이션 룸에서 결과로 끝난 전투가 있으면 통계 표를 채워 보이고, 아니면 접습니다. */
	void ShowCombatStats();

	/** 버튼마다 꺼야 포커스를 받은 버튼도 실행 키(Enter)를 받지 않습니다. 버튼을 담은 패널만 끄면 마우스만 막힙니다. */
	void SetButtonsEnabled(bool bEnabled);
};
