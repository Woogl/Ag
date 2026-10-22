// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Character.h"
#include "GameplayAbilitySpecHandle.h"
#include "GenericTeamAgentInterface.h"
#include "AgCharacterBase.generated.h"

class UAbilitySystemComponent;
class UAgAttributeSet;
class UAgCharacterData;
struct FAgAbilityData;
class UMotionWarpingComponent;
class UAgHitPhysicsComponent;
class UAnimMontage;
class UCapsuleComponent;
class AAgCharacterBase;
struct FGameplayAbilitySpec;
struct FGameplayEventData;
struct FGameplayTag;
struct FOnAttributeChangeData;

DECLARE_MULTICAST_DELEGATE_OneParam(FAgCharacterDiedSignature, AAgCharacterBase*);

UCLASS(Abstract)
class AAgCharacterBase : public ACharacter, public IAbilitySystemInterface, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	AAgCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	static const FName WeaponTag;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual FGenericTeamId GetGenericTeamId() const override
	{
		return FGenericTeamId(1);
	}

	UAgAttributeSet* GetAttributeSet() const
	{
		return AttributeSet;
	}

	UAgCharacterData* GetCharacterData() const
	{
		return CharacterData;
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

	bool CanLosePP() const;

	virtual FVector GetMoveInputDirection() const
	{
		return FVector::ZeroVector;
	}

	AAgCharacterBase* GetLockOnTarget() const;

	bool IsFacingFixed() const;

	void Kill();

	void SetTimeCompensation(float Compensation);

	void NotifyLanded();

	FAgCharacterDiedSignature OnDied;

	virtual void PreInitializeComponents() override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Tick(float DeltaSeconds) override;
	virtual void PossessedBy(AController* NewController) override;

	virtual void Landed(const FHitResult& Hit) override;

	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;
	virtual void MoveBlockedBy(const FHitResult& Impact) override;

	virtual void FellOutOfWorld(const UDamageType& DamageType) override;

protected:
	void SendEventToSelf(const FGameplayTag& EventTag);

	UPROPERTY(EditDefaultsOnly, Category = "Data", meta = (GetOptions = "GetCharacterDataIds"))
	FName CharacterDataId;

	TSubclassOf<UAgCharacterData> CharacterDataClass;

	UPROPERTY(Transient)
	TObjectPtr<UAgCharacterData> CharacterData;

	bool bEndingPlay = false;

private:
	void ApplyDeathState();
	void DieWithRagdoll(const FVector& PushVelocity);
	void DieWithDeathMotion(UAnimMontage& DeathMontage, float StartPosition);

	void StartRagdoll(const FVector& PushVelocity);

	void GrantAbilities();

	void ApplyEffects();

	UFUNCTION()
	TArray<FString> GetCharacterDataIds() const;

	void InitializeAttributes();

	void HandleMOVChanged(const FOnAttributeChangeData& Data);

	void HandleHitstopChanged(const FGameplayTag Tag, int32 NewCount);

	void HandlePassThroughChanged(const FGameplayTag Tag, int32 NewCount);

	void HandlePerfectDodgeWindowChanged(const FGameplayTag Tag, int32 NewCount);

	void HandleTimeStopChanged(const FGameplayTag Tag, int32 NewCount);

	void HandleDeathEvent(const FGameplayEventData* Payload);
	void HandleDeathMotionBlendingOut(UAnimMontage* Montage, bool bInterrupted);

	void UpdateTimeDilation();

	float TimeCompensation = 1.f;

	FTransform ClampRootMotion(const FTransform& WorldRootMotion, UCharacterMovementComponent* Movement, float DeltaSeconds);

	void UpdateApex();

	UPROPERTY(VisibleAnywhere, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UAgAttributeSet> AttributeSet;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> WeaponComponent;

	UPROPERTY(VisibleAnywhere, Category = "Movement")
	TObjectPtr<UMotionWarpingComponent> MotionWarping;

	UPROPERTY(VisibleAnywhere, Category = "Animation")
	TObjectPtr<UAgHitPhysicsComponent> HitPhysics;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	TObjectPtr<UCapsuleComponent> PerfectDodgeCapsule;

	TMap<FGameplayAbilitySpecHandle, FName> AbilityIds;

	TEnumAsByte<ECollisionResponse> PawnResponse = ECR_Block;

	float PreviousVelocityZ = 0.f;

	int32 DeathMotionInstanceId = INDEX_NONE;
	FDelegateHandle DeathEventHandle;
};
