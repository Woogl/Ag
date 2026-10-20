// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AgDamageFloater.generated.h"

class UAgDamageFloaterWidget;
class UWidgetComponent;

UCLASS(NotBlueprintable)
class AAgDamageFloater : public AActor
{
	GENERATED_BODY()

public:
	AAgDamageFloater();

	static void Spawn(UWorld* World, TSubclassOf<UAgDamageFloaterWidget> WidgetClass, const FVector& Location, int32 Amount);

protected:
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "UI")
	TObjectPtr<UWidgetComponent> Widget;

	FVector StartLocation = FVector::ZeroVector;
	float Age = 0.f;
};
