// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "ActiveGameplayEffectHandle.h"
#include "Data/AgAttackData.h"
#include "GameFramework/Character.h"
#include "AgCharacterBase.generated.h"

class UAbilitySystemComponent;
class UCapsuleComponent;
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
 * the character data and the death handling from 전투 시스템 '사망 처리'.
 */
UCLASS(Abstract)
class AAgCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AAgCharacterBase();

	/** Motion warping target the attack montages warp toward. */
	static const FName AttackWarpTarget;

	/** 블루프린트에서 무기 메시 컴포넌트와 무기 판정 캡슐에 다는 컴포넌트 태그 */
	static const FName WeaponTag;
	static const FName WeaponCollisionTag;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UAgAttributeSet* GetAttributeSet() const { return AttributeSet; }

	UAgCharacterData* GetCharacterData() const { return CharacterData; }

	UMotionWarpingComponent* GetMotionWarping() const { return MotionWarping; }

	/** 블루프린트의 무기 메시 컴포넌트(WeaponTag). 없으면 null입니다. */
	USceneComponent* GetWeaponComponent() const { return WeaponComponent; }

	bool IsDead() const { return bDead; }

	/** Sets the attack data the running action's attack windows use, one entry per window. */
	void SetActiveAttack(TConstArrayView<FAgPlayerAttackHit> Hits);

	/** The same for a boss attack, whose windows have none of the player's charges or display flags. */
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
	 * StopApproach까지 매 프레임 공격 워프 목표를 Target 앞 StopDistance 지점으로 옮깁니다.
	 * Target이 MaxRange(0이면 제한 없음)를 벗어나면 따라가기를 멈춥니다. MaxTravel(음수면 제한 없음)은 워프 목표가
	 * 시작 위치에서 떨어질 수 있는 최대 거리입니다. bFaceTarget이면 바라보는 방향도 Target 쪽으로 워프합니다.
	 * 이미 더 가까우면 보통 그 자리에 머물고, bExactGap이면 그 간격까지 물러납니다(처형 맞춤).
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
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PossessedBy(AController* NewController) override;

	/** 레벨의 Kill Z 아래로 떨어지면 HP를 0으로 만들어 사망 처리합니다. (전투 시스템 '사망') */
	virtual void FellOutOfWorld(const UDamageType& DamageType) override;

	/** Grants the character data's abilities. */
	virtual void GrantAbilities();

	/** Starts the regen effects (PP here; the player adds SP). Starting again replaces the running ones. */
	virtual void StartRegen();

	/**
	 * A data file was read again in the editor: starts the regen again with the new rates. The stats change on the next
	 * PIE start (구현 계획서 '데이터' 다시 읽기).
	 */
	virtual void HandleDataReloaded();

	/** Movement speed as a multiple of MOV (the player's guard and sprint change it). */
	virtual float GetMoveSpeedMultiplier() const { return 1.f; }

	/** Applies MOV times the current multiplier to the movement component. */
	void UpdateMoveSpeed();

	/** Restarts a regen delay from the beginning: removes the running one and applies a new one. */
	void RestartRegenDelay(FActiveGameplayEffectHandle& Handle, const FGameplayTag& DelayTag, float Duration);

	/** DA_Player or DA_Boss. Its numbers come from its data file (Player.json, Boss.json). */
	UPROPERTY(EditDefaultsOnly, Category = "Data")
	TObjectPtr<UAgCharacterData> CharacterData;

private:
	/** Sets the stats to the starting values in the character data. */
	void InitializeStats();

	void HandleMOVChanged(const FOnAttributeChangeData& Data);

	/** PP 리젠: any PP decrease restarts the regen delay. */
	void HandlePPChanged(const FOnAttributeChangeData& Data);

	/** 무기 판정 캡슐 축의 양 끝(월드 위치) */
	bool GetWeaponCollision(FVector& OutStart, FVector& OutEnd) const;

	/** 무기 판정 캡슐을 지난 위치에서 지금 위치까지 잘게 나눠 스윕합니다. 빠르게 휘둘러도 건너뛰지 않습니다. */
	void SweepWeapon();

	void UpdateApproach();

	FTransform ClampRootMotion(const FTransform& WorldRootMotion, UCharacterMovementComponent* Movement, float DeltaSeconds);

	UPROPERTY(VisibleAnywhere, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UAgAttributeSet> AttributeSet;

	/** 무기 메시. 블루프린트에서 메시와 붙일 소켓(Parent Socket)을 정합니다. */
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> WeaponComponent;

	/**
	 * 무기 판정 캡슐. 블루프린트에서 무기 컴포넌트의 자식으로 두고 날 전체를 덮게 맞춥니다. 공격은 캡슐 양 끝 중심을 잇는
	 * 선분을 캡슐 반지름의 구로 스윕하며, 캡슐 자체의 충돌은 쓰지 않습니다.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UCapsuleComponent> WeaponCollisionComponent;

	/** 공격 보정 (Motion Warping). */
	UPROPERTY(VisibleAnywhere, Category = "Movement")
	TObjectPtr<UMotionWarpingComponent> MotionWarping;

	TArray<FAgPlayerAttackHit> ActiveAttackHits;
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
