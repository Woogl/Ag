// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/AgAbilityTaskParams.h"
#include "StructUtils/InstancedStruct.h"
#include "AgAbilityData.generated.h"

class FJsonObject;
class UAgCharacterData;
class UGameplayEffect;

USTRUCT()
struct FAgAbilityActivation
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTagContainer RequiredTags;

	UPROPERTY()
	FGameplayTagContainer BlockedTags;

	UPROPERTY()
	FGameplayTagContainer OwnedTags;

	UPROPERTY()
	FGameplayTagContainer BlockTags;

	UPROPERTY()
	FGameplayTagContainer CancelTags;

	UPROPERTY()
	FGameplayTagContainer TriggerEvents;

	UPROPERTY()
	bool bAllowRetrigger = false;
};

USTRUCT()
struct FAgPatternSelection
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<int32> Phases;

	UPROPERTY()
	float WeightNear = 0.f;

	UPROPERTY()
	float WeightMid = 0.f;

	UPROPERTY()
	float WeightFar = 0.f;

	UPROPERTY()
	float ResponseRange = 0.f;

	UPROPERTY()
	float ResponseChance = 0.f;
};

/** 행동이 재생하는 몽타주 */
USTRUCT()
struct FAgAbilityMontage
{
	GENERATED_BODY()

	/** 캐릭터 데이터 Montages의 ID */
	UPROPERTY()
	FName MontageId;

	/** 몽타주 전체 루트 모션의 수평 이동 거리(cm). 이 거리가 되도록 이동 배율을 정해 재생합니다. */
	UPROPERTY()
	float RootMotionDistance = 0.f;
	bool bHasRootMotionDistance = false;
};

/** 행동별 스펙 정의. 실행 중에는 Ability가 이 값과 에셋 참조를 복사하여 보유합니다. */
USTRUCT()
struct FAgAbilityData
{
	GENERATED_BODY()

	UPROPERTY()
	FName Id;

	UPROPERTY()
	FGameplayTagContainer Tags;

	UPROPERTY()
	float Cooldown = 0.f;

	UPROPERTY()
	FAgAbilityActivation Activation;

	UPROPERTY()
	FAgResourceCostParams Cost;

	UPROPERTY()
	FAgAbilityMontage Montage;

	/** 발동 Task 배열. 항목마다 Task별 Params(FAgTaskParams를 상속한 구조체)입니다. */
	UPROPERTY()
	TArray<FInstancedStruct> Tasks;

	UPROPERTY()
	FAgPatternSelection PatternSelection;
	bool bHasCost = false;
	bool bHasMontage = false;
	bool bHasPatternSelection = false;

	bool HasSameGrantSettings(const FAgAbilityData& Other) const;

	/** 발동 Task 배열에서 T인 첫 항목. 없으면 null입니다. 정의를 복사하거나 지우면 바뀌므로 포인터는 그 자리에서만 씁니다. */
	template <typename T> const T* FindTask() const
	{
		for (const FInstancedStruct& Task : Tasks)
		{
			if (const T* Params = Task.GetPtr<T>())
			{
				return Params;
			}
		}
		return nullptr;
	}

	/** 발동 Task 배열에서 T인 항목들(배열 순서) */
	template <typename T> TArray<const T*> FindTasks() const
	{
		TArray<const T*> Result;
		for (const FInstancedStruct& Task : Tasks)
		{
			if (const T* Params = Task.GetPtr<T>())
			{
				Result.Add(Params);
			}
		}
		return Result;
	}
};

/** Abilities.json의 값: 어빌리티 ID마다 정의(구현 계획서 'JSON 데이터'). 캐릭터는 어빌리티 목록에 ID를 적어 씁니다. */
UCLASS()
class UAgAbilitySet : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, VisibleAnywhere, Category = "Abilities")
	TMap<FName, FAgAbilityData> Abilities;
};

namespace AgAbilityData
{
	/** 고정 스키마와 Task별 Params를 읽고, C++ 사건이 요구하는 Task를 검사합니다. 실패 시 OutAbilities를 변경하지 않습니다. */
	bool Parse(const FJsonObject& Json, TMap<FName, FAgAbilityData>& OutAbilities, TArray<FString>& Errors);

	/** 캐릭터의 어빌리티·효과 ID와, 그 어빌리티들이 이 캐릭터에서 쓰는 몽타주를 검사합니다. 오류에는 Path를 앞에 붙입니다. */
	void ValidateCharacter(const UAgCharacterData& Data, const TMap<FName, FAgAbilityData>& Abilities,
	    const TMap<FName, TObjectPtr<UGameplayEffect>>& Effects, const FString& Path, TArray<FString>& Errors);

	TArray<FName> MontageIds(const UAgCharacterData& Data, const FAgAbilityData& Ability);
}
