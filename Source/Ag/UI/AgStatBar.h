// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "AgStatBar.generated.h"

class UImage;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UProgressBar;
class UTextBlock;

UCLASS(Abstract, meta = (DisableNativeTick))
class UAgStatBar : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	void SetValue(float Value, float MaxValue);
	void SetRatio(float Ratio);
	void ClearTrail();

	/** 채움을 그로기 머티리얼로 바꾸고 시작 연출을 보입니다. Time은 머티리얼에 넘기는 그로기 경과 시간(초)입니다. */
	void ShowGroggy(float Time);
	void HideGroggy();

protected:
	virtual void NativePreConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> Bar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> TrailBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ValueText;

	/** 그로기가 시작되는 순간의 번쩍임과 파편입니다. 바 바깥까지 그리도록 바보다 크게 놓고, 브러시 머티리얼이 Time으로 재생합니다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> GroggyBurst;

	UPROPERTY(EditAnywhere, Category = "Stat Bar")
	FLinearColor FillColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Stat Bar")
	TObjectPtr<UMaterialInterface> GroggyFillMaterial;

	UPROPERTY(EditAnywhere, Category = "Stat Bar")
	bool bShowTrail = true;

	UPROPERTY(EditAnywhere, Category = "Stat Bar")
	bool bShowValue = false;

	/** 붙여 놓은 두 바의 수치가 겹치지 않도록, 글자를 바의 위 끝이나 아래 끝에 맞출 때 씁니다. */
	UPROPERTY(EditAnywhere, Category = "Stat Bar", meta = (EditCondition = "bShowValue"))
	TEnumAsByte<EVerticalAlignment> ValueAlignment = VAlign_Center;

	UPROPERTY(EditDefaultsOnly, Category = "Stat Bar|Trail")
	float TrailDelay = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Stat Bar|Trail")
	float TrailShrinkTime = 0.f;

private:
	void StartTrailShrink();
	void UpdateTrailShrink();
	void SetTrailRatio(float Ratio);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GroggyFill;

	FSlateBrush DefaultFillImage;

	FIntPoint ShownValues = FIntPoint(INDEX_NONE, INDEX_NONE);

	float BarRatio = 0.f;
	float TrailRatio = 0.f;
	float TrailShrinkFrom = 0.f;
	double TrailShrinkStartTime = 0.0;
	FTimerHandle TrailTimer;
};
