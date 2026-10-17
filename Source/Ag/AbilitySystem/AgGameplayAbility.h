// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Data/AgAbilityData.h"
#include "Data/AgCharacterData.h"
#include "Data/AgProjectileData.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/EngineTypes.h"
#include "StructUtils/InstancedStruct.h"
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
class UMotionWarpingComponent;
struct FAgTrackParams;
struct FAnimMontageInstance;
enum class EAgEffect : uint8;

/** 시간표 구간 하나의 실행. 재생의 몽타주 인스턴스와 그 재생의 항목 번호로 구분합니다. */
USTRUCT()
struct FAgAbilityWindowExecution
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UAbilityTask> Task;
	FName TaskName;
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
};

USTRUCT()
struct FAgAbilityMontageExecution
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UAnimMontage> Montage;
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
	/**
	 * 엔진 MakeOutgoingGameplayEffectSpec이 GE 클래스로 하는 일(어빌리티 컨텍스트, 어빌리티 태그, 스펙의 SetByCaller)을
	 * Effects.json의 효과로 합니다. 효과가 없으면 빈 핸들입니다.
	 */
	FGameplayEffectSpecHandle MakeEffectSpec(EAgEffect Effect) const;
	FGameplayEffectSpecHandle MakeEffectSpec(
	    FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, EAgEffect Effect) const;
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

	bool PrepareWarpTarget(
	    int32 InstanceId, float WindowStart, const FAgTrackParams& Params, AActor*& Target, float& Gap, bool& bFacingOnly);
	void RegisterWarp(UAgRootMotionModifier_MotionWarping* Modifier);
	static AAgCharacterBase* FindExecutionTarget(const AAgPlayerCharacter* Player);

	/** 이 행동을 시작할 때 복사한 투사체 정의. 없으면 null입니다. */
	const FAgProjectileDefinition* FindProjectile(FName ProjectileId) const
	{
		return ExecutionProjectiles.Find(ProjectileId);
	}

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
		virtual void ExecuteTick(
		    float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) override;
		virtual FString DiagnosticMessage() override;
	};

	AAgCharacterBase* GetAgCharacter() const;
	const FAgAbilityData* FindDefinition(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const;
	bool CheckStartConditions(const FAgAbilityData& Data, const FGameplayAbilityActorInfo* ActorInfo) const;
	bool CheckResources(const FAgAbilityData& Data, const FGameplayAbilityActorInfo* ActorInfo) const;
	void BeginBusy();
	void EndBusy();
	void Finish(bool bCancelled = false);
	bool PlayMotion(FName Id, bool bPlayerAction = false, float Rate = 1.f, FName Section = NAME_None, float TranslationScale = 1.f,
	    float StartTime = 0.f);
	void BindMontageInstance(const FAnimMontageInstance& Instance);
	void HandleMontageEvent(int32 InstanceId, FGameplayTag Event);

	/** 재생을 시작한 직후 그 몽타주의 시간표를 복사하고, 시작 위치에 걸친 구간과 그 위치의 지점을 실행합니다. */
	void BeginTimeline(const FAnimMontageInstance& Instance);

	/** 섹션을 옮긴 직후 판정 위치를 맞춥니다. 건너뛴 지점은 실행하지 않고, 옮긴 위치의 지점은 실행합니다. */
	void ResyncTimeline();
	void TickTimeline();
	void EvaluateTimeline(float NewPosition, ETimelineMode Mode);

	/** 재생이 끝났습니다. 켜진 구간을 끝내고 판정을 멈춥니다. */
	void EndTimeline();

	/** 판정을 멈추고 시간표를 버립니다. 구간의 실행은 부르는 쪽이 정리합니다. */
	void StopTimeline();
	void StartTimelineWindow(int32 Entry);
	void EndTimelineWindow(int32 InstanceId, int32 Entry);
	void RunTimelinePoint(int32 Entry);
	void JumpToMotionSection(FName Section);
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
	void ApplyReward();

	/** 어빌리티 데이터의 태그를 그 시간 동안 붙입니다(TimedTag 효과). */
	FActiveGameplayEffectHandle ApplyTimedTag(const FAgTimedTagParams& Params);
	/** 어빌리티 데이터의 Index번째 단발 타격으로 Targets를 칩니다. */
	void ApplySingleAttack(
	    int32 Index, const TArray<AAgCharacterBase*>& Targets, bool bFinisher = false, TOptional<FVector> HitLocation = {});
	void NotifyBasicAttackStarted();
	void FaceActor(const AActor* Actor);
	UAgAbilityTask_HoldTag* HoldConfiguredTag();

	UFUNCTION()
	void HandleEvent(FGameplayEventData Event);

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

	/** 추적 구간을 엔진이 워핑 노티파이를 찾는 때(워핑 컴포넌트의 OnPreUpdate)에 같은 조건으로 엔진 Motion Warping에 등록합니다. */
	UFUNCTION()
	void HandleWarpingPreUpdate(UMotionWarpingComponent* Warping);

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

	/** 재생 중인 몽타주의 시간표(구현 계획서 '단일 Ability Runner와 독립 기능 Task 설계안'의 '시간표 판정') */
	UPROPERTY()
	TArray<FAgTimelineEntry> TimelineEntries;

	/** 시간표를 판정하는 재생. 없으면 INDEX_NONE입니다. */
	int32 TimelineInstanceId = INDEX_NONE;

	/** 앞 판정의 재생 위치 */
	float TimelinePosition = 0.f;
	FAgTimelineTickFunction TimelineTick;

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
