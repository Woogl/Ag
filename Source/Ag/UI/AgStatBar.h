// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "AgStatBar.generated.h"

class UProgressBar;
class UTextBlock;

UCLASS(Abstract, meta = (DisableNativeTick))
class UAgStatBar : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	// bShowDrop이 false면 줄어든 만큼 잔상 없이 줄입니다.
	void SetValue(float Value, float MaxValue, bool bShowDrop = true);
	void SetRatio(float Ratio, bool bShowDrop = true);

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> Bar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> TrailBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ValueText;

	UPROPERTY(EditAnywhere, Category = "Stat Bar")
	FLinearColor FillColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Stat Bar")
	bool bShowTrail = true;

	UPROPERTY(EditAnywhere, Category = "Stat Bar")
	bool bShowValue = false;

	UPROPERTY(EditDefaultsOnly, Category = "Stat Bar|Trail")
	float TrailDelay = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Stat Bar|Trail")
	float TrailShrinkTime = 0.f;

private:
	void StartTrailShrink();
	void UpdateTrailShrink();
	void SetTrailRatio(float Ratio);

	FIntPoint ShownValues = FIntPoint(INDEX_NONE, INDEX_NONE);

	/** 음수면 아직 값을 받지 않았습니다. */
	float BarRatio = -1.f;
	float TrailRatio = 0.f;
	float TrailShrinkFrom = 0.f;
	double TrailShrinkStartTime = 0.0;
	FTimerHandle TrailTimer;
};
