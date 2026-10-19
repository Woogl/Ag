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

USTRUCT()
struct FAgTimelineEntry
{
	GENERATED_BODY()

	UPROPERTY()
	FInstancedStruct Params;

	UPROPERTY()
	TObjectPtr<UAbilityTask> Task;

	float Start = 0.f;

	float End = 0.f;
	bool bWindow = false;
	bool bActive = false;
};

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

	FAgResourceCostParams GetCost() const;

	/** 엔진 MakeOutgoingGameplayEffectSpec은 GE 클래스를 받는데 Effects.json의 효과는 GE 객체이므로 직접 만듭니다. */
	FGameplayEffectSpecHandle MakeEffectSpec(EAgEffect Effect) const;
	FGameplayEffectSpecHandle MakeEffectSpec(const UGameplayEffect& Effect) const;

	FName GetAbilityId() const
	{
		return GrantedDefinition.Id;
	}

	const FGameplayTagContainer& GetDefinitionTags() const
	{
		return GrantedDefinition.Tags;
	}

	AActor* GetActionTarget() const
	{
		return ActionTarget.Get();
	}

	AActor* GetTargetingTarget() const
	{
		return ExecutionDefinition.bHasTargeting ? ActionTarget.Get() : nullptr;
	}

	float GetStopDistance() const
	{
		return StopDistance;
	}

	void RequestTransition(const FAgTransition& Transition, int32 InstanceId);

protected:
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual bool CheckCost(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCost(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual bool CheckCooldown(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCooldown(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo) const override;

private:
	enum class EAgTimelineMode : uint8
	{
		Advance,
		Jump
	};

	struct FAgTimelineTickFunction : public FTickFunction
	{
		UAgGameplayAbility* Target = nullptr;
		virtual void ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) override;
		virtual FString DiagnosticMessage() override;
	};

	bool OwnsMontage(int32 InstanceId) const
	{
		return IsActive() && InstanceId != INDEX_NONE && MontageInstanceId == InstanceId;
	}

	const FAgAbilityData* FindDefinition(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const;
	FGameplayEffectSpecHandle MakeEffectSpec(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const UGameplayEffect* Effect) const;

	AActor* FindTargetingTarget(const FAgAbilityData& Data, const FGameplayAbilityActorInfo* ActorInfo) const;

	bool IsEventTrigger(const FGameplayEventData* TriggerEventData) const;

	void Finish(bool bCancelled);

	void PlayMontage(FName Section, float BlendTime);

	void JumpToSection(FName Section);

	void HandleMontageBlendingOut(UAnimMontage* EndedMontage, bool bInterrupted, int32 InstanceId);
	void HandleMontageEnded(UAnimMontage* EndedMontage, bool bInterrupted, int32 InstanceId);

	void FinishPlayback(bool bInterrupted);

	void BeginTimeline(const FAnimMontageInstance& Instance);

	void TickTimeline();
	void EvaluateTimeline(float NewPosition, EAgTimelineMode Mode);

	void EndTimeline();

	void StopTimeline();
	void StartTimelineWindow(int32 Entry);
	void EndTimelineWindow(int32 Entry);
	void RunTimelinePoint(int32 Entry);

	UPROPERTY()
	FAgAbilityData GrantedDefinition;

	/** 데이터를 다시 읽어도 이 실행이 끝날 때까지 같은 값을 쓰도록 발동할 때 복사해 둡니다. */
	UPROPERTY()
	FAgAbilityData ExecutionDefinition;

	TWeakObjectPtr<AActor> ActionTarget;
	float StopDistance = 0.f;

	int32 MontageInstanceId = INDEX_NONE;

	FAgTransition StartTransition;

	UPROPERTY()
	TArray<FAgTimelineEntry> TimelineEntries;

	float TimelinePosition = 0.f;
	FAgTimelineTickFunction TimelineTick;
};
