// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "AgAbilityData.generated.h"

class FJsonObject;
class UAgCharacterData;
class UGameplayEffect;
struct FAgMontageTimeline;
struct FAgProjectileDefinition;
struct FGameplayEffectSpec;

UENUM()
enum class EAgResource : uint8
{
	SP,
	MP,
	UP
};

/** 자원 효과(Effects.json의 Resource) 스펙의 SetByCaller 값 */
namespace AgResource
{
	/** 세 자원의 변화량을 모두 넣습니다. 넣지 않은 값은 엔진이 오류로 남기므로, 바꾸지 않는 자원도 0을 넣습니다. */
	void SetAmounts(FGameplayEffectSpec& Spec, float SP, float MP, float UP);

	/** 자원 하나의 변화량을 넣고 다른 자원은 0입니다. */
	void SetAmount(FGameplayEffectSpec& Spec, EAgResource Resource, float Amount);
}

/** 행동의 비용 */
USTRUCT()
struct FAgResourceCostParams
{
	GENERATED_BODY()

	UPROPERTY()
	EAgResource Resource = EAgResource::SP;

	UPROPERTY()
	float Amount = 0.f;

	/** 자원 효과 스펙에 이 비용을 음수로 넣습니다. */
	void AddToSpec(FGameplayEffectSpec& Spec) const;
};

/** 엔진 트리거(FAbilityTriggerData)의 출처 */
UENUM()
enum class EAgTriggerSource : uint8
{
	/** 그 이벤트를 받으면 발동하고, 이벤트가 실행 정보로 옵니다. */
	GameplayEvent,

	/** 태그가 0에서 1이 될 때 발동하고, 0이 되면 엔진이 취소합니다. */
	OwnedTagPresent
};

USTRUCT()
struct FAgAbilityTrigger
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag Tag;

	UPROPERTY()
	EAgTriggerSource Source = EAgTriggerSource::GameplayEvent;
};

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

	/** 스펙의 DynamicAbilityTriggers에 넣는 엔진 트리거 */
	UPROPERTY()
	TArray<FAgAbilityTrigger> Triggers;

	UPROPERTY()
	bool bAllowRetrigger = false;
};

/** 행동 대상을 정하는 타겟팅. 후보를 모으고 거르고 정렬하는 절차는 프리셋에 있고, 범위와 대상의 태그 조건은 요청에 실어 보냅니다. */
USTRUCT()
struct FAgAbilityTargeting
{
	GENERATED_BODY()

	/** TargetingPresets.json의 프리셋 ID */
	UPROPERTY()
	FName Preset;

	/** 캡슐 중심 사이 수평 거리(cm)의 상한. bHasRange가 거짓이면 제한 없음입니다. */
	UPROPERTY()
	float Range = 0.f;

	/** 대상이 가져야 할 태그 */
	UPROPERTY()
	FGameplayTagContainer TargetRequiredTags;

	/** 대상이 가지면 안 되는 태그 */
	UPROPERTY()
	FGameplayTagContainer TargetBlockedTags;

	/** 대상이 없으면 발동하지 않습니다. */
	UPROPERTY()
	bool bRequired = false;

	/**
	 * 대상과 맞출 간격(cm). bHasStopDistance가 거짓이면 캐릭터의 StopDistance입니다. 추적 구간의 접근, Launch의 착지, 행동 내내의
	 * 최소 간격이 함께 씁니다.
	 */
	UPROPERTY()
	float StopDistance = 0.f;

	bool bHasRange = false;
	bool bHasStopDistance = false;
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
};

/** 행동이 끝나는 때 */
UENUM()
enum class EAgAbilityEnd : uint8
{
	/** 행동의 재생이 끝날 때. 완료면 정상 종료, 다른 몽타주에 끊기면 취소입니다. */
	Montage,

	/** OwnedTagPresent 트리거 태그가 사라질 때(엔진이 취소) */
	TriggerTag
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
	FAgAbilityTargeting Targeting;

	UPROPERTY()
	FAgAbilityMontage Montage;

	UPROPERTY()
	EAgAbilityEnd End = EAgAbilityEnd::Montage;

	/** 발동 Task 배열. 항목마다 Task별 Params(FAgTaskParams를 상속한 구조체)입니다. */
	UPROPERTY()
	TArray<FInstancedStruct> Tasks;

	UPROPERTY()
	FAgPatternSelection PatternSelection;
	bool bHasCost = false;
	bool bHasTargeting = false;
	bool bHasMontage = false;
	bool bHasPatternSelection = false;

	bool HasSameGrantSettings(const FAgAbilityData& Other) const;

	/** 이벤트로 발동하면 참입니다. 그 이벤트의 Instigator가 행동 대상이 될 수 있습니다. */
	bool HasEventTrigger() const;
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
	/** 고정 스키마와 Task별 Params를 읽고 정의 안의 규칙(종료와 트리거, 몽타주)을 검사합니다. 실패 시 OutAbilities를 변경하지 않습니다. */
	bool Parse(const FJsonObject& Json, TMap<FName, FAgAbilityData>& OutAbilities, TArray<FString>& Errors);

	/**
	 * 캐릭터의 어빌리티·효과·몽타주 ID, 사망 모션, 입력 계층이 찾는 행동 태그와, 그 어빌리티들의 행동 대상, 속도를 덮어쓰는 Task의
	 * 종류, 발동 Task의 규칙(ValidateLinks)을 그 캐릭터의 몽타주로 검사합니다. 오류에는 Path를 앞에 붙입니다.
	 */
	void ValidateCharacter(const UAgCharacterData& Data, const TMap<FName, FAgAbilityData>& Abilities, const TMap<FName, TObjectPtr<UGameplayEffect>>& Effects, const TMap<FName, FAgMontageTimeline>& Timelines, const FString& Path, TArray<FString>& Errors);

	/** 발동 Task가 가리키는 효과와 투사체가 있는지 검사합니다. 읽지 못한 파일(null)은 검사하지 않습니다. */
	void ValidateReferences(const TMap<FName, FAgAbilityData>& Abilities, const TMap<FName, TObjectPtr<UGameplayEffect>>* Effects, const TMap<FName, FAgProjectileDefinition>* Projectiles, TArray<FString>& Errors);
}
