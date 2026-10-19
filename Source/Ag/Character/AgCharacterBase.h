// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "ActiveGameplayEffectHandle.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Character.h"
#include "GameplayAbilitySpecHandle.h"
#include "GenericTeamAgentInterface.h"
#include "AgCharacterBase.generated.h"

class UAbilitySystemComponent;
class UAgAttributeSet;
class UAgCharacterData;
enum class EAgEffect : uint8;
struct FAgAbilityData;
class UMotionWarpingComponent;
class UAnimMontage;
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
	AAgCharacterBase();

	static const FName WeaponTag;
	static const FName WeaponCollisionTag;

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

	UFUNCTION(BlueprintPure, Category = "Weapon")
	USceneComponent* GetWeaponComponent() const
	{
		return WeaponComponent;
	}

	bool IsDead() const;

	bool CanLosePP() const;

	virtual AActor* GetFallbackFacingTarget() const
	{
		return nullptr;
	}

	virtual FVector GetMoveInputDirection() const
	{
		return FVector::ZeroVector;
	}

	bool IsFacingFixed() const;

	void PlayFlinch();

	void PlayGuardFlinch();

	virtual void Die(const FVector& PushVelocity, bool bRagdoll = true);

	void StartRagdoll(const FVector& PushVelocity);

	void Kill();

	void SetTimeCompensation(float Compensation);

	FAgCharacterDiedSignature OnDied;

protected:
	virtual void PreInitializeComponents() override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Tick(float DeltaSeconds) override;
	virtual void PossessedBy(AController* NewController) override;

	virtual void Landed(const FHitResult& Hit) override;

	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;

	virtual void FellOutOfWorld(const UDamageType& DamageType) override;

	virtual void GrantAbilities();

	void ApplyEffects();

	void SendEventToSelf(const FGameplayTag& EventTag);

	virtual float GetMoveSpeedMultiplier() const
	{
		return 1.f;
	}

	virtual AActor* GetFacingTarget() const
	{
		return nullptr;
	}

	void UpdateMoveSpeed();

	void RestartRegenDelay(FActiveGameplayEffectHandle& Handle, EAgEffect DelayEffect);

	UPROPERTY(EditDefaultsOnly, Category = "Data", meta = (GetOptions = "GetCharacterDataIds"))
	FName CharacterDataId;

	TSubclassOf<UAgCharacterData> CharacterDataClass;

	UPROPERTY(Transient)
	TObjectPtr<UAgCharacterData> CharacterData;

	bool bEndingPlay = false;

private:
	UFUNCTION()
	TArray<FString> GetCharacterDataIds() const;

	void InitializeAttributes();

	void HandleMOVChanged(const FOnAttributeChangeData& Data);

	void HandlePPChanged(const FOnAttributeChangeData& Data);

	void HandleHitstopChanged(const FGameplayTag Tag, int32 NewCount);

	void HandlePassThroughChanged(const FGameplayTag Tag, int32 NewCount);

	void HandleTimeStopChanged(const FGameplayTag Tag, int32 NewCount);

	void HandleDeathEvent(const FGameplayEventData* Payload);
	void HandleDeathMotionBlendingOut(UAnimMontage* Montage, bool bInterrupted);

	void UpdateTimeDilation();

	float TimeCompensation = 1.f;

	FTransform ClampRootMotion(const FTransform& WorldRootMotion, UCharacterMovementComponent* Movement, float DeltaSeconds);

	void UpdateFacing(float DeltaSeconds);

	void UpdateApex();

	UPROPERTY(VisibleAnywhere, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UAgAttributeSet> AttributeSet;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> WeaponComponent;

	UPROPERTY(VisibleAnywhere, Category = "Movement")
	TObjectPtr<UMotionWarpingComponent> MotionWarping;

	FActiveGameplayEffectHandle PPRegenDelay;

	TMap<FGameplayAbilitySpecHandle, FName> AbilityIds;

	TEnumAsByte<ECollisionResponse> PawnResponse = ECR_Block;

	float PreviousVelocityZ = 0.f;

	int32 DeathMotionInstanceId = INDEX_NONE;
	FDelegateHandle DeathEventHandle;
};
