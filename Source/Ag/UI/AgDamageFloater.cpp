// Copyright Woogle. All Rights Reserved.

#include "UI/AgDamageFloater.h"

#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "UI/AgDamageFloaterWidget.h"

AAgDamageFloater::AAgDamageFloater()
{
	PrimaryActorTick.bCanEverTick = true;

	Widget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Widget"));
	Widget->SetWidgetSpace(EWidgetSpace::Screen);
	Widget->SetDrawAtDesiredSize(true);
	Widget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Widget->SetGenerateOverlapEvents(false);
	RootComponent = Widget;
}

void AAgDamageFloater::Spawn(UWorld* World, TSubclassOf<UAgDamageFloaterWidget> WidgetClass, const FVector& Location, int32 Amount)
{
	if (!World || !WidgetClass)
	{
		return;
	}

	// 위젯 컴포넌트는 BeginPlay에서 위젯을 만들므로,
	// 스폰을 마치기 전에 위젯 클래스를 넣습니다.
	const FTransform Transform(Location);
	AAgDamageFloater* Floater = World->SpawnActorDeferred<AAgDamageFloater>(StaticClass(), Transform);
	if (!Floater)
	{
		return;
	}
	Floater->Widget->SetWidgetClass(WidgetClass);
	Floater->StartLocation = Location;
	Floater->FinishSpawning(Transform);

	if (UAgDamageFloaterWidget* FloaterWidget = Cast<UAgDamageFloaterWidget>(Floater->Widget->GetWidget()))
	{
		FloaterWidget->SetDamage(Amount);
	}
}

void AAgDamageFloater::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 위젯 애니메이션이 실제 시간으로 흐르므로 함께 실제 시간으로 셉니다.
	UAgDamageFloaterWidget* FloaterWidget = Cast<UAgDamageFloaterWidget>(Widget->GetWidget());
	Age += GetWorld()->GetTime().GetDeltaRealTimeSeconds();
	if (!FloaterWidget || Age >= FloaterWidget->GetLifetime())
	{
		Destroy();
		return;
	}

	const float Alpha = Age / FloaterWidget->GetLifetime();
	SetActorLocation(StartLocation + FVector(0.f, 0.f, FloaterWidget->GetRiseHeight() * Alpha));
}
