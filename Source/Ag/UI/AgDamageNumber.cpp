// Copyright Woogle. All Rights Reserved.

#include "UI/AgDamageNumber.h"

#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "UI/AgDamageNumberWidget.h"

AAgDamageNumber::AAgDamageNumber()
{
	PrimaryActorTick.bCanEverTick = true;

	// Screen space: the number keeps its size at any distance and draws over the world.
	Widget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Widget"));
	Widget->SetWidgetSpace(EWidgetSpace::Screen);
	Widget->SetDrawAtDesiredSize(true);
	Widget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Widget->SetGenerateOverlapEvents(false);
	RootComponent = Widget;
}

void AAgDamageNumber::Spawn(UWorld* World, TSubclassOf<UAgDamageNumberWidget> WidgetClass, const FVector& Location, int32 Amount, bool bLarge)
{
	if (!World || !WidgetClass)
	{
		return;
	}

	// The widget component creates the widget on BeginPlay, so its class goes in before spawning finishes.
	const FTransform Transform(Location);
	AAgDamageNumber* Number = World->SpawnActorDeferred<AAgDamageNumber>(StaticClass(), Transform);
	if (!Number)
	{
		return;
	}
	Number->Widget->SetWidgetClass(WidgetClass);
	Number->StartLocation = Location;
	Number->FinishSpawning(Transform);

	if (UAgDamageNumberWidget* NumberWidget = Cast<UAgDamageNumberWidget>(Number->Widget->GetWidget()))
	{
		NumberWidget->SetDamage(Amount, bLarge, World->GetWorldSettings()->GetEffectiveTimeDilation());
	}
}

void AAgDamageNumber::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UAgDamageNumberWidget* NumberWidget = Cast<UAgDamageNumberWidget>(Widget->GetWidget());
	Age += DeltaSeconds;
	if (!NumberWidget || Age >= NumberWidget->GetLifetime())
	{
		Destroy();
		return;
	}

	const float Alpha = Age / NumberWidget->GetLifetime();
	SetActorLocation(StartLocation + FVector(0.f, 0.f, NumberWidget->GetRiseHeight() * Alpha));
	NumberWidget->SetGameSpeed(GetWorld()->GetWorldSettings()->GetEffectiveTimeDilation());
}
