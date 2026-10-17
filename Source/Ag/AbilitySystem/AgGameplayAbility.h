// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Data/AgAbilityData.h"
#include "Data/AgCharacterData.h"
#include "AgGameplayAbility.generated.h"

class AAgCharacterBase;
class AAgPlayerCharacter;
class UAbilityTask;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitDelay;
class UAgAbilityTask_HoldTag;
class UAgAbilityTask_Dash;
class UAgAbilityTask_TimeStop;
class UAgAbilityTask_LockOn;
class UAgRootMotionModifier_MotionWarping;
class UAgAnimNotify_SpawnProjectile;
struct FAnimMontageInstance;
struct FBranchingPointNotifyPayload;

USTRUCT()
struct FAgAbilityWindowExecution
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UAbilityTask> Task;
	FName Function;
	int32 MontageInstanceId = INDEX_NONE;
	int32 NotifyIndex = INDEX_NONE;
};

USTRUCT()
struct FAgAbilityMontageExecution
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UAnimMontage> Montage;
	TWeakObjectPtr<UAnimInstance> AnimInstance;
	FName MontageId;
	int32 InstanceId = INDEX_NONE;
	bool bBlocksMovement = false;
	bool bControlsFacing = false;
	TWeakObjectPtr<AAgCharacterBase> Target;
	float TargetPhaseStart = 0.f;
};

/** 모든 행동을 ID별 독립 GAS 스펙으로 부여합니다. 설정은 실행 요청과 구분합니다. */
UCLASS()
class UAgGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
#if WITH_DEV_AUTOMATION_TESTS
	friend class FAgAbilityExecutionTest;
#endif
	UAgGameplayAbility();
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual bool CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	    const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
	    FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual bool DoesAbilitySatisfyTagRequirements(const UAbilitySystemComponent& ASC, const FGameplayTagContainer* SourceTags = nullptr,
	    const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void GetCooldownTimeRemainingAndDuration(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	    float& TimeRemaining, float& Duration) const override;
	FAgResourceCostParams GetCost() const;
	AAgCharacterBase* GetLockOnTarget() const;
	FName GetAbilityId() const
	{
		return GrantedDefinition.Id;
	}

	const FGameplayTagContainer& GetDefinitionTags() const
	{
		return GrantedDefinition.Tags;
	}

	bool BlocksMovementInput() const
	{
		return IsActive() && Motion.Montage && Motion.bBlocksMovement;
	}

	bool OwnsMontage(int32 InstanceId) const
	{
		return IsActive() && Motion.InstanceId == InstanceId && InstanceId != INDEX_NONE;
	}

	void BeginNotifyTask(int32 InstanceId, int32 NotifyIndex, FName Task, FName ParamsId);
	void EndNotifyTask(int32 InstanceId, int32 NotifyIndex);
	void HandleMontageEvent(int32 InstanceId, FGameplayTag Event);
	void SpawnProjectile(int32 InstanceId, const UAgAnimNotify_SpawnProjectile& Notify);
	bool PrepareWarp(int32 InstanceId, float WindowStart, FName ParamsId, FAgMotionWarpingParams& Params, AActor*& Target, float& Gap,
	    bool& bFacingOnly);
	void RegisterWarp(UAgRootMotionModifier_MotionWarping* Modifier);
	static AAgCharacterBase* FindExecutionTarget(const AAgPlayerCharacter* Player);

protected:
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	    FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	    FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual bool CheckCost(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	    FGameplayTagContainer* RelevantTags = nullptr) const override;
	virtual void ApplyCost(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	    FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual bool CheckCooldown(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	    FGameplayTagContainer* RelevantTags = nullptr) const override;
	virtual void ApplyCooldown(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	    FGameplayAbilityActivationInfo ActivationInfo) const override;

private:
	AAgCharacterBase* GetAgCharacter() const;
	const FAgAbilityData* FindDefinition(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const;
	bool CheckStartConditions(const FAgAbilityData& Data, const FGameplayAbilityActorInfo* ActorInfo) const;
	bool CheckResources(const FAgAbilityData& Data, const FGameplayAbilityActorInfo* ActorInfo) const;
	void BeginBusy();
	void EndBusy();
	void Finish(bool bCancelled = false);
	bool PlayMotion(FName Id, bool bPlayerAction = false, float Rate = 1.f, FName Section = NAME_None, float TranslationScale = 1.f,
	    float StartTime = 0.f);
	void BindMontageInstance(UAnimInstance& Anim, const FAnimMontageInstance& Instance);
	void InitializeNotifyStates(UAnimInstance& Anim, FAnimMontageInstance& Instance);
	bool IsCurrentMontageNotify(const FBranchingPointNotifyPayload& Payload) const;
	void ClearMotion(bool bStop);
	void UpdateMotionConstraints();
	void EndWindows();
	void Listen(FGameplayTag Event);
	void WaitForLanding();
	void StartComboHit(int32 Index);
	void StartGuard(bool bParryWindow);
	void SetGuarding(bool bGuarding);
	void CloseParryWindow();
	void StartGroggy();
	void StartKnockUp();
	void StartDeath(const FGameplayEventData* Event);
	void ApplyReward(FName ParamsId);
	void ApplySingleAttack(
	    FName ParamsId, const TArray<AAgCharacterBase*>& Targets, bool bFinisher = false, TOptional<FVector> HitLocation = {});
	void NotifyBasicAttackStarted();
	void FaceActor(const AActor* Actor);
	UAgAbilityTask_HoldTag* HoldConfiguredTag(FName ParamsId);

	UFUNCTION()
	void HandleEvent(FGameplayEventData Event);

	UFUNCTION()
	void HandleMontageNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& Payload);

	UFUNCTION()
	void HandleMontageNotifyEnd(FName NotifyName, const FBranchingPointNotifyPayload& Payload);

	UFUNCTION()
	void HandleMotionCompleted();

	UFUNCTION()
	void HandleMotionCancelled();

	UFUNCTION()
	void HandleMotionBlendOut();

	UFUNCTION()
	void HandleLanded(EMovementMode Mode);

	UFUNCTION()
	void HandleRising();

	UFUNCTION()
	void HandleFalling();

	UFUNCTION()
	void HandleGroggyEndDue();

	UFUNCTION()
	void HandleCutsceneFinished();

	UFUNCTION()
	void HandleDashFinished();

	UFUNCTION()
	void HandleTargetLost();

	UPROPERTY()
	FAgAbilityData GrantedDefinition;

	UPROPERTY()
	FAgAbilityData ExecutionDefinition;

	UPROPERTY()
	TMap<FName, TObjectPtr<UAnimMontage>> ExecutionMontages;

	UPROPERTY()
	TArray<FName> ComboMontageIds;

	UPROPERTY()
	TMap<FName, FAgProjectileDefinition> ExecutionProjectiles;

	UPROPERTY()
	FAgAbilityMontageExecution Motion;

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MotionTask;

	UPROPERTY()
	TArray<FAgAbilityWindowExecution> Windows;

	UPROPERTY()
	TArray<TObjectPtr<UAgRootMotionModifier_MotionWarping>> Warps;

	UPROPERTY()
	TObjectPtr<UAgAbilityTask_HoldTag> BusyTag;

	UPROPERTY()
	TObjectPtr<UAgAbilityTask_HoldTag> GuardingTag;

	UPROPERTY()
	TObjectPtr<UAgAbilityTask_HoldTag> DownedTag;

	UPROPERTY()
	TObjectPtr<UAgAbilityTask_Dash> DashTask;

	UPROPERTY()
	TObjectPtr<UAgAbilityTask_TimeStop> UltimateTimeStop;

	UPROPERTY()
	TObjectPtr<UAgAbilityTask_LockOn> LockOnTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> GroggyWait;
	FActiveGameplayEffectHandle DrainEffect;
	FActiveGameplayEffectHandle ParryWindow;
	TWeakObjectPtr<AAgCharacterBase> ExecutionTarget;
	int32 ComboIndex = 0;
	int32 ExecutionStrike = 0;
	float SPSpent = 0.f;
	float FallBlendSpeed = 0.f;
	float WarpStopDistance = 0.f;
	bool bPerfectDodged = false;
	bool bHoldingDash = false;
	bool bDashFinished = false;
	bool bGettingUp = false;
	bool bEnding = false;
	bool bRootMotionTranslationFromVelocity = false;
};
