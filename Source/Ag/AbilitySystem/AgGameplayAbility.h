// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Data/AgAbilityData.h"
#include "Data/AgProjectileData.h"
#include "Engine/EngineBaseTypes.h"
#include "StructUtils/InstancedStruct.h"
#include "AgGameplayAbility.generated.h"

class AAgCharacterBase;
class UAbilityTask;
class UAgRootMotionModifier_MotionWarping;
class UAnimMontage;
class UGameplayEffect;
class UMotionWarpingComponent;
struct FAgTrackParams;
struct FAgTransition;
struct FAnimMontageInstance;
enum class EAgEffect : uint8;

/** 시간표 구간 하나의 실행. 재생의 몽타주 인스턴스와 그 재생의 항목 번호로 구분합니다. */
USTRUCT()
struct FAgAbilityWindowExecution
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UAbilityTask> Task;

	int32 MontageInstanceId = INDEX_NONE;
	int32 Entry = INDEX_NONE;
};

/** 재생 중인 시간표 항목 하나. 재생을 시작할 때 시간표에서 복사하고 시각을 몽타주 시각으로 바꿉니다. */
USTRUCT()
struct FAgTimelineEntry
{
	GENERATED_BODY()

	/** 항목의 Params. 컷신 Sequence 같은 에셋 참조를 GC가 봅니다. */
	UPROPERTY()
	FInstancedStruct Params;

	FName Task;

	/** 구간의 시작이나 지점의 시각(몽타주 시각) */
	float Start = 0.f;

	/** 구간의 끝(몽타주 시각) */
	float End = 0.f;
	bool bWindow = false;
	bool bActive = false;

	/** 몽타주 루트 모션 없이 Runner가 직접 돌리는 추적 구간의 대상. 구간에 처음 든 판정에서 정합니다. */
	TWeakObjectPtr<AActor> TrackTarget;
	bool bTrackStarted = false;
	bool bTrackFacingOnly = false;
	bool bTrackStopped = false;
};

/**
 * 모든 행동의 Runner(구현 계획서 '어빌리티와 AbilityTask'의 'Runner'). 행동 ID마다 독립 GAS 스펙으로 부여되고, 행동
 * 정의의 태그 조건·트리거·비용·쿨다운·재발동을 엔진 어빌리티 기능에 연결합니다. 발동하면 대상을 정하고 커밋한 뒤 몽타주를
 * 처음부터 재생해 시간표를 판정하고, 발동 Task를 배열 순서대로 시작합니다. Task가 요청한 전환만 적용하며, Task 종류·행동 ID·몽타주
 * ID로 분기하지 않습니다. 행동 정의의 End가 정한 때 끝납니다.
 */
UCLASS()
class UAgGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UAgGameplayAbility();
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual bool CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual bool DoesAbilitySatisfyTagRequirements(const UAbilitySystemComponent& ASC, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void GetCooldownTimeRemainingAndDuration(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, float& TimeRemaining, float& Duration) const override;

	/** 행동의 비용. 실행 중이면 실행한 정의의 값입니다. */
	FAgResourceCostParams GetCost() const;

	/**
	 * 엔진 MakeOutgoingGameplayEffectSpec이 GE 클래스로 하는 일(어빌리티 컨텍스트, 어빌리티 태그, 스펙의 SetByCaller)을
	 * Effects.json의 효과로 합니다. 효과가 없으면 빈 핸들입니다.
	 */
	FGameplayEffectSpecHandle MakeEffectSpec(EAgEffect Effect) const;
	FGameplayEffectSpecHandle MakeEffectSpec(const UGameplayEffect& Effect) const;

	/** 로그와 오류에 쓰는 행동 ID입니다. 이 값으로 분기하지 않습니다. */
	FName GetAbilityId() const
	{
		return GrantedDefinition.Id;
	}

	const FGameplayTagContainer& GetDefinitionTags() const
	{
		return GrantedDefinition.Tags;
	}

	/** 행동 대상: Targeting 결과의 첫 후보이고, Targeting이 없고 이벤트로 발동했으면 그 이벤트의 Instigator입니다. 없으면 null입니다. */
	AActor* GetActionTarget() const;

	/** Targeting으로 정한 행동 대상. 캐릭터가 이 대상에게 최소 간격을 지킵니다. 없으면 null입니다. */
	AActor* GetTargetingTarget() const;

	/** 대상과 맞출 간격(cm): Targeting의 StopDistance이고, 없으면 캐릭터의 StopDistance입니다. */
	float GetStopDistance() const
	{
		return StopDistance;
	}

	/**
	 * Task의 전환 요청. BlendTime이 없으면 같은 재생 안에서 그 섹션으로 옮기고, 있으면 그 섹션부터 그 블렌드로 다시 재생합니다.
	 * MontageInstanceId가 지금 재생이 아니면 무시합니다.
	 */
	void RequestTransition(const FAgTransition& Transition, int32 MontageInstanceId);

	/** 이 행동의 몽타주에서 섹션의 길이(몽타주 시간). 섹션이 없으면 0입니다. */
	float GetSectionLength(FName Section) const;

	/** 행동이 실행 중이고 InstanceId가 이 행동의 지금 재생입니다. */
	bool OwnsMontage(int32 InstanceId) const
	{
		return IsActive() && !bEnding && InstanceId != INDEX_NONE && MontageInstanceId == InstanceId;
	}

	/**
	 * 추적 구간이 향할 대상과 맞출 간격. 행동 대상이 없거나 구간의 TargetingRange 밖이면 캐릭터의 대체 대상이고, bFacingOnly면
	 * 다가가지 않고 돌기만 합니다. 그 재생의 주인이 아니면 거짓입니다.
	 */
	bool PrepareTrackTarget(int32 InstanceId, const FAgTrackParams& Params, AActor*& Target, float& Gap, bool& bFacingOnly) const;
	void RegisterWarp(UAgRootMotionModifier_MotionWarping* Modifier);

	/** 이 행동을 시작할 때 복사한 투사체 정의. 없으면 null입니다. */
	const FAgProjectileDefinition* FindProjectile(FName ProjectileId) const
	{
		return ExecutionProjectiles.Find(ProjectileId);
	}

protected:
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual bool CheckCost(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* RelevantTags = nullptr) const override;
	virtual void ApplyCost(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual bool CheckCooldown(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* RelevantTags = nullptr) const override;
	virtual void ApplyCooldown(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo) const override;

private:
	/** 시간표 판정을 부르는 까닭. 재생 시작과 섹션 이동에서는 그 위치의 지점도 실행합니다. */
	enum class ETimelineMode : uint8
	{
		Start,
		Advance,
		Jump
	};

	/** 시간표 판정 틱. 몽타주가 진행된 뒤(캐릭터 무브먼트와 메시 다음) 같은 프레임의 공격 판정(TG_PostPhysics)보다 먼저 돕니다. */
	struct FAgTimelineTickFunction : public FTickFunction
	{
		UAgGameplayAbility* Target = nullptr;
		virtual void ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) override;
		virtual FString DiagnosticMessage() override;
	};

	AAgCharacterBase* GetAgCharacter() const;
	const FAgAbilityData* FindDefinition(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const;
	FGameplayEffectSpecHandle MakeEffectSpec(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const UGameplayEffect* Effect) const;

	/** Data의 Targeting을 평가해 첫 후보를 찾습니다. Targeting이 없거나 후보가 없으면 null입니다. */
	AActor* FindTargetingTarget(const FAgAbilityData& Data, const FGameplayAbilityActorInfo* ActorInfo) const;

	/** 이벤트로 발동했으면 그 이벤트가 행동 정의의 GameplayEvent 트리거입니다. */
	bool IsEventTrigger(const FGameplayEventData* TriggerEventData) const;

	void Finish(bool bCancelled);

	/**
	 * 행동의 몽타주를 Section부터(None이면 처음부터) 재생합니다. BlendTime이 음수면 몽타주의 블렌드 인입니다. 이전 재생의 구간을
	 * 끝내고, 재생하면 그 위치의 시간표를 판정합니다. 재생하지 못하면 취소로 끝나고 거짓입니다.
	 */
	bool PlayMontage(FName Section, float BlendTime);

	/** 같은 재생 안에서 섹션을 옮기고 그 위치로 판정을 맞춥니다. */
	void JumpToSection(FName Section);

	/** 지금 재생의 끝 처리: 다른 몽타주에 끊기면 취소, 끝나면 End가 Montage인 행동을 정상 종료합니다. */
	void HandleMontageBlendingOut(UAnimMontage* EndedMontage, bool bInterrupted, int32 InstanceId);
	void HandleMontageEnded(UAnimMontage* EndedMontage, bool bInterrupted, int32 InstanceId);

	/** 재생을 시작한 직후 그 몽타주의 시간표를 복사하고, 시작 위치에 걸친 구간과 그 위치의 지점을 실행합니다. */
	void BeginTimeline(const FAnimMontageInstance& Instance);

	/** 섹션을 옮긴 직후 판정 위치를 맞춥니다. 건너뛴 지점은 실행하지 않고, 옮긴 위치의 지점은 실행합니다. */
	void ResyncTimeline();
	void TickTimeline(float DeltaTime);
	void EvaluateTimeline(float NewPosition, ETimelineMode Mode);

	/** 재생이 끝났거나 바뀌었습니다. 켜진 구간을 끝내고 판정을 멈춥니다. */
	void EndTimeline();

	/** 판정을 멈추고 시간표를 버립니다. 구간의 실행은 부르는 쪽이 정리합니다. */
	void StopTimeline();
	void StartTimelineWindow(int32 Entry);
	void EndTimelineWindow(int32 InstanceId, int32 Entry);
	void RunTimelinePoint(int32 Entry);
	void EndWindows();
	void ClearWarps();

	/** 몽타주 루트 모션이 없는 위치의 추적 구간: 캐릭터를 직접 돌리고 다가가지 않습니다(이동은 루트 모션 소스가 맡음). */
	void TurnTracks(const FAnimMontageInstance& Instance, float PreviousPosition, float DeltaTime);

	/** 추적 구간을 엔진이 워핑 노티파이를 찾는 때(워핑 컴포넌트의 OnPreUpdate)에 같은 조건으로 엔진 Motion Warping에 등록합니다. */
	UFUNCTION()
	void HandleWarpingPreUpdate(UMotionWarpingComponent* Warping);

	UPROPERTY()
	FAgAbilityData GrantedDefinition;

	/** 발동할 때 복사한 정의. 데이터를 다시 읽어도 이 실행이 끝날 때까지 같은 값을 씁니다. */
	UPROPERTY()
	FAgAbilityData ExecutionDefinition;

	UPROPERTY()
	TObjectPtr<UAnimMontage> Montage;

	UPROPERTY()
	TMap<FName, FAgProjectileDefinition> ExecutionProjectiles;

	TWeakObjectPtr<AActor> ActionTarget;
	bool bTargetFromTargeting = false;
	float StopDistance = 0.f;

	/** 몽타주 루트 모션의 이동 배율(Montage의 RootMotionDistance) */
	float RootMotionScale = 1.f;

	/** 이 행동의 지금 재생. 없으면 INDEX_NONE입니다. */
	int32 MontageInstanceId = INDEX_NONE;

	UPROPERTY()
	TArray<FAgAbilityWindowExecution> Windows;

	/** 재생 중인 몽타주의 시간표 */
	UPROPERTY()
	TArray<FAgTimelineEntry> TimelineEntries;

	/** 시간표를 판정하는 재생. 없으면 INDEX_NONE입니다. */
	int32 TimelineInstanceId = INDEX_NONE;

	/** 앞 판정의 재생 위치 */
	float TimelinePosition = 0.f;
	FAgTimelineTickFunction TimelineTick;

	UPROPERTY()
	TArray<TObjectPtr<UAgRootMotionModifier_MotionWarping>> Warps;

	bool bEnding = false;
};
