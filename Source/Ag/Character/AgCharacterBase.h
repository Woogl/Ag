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
 * 플레이어와 보스의 공통 부모.
 * 어빌리티 시스템과 스탯, 무기와 판정 구간, 정지 간격, 캐릭터 데이터, 전투 시스템 '사망 처리'를 맡습니다.
 */
UCLASS(Abstract)
class AAgCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AAgCharacterBase();

	/** 공격 몽타주의 Motion Warping 구간이 향하는 워프 목표 이름. 공격 중 이동 태스크가 넣고 지웁니다. */
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

	/** 사망 처리를 시작했습니다(State.Dead). */
	bool IsDead() const;

	/** PP를 잃을 수 있습니다: 무한 PP가 아니고 그로기 중이 아닙니다(전투 시스템 'PP 감소량 계산식'). */
	bool CanLosePP() const;

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

	/** 정지 간격: 루트 모션이 이 캐릭터를 Target에 StopDistance보다 가까이 데려가지 않습니다(캡슐 중심 사이 수평 거리). */
	void SetStopTarget(AActor* Target, float InStopDistance);
	void ClearStopTarget();

	/**
	 * 바라보는 방향이 고정된 동안 참입니다. 기본은 판정 구간 동안입니다(플레이어 사양 '공격 중 이동').
	 * 공격 중 이동 태스크는 이 순간 접근 목표를 고정합니다.
	 */
	virtual bool IsFacingFixed() const { return OpenWindowHit != INDEX_NONE; }

	/** 움찔: plays the additive flinch on top of the current motion. */
	void PlayFlinch();

	/** 가드 움찔: 가드 자세 위에 가드 움찔을 덧입혀 재생합니다. 가드 움찔 몽타주가 없는 캐릭터는 재생하지 않습니다. */
	void PlayGuardFlinch();

	/**
	 * 사망 처리: stops every action, input and movement, removes this character's attack windows,
	 * turns off character-to-character collision and switches to ragdoll moving with PushVelocity. Broadcasts OnDied.
	 * DeathAbility is the ability running the death, which is not cancelled. Without bRagdoll the current motion keeps
	 * playing (처형 사망) and the caller starts the ragdoll later.
	 */
	virtual void Die(const FVector& PushVelocity, UGameplayAbility* DeathAbility = nullptr, bool bRagdoll = true);

	/** 사망 처리 6: switches the dead character to ragdoll, pushed with PushVelocity. */
	void StartRagdoll(const FVector& PushVelocity);

	/** HP를 0으로 만드는 GE를 적용해 사망 판정을 일으킵니다. (Kill Z 낙하, 테스트용 콘솔 명령) */
	void Kill();

	/** Broadcast once when this character dies. */
	FAgCharacterDiedSignature OnDied;

protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;

	/** 플레이가 끝나면 어빌리티를 모두 취소해, 컴포넌트가 등록 해제되는 동안 어빌리티가 돌지 않게 합니다. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Tick(float DeltaSeconds) override;
	virtual void PossessedBy(AController* NewController) override;

	/** 레벨의 Kill Z 아래로 떨어지면 HP를 0으로 만들어 사망 처리합니다. (전투 시스템 '사망') */
	virtual void FellOutOfWorld(const UDamageType& DamageType) override;

	/** Grants the character data's abilities. */
	virtual void GrantAbilities();

	/** 캐릭터 데이터의 리젠 효과(RegenEffects)를 겁니다. */
	void StartRegen();

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

	/** PP 리젠: PP가 줄 때마다 리젠 대기를 다시 시작합니다. */
	void HandlePPChanged(const FOnAttributeChangeData& Data);

	/** 히트스톱 태그가 붙고 빠질 때 이 캐릭터의 시간 배율을 바꿉니다. */
	void HandleHitstopChanged(const FGameplayTag Tag, int32 NewCount);

	/** 무기 판정 캡슐 축의 양 끝(월드 위치) */
	bool GetWeaponCollision(FVector& OutStart, FVector& OutEnd) const;

	/** 무기 판정 캡슐을 지난 위치에서 지금 위치까지 잘게 나눠 스윕합니다. 빠르게 휘둘러도 건너뛰지 않습니다. */
	void SweepWeapon();

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

	FActiveGameplayEffectHandle PPRegenDelay;
};
