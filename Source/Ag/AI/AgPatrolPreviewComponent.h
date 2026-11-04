// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AI/AgPatrolRoute.h"
#include "Components/ActorComponent.h"
#include "AgPatrolPreviewComponent.generated.h"

#if WITH_EDITOR
struct FAgPatrolPreviewSegment
{
	/** 걸을 수 있으면 걷는 길의 점들이고, 아니면 두 끝점입니다. */
	TArray<FVector> Path;
	EAgPatrolPathResult Result = EAgPatrolPathResult::Unknown;
};
#endif

/** 정찰 경로나 적에 붙어, 편집 뷰포트에 그릴 정찰 경로를 모읍니다. 주인이 적이면 그 적의 정찰 경로를 씁니다. */
UCLASS(MinimalAPI)
class UAgPatrolPreviewComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAgPatrolPreviewComponent();

#if WITH_EDITOR
	/** 표시할 정찰 경로의 점 위치를 순서대로 모읍니다. 표시할 경로가 없으면 거짓입니다. */
	AG_API bool GetPreviewPoints(TArray<FVector>& OutPoints) const;

	/** 점 사이 구간과, 주인이 적이면 처음 자리에서 가장 가까운 점까지의 구간을 모읍니다. */
	AG_API void GetPreviewSegments(TArray<FAgPatrolPreviewSegment>& OutSegments) const;
#endif

private:
#if WITH_EDITOR
	const AAgPatrolRoute* GetRoute() const;
#endif
};
