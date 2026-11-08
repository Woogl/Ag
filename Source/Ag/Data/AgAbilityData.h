// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTriggerType.h"
#include "AttributeSet.h"
#include "Data/AgTaskParams.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "AgAbilityData.generated.h"

class UAbilityTask;
class UAgGameplayAbility;
class UAnimMontage;
class UTexture2D;
struct FGameplayEffectSpec;

USTRUCT()
struct FAgResourceCost
{
	GENERATED_BODY()

	UPROPERTY()
	float SP = 0.f;

	UPROPERTY()
	float MP = 0.f;

	UPROPERTY()
	float UP = 0.f;

	bool IsFree() const;

	void AddToSpec(FGameplayEffectSpec& Spec) const;
};

/** 이만큼 있어야 발동하지만 발동할 때 치르지는 않는 어트리뷰트입니다. */
USTRUCT()
struct FAgAttributeCondition
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayAttribute Attribute;

	UPROPERTY()
	float AtLeast = 0.f;
};

USTRUCT()
struct FAgTargetingRequest
{
	GENERATED_BODY()

	// 애님 블루프린트 기본값(대상 바라보기)에서도 고치므로 편집 가능하게 둡니다. JSON은 구조체 멤버를 플래그와 관계없이 읽습니다.
	UPROPERTY(EditAnywhere, meta = (DataId = "TargetingPresets"))
	FName Preset;

	UPROPERTY(EditAnywhere)
	float Range = 0.f;

	UPROPERTY(EditAnywhere)
	FGameplayTagContainer TargetRequiredTags;

	UPROPERTY(EditAnywhere)
	FGameplayTagContainer TargetBlockedTags;
};

USTRUCT()
struct FAgAbilityTargeting : public FAgTargetingRequest
{
	GENERATED_BODY()

	UPROPERTY()
	bool bRequired = false;

	UPROPERTY()
	float TargetSpacing = 0.f;
};

USTRUCT()
struct FAgPatternSelection
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<int32> Phases;

	UPROPERTY()
	FFloatInterval Distance = FFloatInterval(0.f, 0.f);

	UPROPERTY()
	float Weight = 0.f;

	UPROPERTY()
	float ResponseChance = 0.f;

	bool IsAvailable(float TargetDistance, int32 Phase) const;
	bool IsInPhase(int32 Phase) const;
};

UENUM()
enum class EAgAbilityEnd : uint8
{
	Tasks,
	TriggerTag,
	InputReleased
};

// 정면에서 시계 방향으로 45°씩이며, 값의 순서가 곧 각도입니다.
UENUM()
enum class EAgMoveDirection : uint8
{
	Forward,
	ForwardRight,
	Right,
	BackwardRight,
	Backward,
	BackwardLeft,
	Left,
	ForwardLeft
};

USTRUCT()
struct FAgTimelineSection
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TInstancedStruct<FAgTaskParams>> Tasks;
};

USTRUCT()
struct FAgMontageStart
{
	GENERATED_BODY()

	/** 비어 있으면 몽타주의 첫 섹션입니다. */
	UPROPERTY()
	FName Section;

	UPROPERTY()
	FGameplayTagContainer RequiredTags;

	UPROPERTY()
	FGameplayTagContainer BlockedTags;

	UPROPERTY()
	TOptional<EAgMoveDirection> MoveDirection;

	/** 값이 있으면 어빌리티를 발동시킨 이벤트 태그가 이 태그와 맞을 때만 고릅니다. */
	UPROPERTY()
	FGameplayTag EventTag;
};

USTRUCT()
struct FAgAbilityMontage
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UAnimMontage> Asset;

	UPROPERTY()
	TArray<FAgMontageStart> StartSections;

	UPROPERTY()
	TMap<FName, FAgTimelineSection> Sections;

	UAbilityTask* MakeTask(UAgGameplayAbility& Ability) const;
};

USTRUCT()
struct FAgAbilityData
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTagContainer AbilityTags;

	UPROPERTY()
	FGameplayTag InputTag;

	UPROPERTY()
	TObjectPtr<UTexture2D> Icon;

	UPROPERTY()
	FGameplayTagContainer ActivationRequiredTags;

	UPROPERTY()
	FGameplayTagContainer ActivationBlockedTags;

	UPROPERTY()
	FGameplayTagContainer ActivationOwnedTags;

	UPROPERTY()
	FGameplayTagContainer BlockAbilitiesWithTag;

	UPROPERTY()
	FGameplayTagContainer CancelAbilitiesWithTag;

	UPROPERTY()
	TArray<FAbilityTriggerData> AbilityTriggers;

	UPROPERTY()
	bool bAllowRetrigger = false;

	UPROPERTY()
	FAgResourceCost Cost;

	UPROPERTY()
	TArray<FAgAttributeCondition> ActivationAttributes;

	UPROPERTY()
	float Cooldown = 0.f;

	UPROPERTY()
	int32 MaxCharges = 1;

	UPROPERTY()
	TOptional<FAgAbilityTargeting> Targeting;

	UPROPERTY()
	EAgAbilityEnd End = EAgAbilityEnd::Tasks;

	UPROPERTY()
	TArray<TInstancedStruct<FAgTaskParams>> Tasks;

	UPROPERTY()
	TOptional<FAgAbilityMontage> Montage;

	UPROPERTY()
	TObjectPtr<UAnimMontage> DeathMontage;

	UPROPERTY()
	TOptional<FAgPatternSelection> PatternSelection;

	bool HasSameGrantSettings(const FAgAbilityData& Other) const;
};

UCLASS()
class UAgAbilitySet : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, VisibleAnywhere, Category = "Abilities")
	TMap<FName, FAgAbilityData> Abilities;
};
