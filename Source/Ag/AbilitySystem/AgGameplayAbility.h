// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Data/AgAbilityData.h"
#include "Data/AgTaskParams.h"
#include "Engine/EngineBaseTypes.h"
#include "StructUtils/InstancedStruct.h"
#include "AgGameplayAbility.generated.h"

class UAbilityTask;
class UAnimMontage;
class UGameplayEffect;
struct FAnimMontageInstance;
enum class EAgEffect : uint8;

/** 재생 중인 시간표 항목 하나. 재생을 시작할 때 시간표에서 복사하고 시각을 몽타주 시각으로 바꿉니다. */
USTRUCT()
struct FAgTimelineEntry
{
	GENERATED_BODY()

	/** 항목의 Params. 컷신 Sequence 같은 에셋 참조를 GC가 봅니다. */
	UPROPERTY()
	FInstancedStruct Params;

	/** 켜진 구간이 시작한 Task. 구간이 끝나거나 재생이 바뀌면 Runner가 끝냅니다. */
	UPROPERTY()
	TObjectPtr<UAbilityTask> Task;

	/** 구간의 시작이나 지점의 시각(몽타주 시각) */
	float Start = 0.f;

	/** 구간의 끝(몽타주 시각) */
	float End = 0.f;
	bool bWindow = false;
	bool bActive = false;
};

/**
 * 모든 행동의 Runner(구현 계획서 '어빌리티와 AbilityTask'의 'Runner'). 행동 ID마다 독립 GAS 스펙으로 부여되고, 행동
 * 정의의 태그 조건·트리거·비용·쿨다운·재발동을 엔진 어빌리티 기능에 연결합니다. 발동하면 대상을 정하고 커밋한 뒤 발동 Task를
 * 시작하고, 몽타주를 재생 전에 온 전환 요청의 섹션(없으면 처음)부터 재생해 시간표의 시각에 맞춰 Task를 시작하고 끝냅니다. Task
 * 사이의 순서는 정하지 않습니다. Task가 요청한 전환만 적용하며, Task 종류·행동 ID·몽타주 ID로 분기하지 않습니다. 행동 정의의
 * End가 정한 때 끝납니다.
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
	virtual void GetCooldownTimeRemainingAndDuration(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, float& TimeRemaining, float& CooldownDuration) const override;

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

	/**
	 * 행동 대상: Targeting 결과의 첫 후보이고, Targeting이 없고 이벤트로 발동했으면 그 이벤트의 Instigator입니다. 실행 중에만 정하고
	 * 끝날 때 비우므로, 없거나 실행 중이 아니면 null입니다.
	 */
	AActor* GetActionTarget() const
	{
		return ActionTarget.Get();
	}

	/** Targeting으로 정한 행동 대상. 캐릭터가 이 대상에게 최소 간격을 지킵니다. 없으면 null입니다. */
	AActor* GetTargetingTarget() const
	{
		return ExecutionDefinition.bHasTargeting ? ActionTarget.Get() : nullptr;
	}

	/** 대상과 맞출 간격(cm): Targeting의 StopDistance이고, 없으면 캐릭터의 StopDistance입니다. */
	float GetStopDistance() const
	{
		return StopDistance;
	}

	/**
	 * Task의 전환 요청. BlendTime이 없으면 같은 재생 안에서 그 섹션으로 옮기고, 있으면 그 섹션부터 그 블렌드로 다시 재생합니다.
	 * InstanceId(Task를 만든 재생의 몽타주 인스턴스 ID)가 지금 재생이 아니면 무시합니다. 재생 전(발동 Task를 시작하는 동안)에
	 * InstanceId 없이 온 요청은 시작 섹션으로 기억하고, 여럿이면 먼저 온 하나를 씁니다.
	 */
	void RequestTransition(const FAgTransition& Transition, int32 InstanceId);

protected:
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual bool CheckCost(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCost(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual bool CheckCooldown(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCooldown(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo) const override;

private:
	/** 시간표 판정을 부르는 까닭 */
	enum class EAgTimelineMode : uint8
	{
		/** 재생이 진행했습니다. 지나간 구간 경계와 지점을 실행합니다. */
		Advance,

		/** 재생 시작이나 섹션 이동으로 위치가 정해졌습니다. 그 위치로 구간을 맞추고 그 위치의 지점도 실행합니다. */
		Jump
	};

	/**
	 * 시간표 판정 틱. 부여부터 제거까지 레벨에 등록하고 시간표가 있는 동안만 켭니다. 몽타주가 진행된 뒤(캐릭터 무브먼트와 메시
	 * 다음) 같은 프레임의 공격 판정(TG_PostPhysics)보다 먼저 돕니다.
	 */
	struct FAgTimelineTickFunction : public FTickFunction
	{
		UAgGameplayAbility* Target = nullptr;
		virtual void ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) override;
		virtual FString DiagnosticMessage() override;
	};

	/** 행동이 실행 중이고 InstanceId가 이 행동의 지금 재생입니다. */
	bool OwnsMontage(int32 InstanceId) const
	{
		return IsActive() && InstanceId != INDEX_NONE && MontageInstanceId == InstanceId;
	}

	const FAgAbilityData* FindDefinition(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const;
	FGameplayEffectSpecHandle MakeEffectSpec(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const UGameplayEffect* Effect) const;

	/** Data의 Targeting을 평가해 첫 후보를 찾습니다. Targeting이 없거나 후보가 없으면 null입니다. */
	AActor* FindTargetingTarget(const FAgAbilityData& Data, const FGameplayAbilityActorInfo* ActorInfo) const;

	/** 이벤트로 발동했으면 그 이벤트가 행동 정의의 GameplayEvent 트리거입니다. */
	bool IsEventTrigger(const FGameplayEventData* TriggerEventData) const;

	void Finish(bool bCancelled);

	/**
	 * 행동의 몽타주를 Section부터(None이면 처음부터) 재생합니다. 실행 중이고 몽타주가 있을 때(발동, 전환) 부릅니다. BlendTime이
	 * 음수면 몽타주의 블렌드 인입니다. 이전 재생의 구간을 끝내고, 재생하면 그 위치의 시간표를 판정합니다. 재생하지 못하면 취소로
	 * 끝납니다.
	 */
	void PlayMontage(FName Section, float BlendTime);

	/** 같은 재생 안에서 섹션을 옮기고 그 위치로 판정을 맞춥니다. */
	void JumpToSection(FName Section);

	/** 지금 재생의 콜백: 다른 몽타주에 끊기면 블렌드 아웃이 시작될 때, 아니면 끝날 때 재생을 마칩니다. */
	void HandleMontageBlendingOut(UAnimMontage* EndedMontage, bool bInterrupted, int32 InstanceId);
	void HandleMontageEnded(UAnimMontage* EndedMontage, bool bInterrupted, int32 InstanceId);

	/** 지금 재생을 마칩니다: 판정을 멈추고, End가 Montage인 행동이면 끝냅니다(끊겼으면 취소). */
	void FinishPlayback(bool bInterrupted);

	/** 재생을 시작한 직후 그 몽타주의 시간표를 복사하고, 시작 위치에 걸친 구간과 그 위치의 지점을 실행합니다. */
	void BeginTimeline(const FAnimMontageInstance& Instance);

	void TickTimeline();
	void EvaluateTimeline(float NewPosition, EAgTimelineMode Mode);

	/** 재생이 끝났거나 바뀌었습니다. 켜진 구간을 끝내고 판정을 멈춥니다. */
	void EndTimeline();

	/** 판정을 멈추고 시간표를 버립니다. 켜진 구간의 Task는 끝내지 않습니다(EndTimeline이나 엔진이 끝냄). */
	void StopTimeline();
	void StartTimelineWindow(int32 Entry);
	void EndTimelineWindow(int32 Entry);
	void RunTimelinePoint(int32 Entry);

	UPROPERTY()
	FAgAbilityData GrantedDefinition;

	/** 발동할 때 복사한 정의. 데이터를 다시 읽어도 이 실행이 끝날 때까지 같은 값을 씁니다. */
	UPROPERTY()
	FAgAbilityData ExecutionDefinition;

	TWeakObjectPtr<AActor> ActionTarget;
	float StopDistance = 0.f;

	/** 이 행동의 지금 재생. 시간표도 이 재생을 판정합니다. 없으면 INDEX_NONE입니다. */
	int32 MontageInstanceId = INDEX_NONE;

	/** 재생 전에 발동 Task가 보낸 전환 요청. 첫 재생을 이 섹션(없으면 처음)부터 합니다. */
	FAgTransition StartTransition;

	/** 지금 재생의 시간표. 비어 있으면 판정하지 않습니다. */
	UPROPERTY()
	TArray<FAgTimelineEntry> TimelineEntries;

	/** 앞 판정의 재생 위치 */
	float TimelinePosition = 0.f;
	FAgTimelineTickFunction TimelineTick;
};
