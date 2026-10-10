// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "ActiveGameplayEffectHandle.h"
#include "Data/AgAttackData.h"
#include "GameFramework/Character.h"
#include "AgCharacterBase.generated.h"

class UAbilitySystemComponent;
class UAgAttributeSet;
class UAgCharacterData;
class UGameplayAbility;
class UMotionWarpingComponent;
class AAgCharacterBase;
struct FOnAttributeChangeData;

DECLARE_MULTICAST_DELEGATE_OneParam(FAgCharacterDiedSignature, AAgCharacterBase* /*DeadCharacter*/);

/**
 * Base for the player and the boss.
 * Owns the ability system and stats, the weapon and its attack windows, movement during attacks,
 * the character data asset and the death handling from 전투 시스템 '사망 처리'.
 */
UCLASS(Abstract)
class AAgCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AAgCharacterBase();

	/** Motion warping target the attack montages warp toward. */
	static const FName AttackWarpTarget;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UAgAttributeSet* GetAttributeSet() const { return AttributeSet; }

	UAgCharacterData* GetCharacterData() const { return CharacterData; }

	UMotionWarpingComponent* GetMotionWarping() const { return MotionWarping; }

	bool IsDead() const { return bDead; }

	/** Sets the attack data the running action's attack windows use, one entry per window. */
	void SetActiveAttack(TConstArrayView<FAgAttackHit> Hits);

	/** Closes any open attack window and forgets the action's attack data. */
	void ClearActiveAttack();

	/** 판정 구간 start, from the attack window notify. */
	void BeginAttackWindow(int32 HitIndex);

	/** 판정 구간 end, from the attack window notify. */
	void EndAttackWindow(int32 HitIndex);

	/** 정지 간격: root motion never carries this character closer than StopDistance to Target (capsule centers, horizontal). */
	void SetStopTarget(AActor* Target, float InStopDistance);
	void ClearStopTarget();

	/**
	 * Steers the attack warp target to StopDistance in front of Target every frame until StopApproach.
	 * Stops following when Target goes beyond MaxRange (0 = no limit); MaxTravel (0 = no limit) caps how far
	 * from the starting point the warp target may be. bFaceTarget also warps the facing toward Target.
	 * Normally a character already closer stays put; bExactGap moves it back to the gap too (처형 alignment).
	 */
	void StartApproach(AActor* Target, float InStopDistance, float MaxRange, float MaxTravel, bool bFaceTarget, bool bExactGap = false);

	/** Stops updating the warp target; the last one stays for the rest of the warp window. */
	void StopApproach();

	bool IsApproaching() const { return ApproachTarget.IsValid(); }

	/** 움찔: plays the additive flinch on top of the current motion. */
	void PlayFlinch();

	/** 가드 움찔: plays the additive guard flinch on the guard pose. Only characters that guard have one. */
	virtual void PlayGuardFlinch() {}

	/**
	 * 사망 처리: stops every action, input and movement, removes this character's attack windows,
	 * turns off character-to-character collision and switches to ragdoll moving with PushVelocity. Broadcasts OnDied.
	 * DeathAbility is the ability running the death, which is not cancelled. Without bRagdoll the current motion keeps
	 * playing (처형 사망) and the caller starts the ragdoll later.
	 */
	virtual void Die(const FVector& PushVelocity, UGameplayAbility* DeathAbility = nullptr, bool bRagdoll = true);

	/** 사망 처리 6: switches the dead character to ragdoll, pushed with PushVelocity. */
	void StartRagdoll(const FVector& PushVelocity);

	/** HP를 0으로 만들어 사망 처리를 시작합니다. (Kill Z 낙하, 테스트용 콘솔 명령) */
	void Kill();

	/** Broadcast once when this character dies. */
	FAgCharacterDiedSignature OnDied;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PossessedBy(AController* NewController) override;

	/** 레벨의 Kill Z 아래로 떨어지면 HP를 0으로 만들어 사망 처리합니다. (전투 시스템 '사망') */
	virtual void FellOutOfWorld(const UDamageType& DamageType) override;

	/** Grants the character data's abilities. */
	virtual void GrantAbilities();

	/** Starts the regen effects (PP here; the player adds SP). */
	virtual void StartRegen();

	/** Movement speed as a multiple of MOV (the player's guard and sprint change it). */
	virtual float GetMoveSpeedMultiplier() const { return 1.f; }

	/** Applies MOV times the current multiplier to the movement component. */
	void UpdateMoveSpeed();

	/** Restarts a regen delay from the beginning: removes the running one and applies a new one. */
	void RestartRegenDelay(FActiveGameplayEffectHandle& Handle, const FGameplayTag& DelayTag, float Duration);

	/** DA_Player or DA_Boss. */
	UPROPERTY(EditDefaultsOnly, Category = "Data")
	TObjectPtr<UAgCharacterData> CharacterData;

private:
	/** Sets the stats to the starting values in the character data. */
	void InitializeStats();

	/** Puts the character data's weapon mesh on the matching component and attaches it to the weapon socket. */
	void ApplyWeapon();

	void HandleMOVChanged(const FOnAttributeChangeData& Data);

	/** PP 리젠: any PP decrease restarts the regen delay. */
	void HandlePPChanged(const FOnAttributeChangeData& Data);

	/** The component holding the weapon mesh, or null. */
	USceneComponent* GetWeaponComponent() const;

	/** World positions of the weapon's damaging edge. */
	bool GetWeaponEdge(FVector& OutStart, FVector& OutEnd) const;

	/** Sweeps the weapon edge from its last position to the current one, in small steps so fast swings don't skip. */
	void SweepWeapon();

	void UpdateApproach();

	FTransform ClampRootMotion(const FTransform& WorldRootMotion, UCharacterMovementComponent* Movement, float DeltaSeconds);

	UPROPERTY(VisibleAnywhere, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UAgAttributeSet> AttributeSet;

	/** Holds the weapon when it is a static mesh (katana). */
	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	TObjectPtr<UStaticMeshComponent> StaticWeapon;

	/** Holds the weapon when it is a skeletal mesh (halberd). */
	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	TObjectPtr<USkeletalMeshComponent> SkeletalWeapon;

	/** 공격 보정 (Motion Warping). */
	UPROPERTY(VisibleAnywhere, Category = "Movement")
	TObjectPtr<UMotionWarpingComponent> MotionWarping;

	TArray<FAgAttackHit> ActiveAttackHits;
	int32 OpenWindowHit = INDEX_NONE;
	/** Characters the open window already touched (one hit per target per window). */
	TArray<TWeakObjectPtr<AActor>> WindowTouchedActors;
	FVector LastEdgeStart = FVector::ZeroVector;
	FVector LastEdgeEnd = FVector::ZeroVector;

	TWeakObjectPtr<AActor> StopTarget;
	float StopDistance = 0.f;

	TWeakObjectPtr<AActor> ApproachTarget;
	FVector ApproachOrigin = FVector::ZeroVector;
	float ApproachStopDistance = 0.f;
	float ApproachMaxRange = 0.f;
	float ApproachMaxTravel = 0.f;
	bool bApproachFacesTarget = false;
	bool bApproachExactGap = false;

	FActiveGameplayEffectHandle PPRegenDelay;

	bool bDead = false;
};
