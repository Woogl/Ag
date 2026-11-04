// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AgPatrolRoute.generated.h"

class FTokenizedMessage;
class UAgPatrolPreviewComponent;
class USplineComponent;

#if WITH_EDITOR
enum class EAgPatrolPathResult : uint8
{
	Walkable,
	Unreachable,
	Unknown
};
#endif

/** 적이 순서대로 도는 정찰 지점입니다. 점은 스플라인 점이며, 마지막 점 다음에는 첫 점으로 돌아갑니다. */
UCLASS()
class AAgPatrolRoute : public AActor
{
	GENERATED_BODY()

public:
	AAgPatrolRoute();

	int32 GetPointCount() const;

	FVector GetPointLocation(int32 Index) const;

	/** 수평 거리로 가장 가까운 점의 순서입니다. 점이 없으면 INDEX_NONE입니다. */
	int32 FindNearestPoint(const FVector& Location) const;

	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	/** 내비게이션으로 두 위치 사이의 걷는 길을 찾습니다. 걸을 수 있을 때만 OutPath를 채웁니다. */
	static EAgPatrolPathResult FindWalkPath(UWorld& World, const FVector& From, const FVector& To, TArray<FVector>& OutPath);

	static TSharedRef<FTokenizedMessage> MakeWarning(const UObject& Object, const FString& Text);

	/** 편집 중에 찾은 문제를 맵 검사 로그의 새 페이지에 남기고 알립니다. 문제가 없으면 아무것도 하지 않습니다. */
	static void ReportEditProblems(const TArray<TSharedRef<FTokenizedMessage>>& Messages);

	/** 점을 모두 월드 위치 목록으로 바꾸고 바닥에 붙인 뒤 경로를 확인합니다. 높이는 맞추지 않아도 됩니다. */
	UFUNCTION(BlueprintCallable, Category = "Patrol")
	void SetPointLocations(const TArray<FVector>& WorldLocations);

	virtual void PostEditMove(bool bFinished) override;
	virtual void CheckForErrors() override;
#endif

private:
#if WITH_EDITOR
	void SnapPointsToGround();
	void CollectProblems(TArray<TSharedRef<FTokenizedMessage>>& OutMessages) const;
#endif

	UPROPERTY(VisibleAnywhere, Category = "Patrol")
	TObjectPtr<USplineComponent> Spline;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UAgPatrolPreviewComponent> PatrolPreview;
#endif
};
