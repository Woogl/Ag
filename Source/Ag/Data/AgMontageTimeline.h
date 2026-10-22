// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/AgTaskParams.h"
#include "StructUtils/InstancedStruct.h"
#include "AgMontageTimeline.generated.h"

class UAnimMontage;
struct FAgTaskParams;

USTRUCT()
struct FAgTimelineSection
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TInstancedStruct<FAgTaskParams>> Tasks;
};

USTRUCT()
struct FAgMontageTimeline
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<FName, FAgTimelineSection> Sections;
};

UCLASS()
class UAgMontageTimelineData : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, VisibleAnywhere, Category = "Timeline")
	TMap<TObjectPtr<UAnimMontage>, FAgMontageTimeline> Timelines;

	/** 데이터를 다시 읽으면 바뀌므로 포인터는 그 함수 안에서만 씁니다. */
	const FAgMontageTimeline* Find(UAnimMontage* Montage) const;
};
