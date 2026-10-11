// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AgDamageFloater.generated.h"

class UAgDamageFloaterWidget;
class UWidgetComponent;

/**
 * 전투 HUD '대미지 숫자': 월드 위치에 띄운 숫자가 일정한 속도로 떠오르는 동안 위젯이 투명하게 만들고, 다 사라지면
 * 스스로 지웁니다. 투명해지는 애니메이션(곧 표시 시간)과 떠오르는 높이는 위젯(WBP_DamageFloater)에 있습니다.
 */
UCLASS(NotBlueprintable)
class AAgDamageFloater : public AActor
{
	GENERATED_BODY()

public:
	AAgDamageFloater();

	/** Location에 Amount를 띄웁니다. bLarge면 큰 크기(스킬·궁극기·처형)로 표시합니다. */
	static void Spawn(UWorld* World, TSubclassOf<UAgDamageFloaterWidget> WidgetClass, const FVector& Location, int32 Amount, bool bLarge);

protected:
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "UI")
	TObjectPtr<UWidgetComponent> Widget;

	FVector StartLocation = FVector::ZeroVector;
	float Age = 0.f;
};
