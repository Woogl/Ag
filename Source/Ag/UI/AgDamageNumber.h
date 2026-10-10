// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AgDamageNumber.generated.h"

class UAgDamageNumberWidget;
class UWidgetComponent;

/**
 * 전투 HUD '대미지 숫자': a number at a world position that rises at a constant speed while fading out, then removes
 * itself. How long it shows and how far it rises are in the widget (WBP_DamageNumber).
 */
UCLASS(NotBlueprintable)
class AAgDamageNumber : public AActor
{
	GENERATED_BODY()

public:
	AAgDamageNumber();

	/** Shows Amount at Location; bLarge uses the large size (스킬·궁극기·처형). */
	static void Spawn(UWorld* World, TSubclassOf<UAgDamageNumberWidget> WidgetClass, const FVector& Location, int32 Amount, bool bLarge);

protected:
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "UI")
	TObjectPtr<UWidgetComponent> Widget;

	FVector StartLocation = FVector::ZeroVector;
	float Age = 0.f;
};
