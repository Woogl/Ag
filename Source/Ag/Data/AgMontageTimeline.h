// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"
#include "AgMontageTimeline.generated.h"

class FJsonObject;
class UAnimMontage;
class UGameplayEffect;
struct FAgProjectileDefinition;

/** 시간표 항목 하나: 구간이나 지점과 그 Task의 Params. 시각은 항목을 둔 섹션의 시작 기준(초)입니다. */
USTRUCT()
struct FAgTimelineItem
{
	GENERATED_BODY()

	/** Task 이름. 오류와 로그에 씁니다. */
	UPROPERTY()
	FName Task;

	/** Task별 Params(FAgTaskParams를 상속한 구조체) */
	UPROPERTY()
	FInstancedStruct Params;

	/** 구간이면 Start·End가 있고, 지점이면 Start가 그 시각(Time)입니다. */
	UPROPERTY()
	bool bWindow = false;

	UPROPERTY()
	float Start = 0.f;

	/** 구간이 끝나는 섹션. None이면 항목을 둔 섹션입니다. */
	UPROPERTY()
	FName EndSection;

	/** 구간이 끝나는 시각(끝나는 섹션의 시작 기준). bEndAtSectionEnd면 쓰지 않습니다. */
	UPROPERTY()
	float End = 0.f;

	/** End의 시각을 적지 않아 끝나는 섹션의 끝까지인 구간 */
	UPROPERTY()
	bool bEndAtSectionEnd = false;
};

/** 섹션 하나의 항목. 시각 순서로 실행하고, 같은 시각의 항목 사이에는 순서가 없습니다. */
USTRUCT()
struct FAgTimelineSection
{
	GENERATED_BODY()

	UPROPERTY()
	FName Name;

	UPROPERTY()
	TArray<FAgTimelineItem> Items;
};

/** 몽타주 하나의 시간표. 섹션은 JSON에 적은 순서입니다. */
USTRUCT()
struct FAgMontageTimeline
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FAgTimelineSection> Sections;
};

/** MontageTimelines.json의 값. 키는 몽타주 에셋 경로입니다. */
UCLASS()
class UAgMontageTimelineData : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, VisibleAnywhere, Category = "Timeline")
	TMap<FName, FAgMontageTimeline> Timelines;

	/** 몽타주의 시간표. 없으면 null입니다. 데이터를 다시 읽으면 바뀌므로 포인터는 그 함수 안에서만 씁니다. */
	const FAgMontageTimeline* Find(const UAnimMontage& Montage) const;
};

namespace AgMontageTimeline
{
	/** 몽타주 → 섹션 → 항목 배열을 읽습니다. 실패하면 Out을 바꾸지 않습니다. */
	bool Parse(const FJsonObject& Json, TMap<FName, FAgMontageTimeline>& Out, TArray<FString>& Errors);

	/** 항목의 시각을 몽타주 시각(재생 위치)으로 바꿉니다. 지점은 OutStart만 씁니다. 섹션이 몽타주에 없으면 거짓입니다. */
	bool ResolveTimes(const UAnimMontage& Montage, FName Section, const FAgTimelineItem& Item, float& OutStart, float& OutEnd);

	/** Start부터 End까지(몽타주 시각) 몽타주가 루트 모션을 줍니다. 루트 모션 끄기 노티파이가 그 구간을 덮으면 주지 않습니다. */
	bool GivesRootMotion(const UAnimMontage& Montage, float Start, float End);

	/** Start부터 End까지(몽타주 시각)가 반복 섹션과 겹칩니다. */
	bool OverlapsLoop(const UAnimMontage& Montage, float Start, float End);

	/** 항목이 가리키는 효과와 투사체가 있는지 검사합니다. 읽지 못한 파일(null)은 검사하지 않습니다. */
	void ValidateReferences(const TMap<FName, FAgMontageTimeline>& Timelines, const TMap<FName, TObjectPtr<UGameplayEffect>>* Effects, const TMap<FName, FAgProjectileDefinition>* Projectiles, TArray<FString>& Errors);

#if WITH_EDITOR
	/**
	 * 몽타주 에셋과 대조합니다: 섹션과 길이, 전환할 섹션과 전환 지점(섹션 끝 앞, 같은 시각의 다른 항목 없음), 반복 섹션(지점,
	 * 기다리는 구간), 몽타주에 남긴 노티파이와 공격 예고의 시각. 항목마다 그 Task의 규칙(ValidateLinks)도 검사합니다. Effects는
	 * 효과 파일을 읽지 못했으면 null입니다.
	 */
	void ValidateAssets(const TMap<FName, FAgMontageTimeline>& Timelines, const TMap<FName, TObjectPtr<UGameplayEffect>>* Effects, TArray<FString>& Errors);
#endif
}
