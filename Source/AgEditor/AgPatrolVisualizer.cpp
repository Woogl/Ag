// Copyright Woogle. All Rights Reserved.

#include "AgPatrolVisualizer.h"

#include "AI/AgPatrolPreviewComponent.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "SceneManagement.h"
#include "SceneView.h"

namespace AgPatrolVisualizerPrivate
{
	const FLinearColor WalkableColor(0.15f, 0.85f, 0.15f);
	const FLinearColor UnreachableColor(1.f, 0.1f, 0.1f);
	const FLinearColor UnknownColor(0.45f, 0.45f, 0.45f);
	constexpr float LineThickness = 3.f;
	constexpr float PointSize = 12.f;
	constexpr float NumberScale = 1.6f;
	const FVector2D NumberOffset(8.f, -36.f);

	const UAgPatrolPreviewComponent* GetEditorPreview(const UActorComponent* Component)
	{
		const UAgPatrolPreviewComponent* Preview = Cast<UAgPatrolPreviewComponent>(Component);
		return Preview && Preview->GetWorld() && Preview->GetWorld()->WorldType == EWorldType::Editor ? Preview : nullptr;
	}

	const FLinearColor& GetPathColor(EAgPatrolPathResult Result)
	{
		switch (Result)
		{
			case EAgPatrolPathResult::Walkable:
				return WalkableColor;
			case EAgPatrolPathResult::Unreachable:
				return UnreachableColor;
			default:
				return UnknownColor;
		}
	}
} // namespace AgPatrolVisualizerPrivate

void FAgPatrolVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	const UAgPatrolPreviewComponent* Preview = AgPatrolVisualizerPrivate::GetEditorPreview(Component);
	TArray<FVector> Points;
	if (!Preview || !Preview->GetPreviewPoints(Points))
	{
		return;
	}
	TArray<FAgPatrolPreviewSegment> Segments;
	Preview->GetPreviewSegments(Segments);
	// 언덕을 넘는 구간이 지형에 가려지지 않도록 전경에 그립니다.
	for (const FAgPatrolPreviewSegment& Segment : Segments)
	{
		const FLinearColor& Color = AgPatrolVisualizerPrivate::GetPathColor(Segment.Result);
		for (int32 Index = 1; Index < Segment.Path.Num(); ++Index)
		{
			PDI->DrawLine(Segment.Path[Index - 1], Segment.Path[Index], Color, SDPG_Foreground, AgPatrolVisualizerPrivate::LineThickness);
		}
	}
	for (const FVector& Point : Points)
	{
		PDI->DrawPoint(Point, FLinearColor::White, AgPatrolVisualizerPrivate::PointSize, SDPG_Foreground);
	}
}

void FAgPatrolVisualizer::DrawVisualizationHUD(const UActorComponent* Component, const FViewport* Viewport, const FSceneView* View, FCanvas* Canvas)
{
	const UAgPatrolPreviewComponent* Preview = AgPatrolVisualizerPrivate::GetEditorPreview(Component);
	TArray<FVector> Points;
	if (!Preview || !Preview->GetPreviewPoints(Points))
	{
		return;
	}
	const float DPIScale = Canvas->GetDPIScale();
	for (int32 Index = 0; Index < Points.Num(); ++Index)
	{
		FVector2D Pixel;
		if (View->WorldToPixel(Points[Index], Pixel))
		{
			FCanvasTextItem Number(Pixel / DPIScale + AgPatrolVisualizerPrivate::NumberOffset, FText::AsNumber(Index + 1), GEngine->GetLargeFont(), FLinearColor::White);
			Number.Scale = FVector2D(AgPatrolVisualizerPrivate::NumberScale);
			Number.EnableShadow(FLinearColor::Black);
			Canvas->DrawItem(Number);
		}
	}
}
