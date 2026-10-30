// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Character.h"
#include "GameplayAbilitySpecHandle.h"
#include "GenericTeamAgentInterface.h"
#include "AgCharacterBase.generated.h"

class UAgAttributeSet;
class UAgCharacterData;
struct FAgAbilityData;
class UMotionWarpingComponent;
class UAgHitPhysicsComponent;
class UAnimMontage;
class AAgCharacterBase;
struct FGameplayAbilitySpec;
struct FGameplayEventData;
struct FGameplayTag;
struct FOnAttributeChangeData;

DECLARE_MULTICAST_DELEGATE_OneParam(FAgCharacterDiedSignature, AAgCharacterBase*);

enum class EAgTeam : uint8
{
	Ally,
	Enemy,
	Neutral
};

UCLASS(Abstract)
class AAgCharacterBase : public ACharacter, public IAbilitySystemInterface, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	AAgCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual UAgAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual FGenericTeamId GetGenericTeamId() const override
	{
		return FGenericTeamId(static_cast<uint8>(Team));
	}

	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;

	UAgAttributeSet* GetAttributeSet() const
	{
		return AttributeSet;
	}

	UAgCharacterData* GetCharacterData() const
	{
		return CharacterData;
	}

	FName GetCharacterDataId() const
	{
		return CharacterDataId;
	}

	FGameplayAbilitySpec* FindAbilitySpec(FName Id) const;

	FName FindAbilityId(const FGameplayAbilitySpecHandle Handle) const;

	/** 데이터를 다시 읽으면 바뀌므로, 받은 포인터는 그 함수 안에서만 씁니다. */
	const FAgAbilityData* FindAbility(const FGameplayAbilitySpecHandle Handle) const;

	UMotionWarpingComponent* GetMotionWarping() const
	{
		return MotionWarping;
	}

	UAgHitPhysicsComponent* GetHitPhysics() const
	{
		return HitPhysics;
	}

	USceneComponent* GetWeaponComponent() const
	{
		return WeaponComponent;
	}

	bool IsDead() const;

	/** 무기, 투사체, 범위 적중이 모두 이 판정을 써야 맞는 대상이 공격 방식마다 갈리지 않습니다. */
	bool CanBeHitBy(const AActor& Attacker) const;

	virtual bool CanLosePP() const
	{
		return false;
	}

	virtual FVector GetMoveInputDirection() const
	{
		return FVector::ZeroVector;
	}

	AAgCharacterBase* GetLockOnTarget() const;

	bool IsFacingFixed() const;

	void Kill();

	void SetTimeCompensation(float Compensation);

	void NotifyLanded();

	/** 몽타주 루트 모션이 있으면 엔진이 루트 모션 소스를 쓰지 않으므로, 소스로 움직이는 Task가 그동안 끕니다. */
	void PushDisableMontageRootMotion();
	void PopDisableMontageRootMotion();

	FAgCharacterDiedSignature OnDied;

	virtual void PreInitializeComponents() override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void PossessedBy(AController* NewController) override;

	virtual void Landed(const FHitResult& Hit) override;

	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;
	virtual void NotifyJumpApex() override;
	virtual void MoveBlockedBy(const FHitResult& Impact) override;

	virtual void FellOutOfWorld(const UDamageType& DamageType) override;

protected:
	virtual void ApplyDeathState();

	void SendEventToSelf(const FGameplayTag& EventTag);

	void DieWithRagdoll(const FVector& PushVelocity);

	void StartRagdoll(const FVector& PushVelocity);

	virtual void HandleDeathEvent(const FGameplayEventData* Payload);

	UPROPERTY(EditDefaultsOnly, Category = "Data", meta = (GetOptions = "GetCharacterDataIds"))
	FName CharacterDataId;

	EAgTeam Team = EAgTeam::Neutral;

	UPROPERTY(Transient)
	TObjectPtr<UAgCharacterData> CharacterData;

private:
	void GrantAbilities();

	void ApplyEffects();

	UFUNCTION()
	TArray<FString> GetCharacterDataIds() const;

	void InitializeAttributes();

	void HandleMOVChanged(const FOnAttributeChangeData& Data);

	void HandleHitstopChanged(const FGameplayTag Tag, int32 NewCount);

	void HandlePassThroughChanged(const FGameplayTag Tag, int32 NewCount);

	void UpdateTimeDilation();

	float TimeCompensation = 1.f;

	FTransform ClampRootMotion(const FTransform& WorldRootMotion, UCharacterMovementComponent* Movement, float DeltaSeconds);

	void DisableRootMotionMontage();

	/** 끄는 동안 새로 시작한 재생(넉업 정점의 블렌드 전환 등)이 루트 모션을 가져가므로 그 재생도 끕니다. */
	UFUNCTION()
	void HandleMontageStarted(UAnimMontage* Montage);

	int32 MontageRootMotionDisableCount = 0;

	int32 RootMotionDisabledMontageId = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, Category = "Abilities")
	TObjectPtr<UAgAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UAgAttributeSet> AttributeSet;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> WeaponComponent;

	UPROPERTY(VisibleAnywhere, Category = "Movement")
	TObjectPtr<UMotionWarpingComponent> MotionWarping;

	UPROPERTY(VisibleAnywhere, Category = "Animation")
	TObjectPtr<UAgHitPhysicsComponent> HitPhysics;

	TMap<FGameplayAbilitySpecHandle, FName> AbilityIds;

	TEnumAsByte<ECollisionResponse> PawnResponse = ECR_Block;

	FDelegateHandle DeathEventHandle;
};
