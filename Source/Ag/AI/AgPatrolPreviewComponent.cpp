// Copyright Woogle. All Rights Reserved.

#include "AI/AgPatrolPreviewComponent.h"

#if WITH_EDITOR
#include "Engine/World.h"
#include "Game/AgEnemySpawner.h"
#endif

UAgPatrolPreviewComponent::UAgPatrolPreviewComponent()
{
	bIsEditorOnly = true;
}

#if WITH_EDITOR
bool UAgPatrolPreviewComponent::GetPreviewPoints(TArray<FVector>& OutPoints) const
{
	const AAgPatrolRoute* Route = GetRoute();
	if (!Route || Route->GetPointCount() == 0)
	{
		return false;
	}
	for (int32 Index = 0; Index < Route->GetPointCount(); ++Index)
	{
		OutPoints.Add(Route->GetPointLocation(Index));
	}
	return true;
}

void UAgPatrolPreviewComponent::GetPreviewSegments(TArray<FAgPatrolPreviewSegment>& OutSegments) const
{
	const AAgPatrolRoute* Route = GetRoute();
	UWorld* World = GetWorld();
	if (!Route || !World || Route->GetPointCount() == 0)
	{
		return;
	}
	auto AddSegment = [World, &OutSegments](const FVector& From, const FVector& To)
	{
		FAgPatrolPreviewSegment& Segment = OutSegments.AddDefaulted_GetRef();
		Segment.Result = AAgPatrolRoute::FindWalkPath(*World, From, To, Segment.Path);
		if (Segment.Result != EAgPatrolPathResult::Walkable)
		{
			Segment.Path = {From, To};
		}
	};

	const int32 PointCount = Route->GetPointCount();
	if (PointCount >= 2)
	{
		for (int32 Index = 0; Index < PointCount; ++Index)
		{
			AddSegment(Route->GetPointLocation(Index), Route->GetPointLocation((Index + 1) % PointCount));
		}
	}
	if (const AAgEnemySpawner* Spawner = Cast<AAgEnemySpawner>(GetOwner()))
	{
		AddSegment(Spawner->GetActorLocation(), Route->GetPointLocation(Route->FindNearestPoint(Spawner->GetActorLocation())));
	}
}

const AAgPatrolRoute* UAgPatrolPreviewComponent::GetRoute() const
{
	const AActor* Owner = GetOwner();
	if (const AAgEnemySpawner* Spawner = Cast<AAgEnemySpawner>(Owner))
	{
		return Spawner->GetPatrolRoute();
	}
	return Cast<AAgPatrolRoute>(Owner);
}
#endif
