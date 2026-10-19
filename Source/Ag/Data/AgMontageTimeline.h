// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"
#include "AgMontageTimeline.generated.h"

class FJsonObject;
class UAnimMontage;
class UGameplayEffect;
struct FAgProjectileDefinition;

USTRUCT()
struct FAgTimelineItem
{
	GENERATED_BODY()

	UPROPERTY()
	FName Task;

	UPROPERTY()
	FInstancedStruct Params;

	UPROPERTY()
	bool bWindow = false;

	UPROPERTY()
	float Start = 0.f;

	UPROPERTY()
	FName EndSection;

	UPROPERTY()
	float End = 0.f;

	UPROPERTY()
	bool bEndAtSectionEnd = false;
};

USTRUCT()
struct FAgTimelineSection
{
	GENERATED_BODY()

	UPROPERTY()
	FName Name;

	UPROPERTY()
	TArray<FAgTimelineItem> Items;
};

USTRUCT()
struct FAgMontageTimeline
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FAgTimelineSection> Sections;
};

UCLASS()
class UAgMontageTimelineData : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, VisibleAnywhere, Category = "Timeline")
	TMap<FName, FAgMontageTimeline> Timelines;

	/** 데이터를 다시 읽으면 바뀌므로 포인터는 그 함수 안에서만 씁니다. */
	const FAgMontageTimeline* Find(const UAnimMontage& Montage) const;
};

namespace AgMontageTimeline
{
	bool Parse(const FJsonObject& Json, TMap<FName, FAgMontageTimeline>& Out, TArray<FString>& Errors);

	bool ResolveTimes(const UAnimMontage& Montage, FName Section, const FAgTimelineItem& Item, float& OutStart, float& OutEnd);

	bool GivesRootMotion(const UAnimMontage& Montage, float Start, float End);

	bool OverlapsLoop(const UAnimMontage& Montage, float Start, float End);

	void ValidateReferences(const TMap<FName, FAgMontageTimeline>& Timelines, const TMap<FName, TObjectPtr<UGameplayEffect>>* Effects, const TMap<FName, FAgProjectileDefinition>* Projectiles, TArray<FString>& Errors);

#if WITH_EDITOR
	void ValidateAssets(const TMap<FName, FAgMontageTimeline>& Timelines, const TMap<FName, TObjectPtr<UGameplayEffect>>* Effects, TArray<FString>& Errors);
#endif
}
