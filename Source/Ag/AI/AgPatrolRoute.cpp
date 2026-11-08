// Copyright Woogle. All Rights Reserved.

#include "AI/AgPatrolRoute.h"

#include "AI/AgPatrolPreviewComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/World.h"

#if WITH_EDITOR
#include "Algo/MinElement.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Volume.h"
#include "Logging/MessageLog.h"
#include "Logging/TokenizedMessage.h"
#include "Misc/UObjectToken.h"
#include "NavigationData.h"
#include "NavigationSystem.h"
#endif

#if WITH_EDITOR
namespace AgPatrolRoutePrivate
{
	constexpr double GroundSearchHeight = 100000.;
	constexpr double NearFloorSearchHeight = 100.;
	constexpr int32 MaxFloorTraces = 16;
	const FVector NavigableFloorExtent(10., 10., 50.);

	/** Start에서 End로 그은 선에 맞는 바닥 면의 높이를 모읍니다. 걸을 수 없는 경사, 볼륨, 캐릭터가 통과하는 면은 건너뜁니다. */
	void TraceFloors(const UWorld& World, const FVector& Start, const FVector& End, bool bFirstOnly, TArray<double>& OutHeights)
	{
		const float WalkableFloorZ = GetDefault<UCharacterMovementComponent>()->GetWalkableFloorZ();
		FCollisionQueryParams Params(SCENE_QUERY_STAT(AgPatrolFloor));
		for (int32 Index = 0; Index < MaxFloorTraces; ++Index)
		{
			FHitResult Hit;
			if (!World.LineTraceSingleByObjectType(Hit, Start, End, FCollisionObjectQueryParams(ECC_WorldStatic), Params) || !Hit.GetComponent())
			{
				return;
			}
			const UPrimitiveComponent* Component = Hit.GetComponent();
			const bool bFloor = !Hit.bStartPenetrating && Hit.ImpactNormal.Z >= WalkableFloorZ && !Cast<AVolume>(Hit.GetActor()) &&
			                    Component->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block;
			if (bFloor)
			{
				OutHeights.Add(Hit.ImpactPoint.Z);
				if (bFirstOnly)
				{
					return;
				}
			}
			Params.AddIgnoredComponent(Component);
		}
	}

	/** 내비게이션이 있는 바닥만 남깁니다. 내비게이션이 없거나 남는 바닥이 없으면 그대로 둡니다. */
	void KeepNavigableFloors(const UWorld& World, const FVector& Point, TArray<double>& Heights)
	{
		const UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(&World);
		const ANavigationData* NavigationData = NavigationSystem ? NavigationSystem->GetDefaultNavDataInstance() : nullptr;
		if (!NavigationData)
		{
			return;
		}
		TArray<double> Navigable = Heights.FilterByPredicate([NavigationData, &Point](double Height)
		{
			FNavLocation Location;
			return NavigationData->ProjectPoint(FVector(Point.X, Point.Y, Height), Location, NavigableFloorExtent);
		});
		if (!Navigable.IsEmpty())
		{
			Heights = MoveTemp(Navigable);
		}
	}

	/** 점의 수평 위치에서 점 높이에 가장 가까운 바닥 면을 찾습니다. */
	TOptional<FVector> FindGround(const UWorld& World, const FVector& Point)
	{
		TArray<double> Heights;
		// 위에서 그은 선은 맞은 메시를 건너뛰며 내려가 한 메시 안의 아래층을 찾지 못하므로, 점 바로 위에서도 긋습니다.
		TraceFloors(World, Point + FVector(0., 0., GroundSearchHeight), Point - FVector(0., 0., GroundSearchHeight), false, Heights);
		TraceFloors(World, Point + FVector(0., 0., NearFloorSearchHeight), Point - FVector(0., 0., GroundSearchHeight), true, Heights);
		// 바닥 아래에 숨은 구조물 윗면처럼 적이 갈 수 없는 면에 붙지 않도록, 내비게이션이 있는 바닥을 먼저 고릅니다.
		KeepNavigableFloors(World, Point, Heights);
		const double* Nearest = Algo::MinElementBy(Heights, [&Point](double Height)
		{ return FMath::Abs(Height - Point.Z); });
		if (!Nearest)
		{
			return {};
		}
		return FVector(Point.X, Point.Y, *Nearest);
	}
} // namespace AgPatrolRoutePrivate
#endif

AAgPatrolRoute::AAgPatrolRoute()
{
	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
	SetRootComponent(Spline);
	Spline->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Spline->SetCanEverAffectNavigation(false);
	Spline->SetClosedLoop(true, false);
	for (int32 Index = 0; Index < Spline->GetNumberOfSplinePoints(); ++Index)
	{
		Spline->SetSplinePointType(Index, ESplinePointType::Linear, false);
	}
	Spline->UpdateSpline();

#if WITH_EDITORONLY_DATA
	PatrolPreview = CreateEditorOnlyDefaultSubobject<UAgPatrolPreviewComponent>(TEXT("PatrolPreview"));
#endif
}

int32 AAgPatrolRoute::GetPointCount() const
{
	return Spline->GetNumberOfSplinePoints();
}

FVector AAgPatrolRoute::GetPointLocation(int32 Index) const
{
	return Spline->GetLocationAtSplinePoint(Index, ESplineCoordinateSpace::World);
}

int32 AAgPatrolRoute::FindNearestPoint(const FVector& Location) const
{
	int32 NearestIndex = INDEX_NONE;
	double NearestDistanceSq = TNumericLimits<double>::Max();
	for (int32 Index = 0; Index < GetPointCount(); ++Index)
	{
		const double DistanceSq = FVector::DistSquared2D(GetPointLocation(Index), Location);
		if (DistanceSq < NearestDistanceSq)
		{
			NearestDistanceSq = DistanceSq;
			NearestIndex = Index;
		}
	}
	return NearestIndex;
}

void AAgPatrolRoute::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

#if WITH_EDITOR
	// 게임 중에 생성할 때도 불리므로 편집 중인 레벨에서만 바닥에 붙입니다.
	if (GetWorld() && GetWorld()->WorldType == EWorldType::Editor)
	{
		SnapPointsToGround();
	}
#endif
}

#if WITH_EDITOR
EAgPatrolPathResult AAgPatrolRoute::FindWalkPath(UWorld& World, const FVector& From, const FVector& To, TArray<FVector>& OutPath)
{
	UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(&World);
	const ANavigationData* NavigationData = NavigationSystem ? NavigationSystem->GetDefaultNavDataInstance() : nullptr;
	if (!NavigationData)
	{
		return EAgPatrolPathResult::Unknown;
	}
	const FPathFindingResult Result = NavigationSystem->FindPathSync(FPathFindingQuery(nullptr, *NavigationData, From, To));
	if (!Result.IsSuccessful() || Result.IsPartial() || !Result.Path.IsValid())
	{
		return EAgPatrolPathResult::Unreachable;
	}
	for (const FNavPathPoint& PathPoint : Result.Path->GetPathPoints())
	{
		OutPath.Add(PathPoint.Location);
	}
	return EAgPatrolPathResult::Walkable;
}

TSharedRef<FTokenizedMessage> AAgPatrolRoute::MakeWarning(const UObject& Object, const FString& Text)
{
	TSharedRef<FTokenizedMessage> Message = FTokenizedMessage::Create(EMessageSeverity::Warning);
	Message->AddToken(FUObjectToken::Create(&Object));
	Message->AddToken(FTextToken::Create(FText::FromString(Text)));
	return Message;
}

void AAgPatrolRoute::ReportEditProblems(const TArray<TSharedRef<FTokenizedMessage>>& Messages)
{
	if (Messages.IsEmpty())
	{
		return;
	}
	FMessageLog Log(TEXT("MapCheck"));
	Log.NewPage(FText::FromString(TEXT("Patrol route")));
	Log.AddMessages(Messages);
	Log.Notify(FText::FromString(TEXT("Patrol route has problems. See the Map Check log.")), EMessageSeverity::Warning);
}

void AAgPatrolRoute::SetPointLocations(const TArray<FVector>& WorldLocations)
{
	Modify();
	Spline->Modify();
	Spline->SetSplinePoints(WorldLocations, ESplineCoordinateSpace::World, false);
	for (int32 Index = 0; Index < Spline->GetNumberOfSplinePoints(); ++Index)
	{
		Spline->SetSplinePointType(Index, ESplinePointType::Linear, false);
	}
	Spline->bSplineHasBeenEdited = true;
	Spline->UpdateSpline();
	SnapPointsToGround();

	TArray<TSharedRef<FTokenizedMessage>> Messages;
	CollectProblems(Messages);
	ReportEditProblems(Messages);
}

void AAgPatrolRoute::PostEditMove(bool bFinished)
{
	// 엔진 스플라인 편집도 점 끌기를 마치면 이 함수를 부르며, Super가 OnConstruction을 다시 실행해 바닥에 붙입니다.
	Super::PostEditMove(bFinished);

	if (bFinished)
	{
		TArray<TSharedRef<FTokenizedMessage>> Messages;
		CollectProblems(Messages);
		ReportEditProblems(Messages);
	}
}

void AAgPatrolRoute::CheckForErrors()
{
	Super::CheckForErrors();

	TArray<TSharedRef<FTokenizedMessage>> Messages;
	CollectProblems(Messages);
	FMessageLog(TEXT("MapCheck")).AddMessages(Messages);
}

void AAgPatrolRoute::SnapPointsToGround()
{
	bool bMoved = false;
	for (int32 Index = 0; Index < GetPointCount(); ++Index)
	{
		const FVector Point = GetPointLocation(Index);
		const TOptional<FVector> Ground = AgPatrolRoutePrivate::FindGround(*GetWorld(), Point);
		if (Ground && !Ground->Equals(Point, UE_KINDA_SMALL_NUMBER))
		{
			Spline->SetLocationAtSplinePoint(Index, *Ground, ESplineCoordinateSpace::World, false);
			bMoved = true;
		}
	}
	if (bMoved)
	{
		Spline->UpdateSpline();
		Spline->MarkRenderStateDirty();
	}
}

void AAgPatrolRoute::CollectProblems(TArray<TSharedRef<FTokenizedMessage>>& OutMessages) const
{
	UWorld& World = *GetWorld();
	const int32 PointCount = GetPointCount();
	for (int32 Index = 0; Index < PointCount; ++Index)
	{
		if (!AgPatrolRoutePrivate::FindGround(World, GetPointLocation(Index)))
		{
			OutMessages.Add(MakeWarning(*this, FString::Printf(TEXT("Patrol point %d has no floor below it."), Index + 1)));
		}
	}
	if (PointCount < 2)
	{
		return;
	}
	for (int32 Index = 0; Index < PointCount; ++Index)
	{
		const int32 NextIndex = (Index + 1) % PointCount;
		TArray<FVector> Path;
		if (FindWalkPath(World, GetPointLocation(Index), GetPointLocation(NextIndex), Path) == EAgPatrolPathResult::Unreachable)
		{
			OutMessages.Add(MakeWarning(*this, FString::Printf(TEXT("Cannot walk from patrol point %d to %d."), Index + 1, NextIndex + 1)));
		}
	}
}
#endif
