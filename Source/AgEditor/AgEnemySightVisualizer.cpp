// Copyright Woogle. All Rights Reserved.

#include "AgEnemySightVisualizer.h"

#include "Character/AgEnemySightPreviewComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "SceneManagement.h"

void FAgEnemySightVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	const UAgEnemySightPreviewComponent* Preview = Cast<UAgEnemySightPreviewComponent>(Component);
	if (!Preview || !Preview->GetWorld() || Preview->GetWorld()->WorldType != EWorldType::Editor || !Preview->GetOwner()->IsSelected())
	{
		return;
	}
	FVector Origin;
	FQuat Rotation;
	float Distance;
	float Angle;
	if (!Preview->GetSightPreview(Origin, Rotation, Distance, Angle))
	{
		return;
	}
	const FLinearColor Color(0.1f, 0.8f, 1.f);
	const int32 Segments = FMath::Max(1, FMath::CeilToInt(Angle / 3.f));
	FVector Previous;
	for (int32 Index = 0; Index <= Segments; ++Index)
	{
		const float Radians = FMath::DegreesToRadians(-Angle * 0.5f + Angle * Index / Segments);
		const FVector Point = Origin + Rotation.RotateVector(FVector(FMath::Cos(Radians), FMath::Sin(Radians), 0.)) * Distance;
		if (Index == 0 || Index == Segments)
		{
			PDI->DrawLine(Origin, Point, Color, SDPG_World, 2.f);
		}
		if (Index > 0)
		{
			PDI->DrawLine(Previous, Point, Color, SDPG_World, 2.f);
		}
		Previous = Point;
	}
}
