// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "ActiveGameplayEffectHandle.h"
#include "Character/AgCharacterMotionConstraints.h"
#include "GameFramework/Character.h"
#include "GameplayAbilitySpecHandle.h"
#include "GenericTeamAgentInterface.h"
#include "AgCharacterBase.generated.h"

class UAbilitySystemComponent;
class UAgAttributeSet;
class UAgCharacterData;
struct FAgAbilityData;
class UGameplayAbility;
class UMotionWarpingComponent;
class UAgMontageTimelineComponent;
class UAnimMontage;
class AAgCharacterBase;
struct FGameplayAbilitySpec;
struct FGameplayTag;
struct FOnAttributeChangeData;

DECLARE_MULTICAST_DELEGATE_OneParam(FAgCharacterDiedSignature, AAgCharacterBase* /*DeadCharacter*/);

/**
 * 플레이어와 보스의 공통 부모. 어빌리티 시스템과 스탯, 무기, 정지 간격과 회전(전투 시스템 '공격 중 이동'),
 * 캐릭터 데이터, 전투 시스템 '사망 처리'를 맡습니다.
 */
UCLASS(Abstract)
class AAgCharacterBase : public ACharacter, public IAbilitySystemInterface, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

#if WITH_DEV_AUTOMATION_TESTS
	friend class FAgAbilityExecutionTest;
#endif

public:
	AAgCharacterBase();

	/** 블루프린트에서 무기 메시 컴포넌트와 무기 판정 캡슐에 다는 컴포넌트 태그 */
	static const FName WeaponTag;
	static const FName WeaponCollisionTag;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/** 팀(엔진 팀 인터페이스). 팀이 다르면 적대합니다. 플레이어는 0번 팀이고, 그 밖의 캐릭터는 1번 팀입니다. */
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

	/** 어빌리티 ID로 부여된 어빌리티의 스펙. 없으면 null입니다. */
	FGameplayAbilitySpec* FindAbilitySpec(FName Id) const;

	/** 부여된 스펙(Handle)의 어빌리티 ID. 어빌리티 목록으로 부여한 스펙이 아니면 None입니다. */
	FName FindAbilityId(const FGameplayAbilitySpecHandle Handle) const;
	void ForgetAbility(FGameplayAbilitySpecHandle Handle)
	{
		AbilityIds.Remove(Handle);
	}

	/**
	 * 부여된 스펙(Handle)의 어빌리티 항목(캐릭터 데이터의 어빌리티 목록). 어빌리티 목록으로 부여하지 않았으면 null입니다.
	 * 데이터를 다시 읽으면 바뀌므로 포인터는 그 함수 안에서만 씁니다(구현 계획서 '다시 읽기').
	 */
	const FAgAbilityData* FindAbility(const FGameplayAbilitySpecHandle Handle) const;

	UMotionWarpingComponent* GetMotionWarping() const
	{
		return MotionWarping;
	}

	/** 재생 중인 몽타주의 시간표를 판정합니다. */
	UAgMontageTimelineComponent* GetMontageTimeline() const
	{
		return MontageTimeline;
	}

	/** 블루프린트의 무기 메시 컴포넌트(WeaponTag). 없으면 null입니다. */
	USceneComponent* GetWeaponComponent() const
	{
		return WeaponComponent;
	}

	/** 사망 처리를 시작했습니다(State.Dead). */
	bool IsDead() const;

	/** PP를 잃을 수 있습니다: 그로기 면역(State.GroggyImmune)도 그로기 중도 아닙니다(전투 시스템 'PP 감소량 계산식'). */
	bool CanLosePP() const;

	/** 실행 소유자가 정한 최종 이동 제약을 적용합니다. 다른 소유자의 해제 요청은 무시합니다. */
	void SetMotionConstraints(UObject& RequestOwner, const FAgCharacterMotionConstraints& Constraints);
	void ClearMotionConstraints(const UObject& RequestOwner);

	/** 전투 시스템 '공격 중 이동'의 대상: 타를 시작할 때 이 캐릭터가 공격할 적입니다. 없으면 null입니다. */
	virtual AAgCharacterBase* FindAttackTarget() const
	{
		return nullptr;
	}

	/** 플레이어 사양 '공격 방향': 타겟팅 범위 안에 공격 대상이 없는 타에서 향해 돌 대상입니다. 없으면 null(정면)입니다. */
	virtual AActor* GetFallbackFacingTarget() const
	{
		return nullptr;
	}

	/** 실행 소유자가 회전을 제어하거나 캐릭터 상태가 일반 회전을 금지합니다. */
	virtual bool IsFacingFixed() const;

	/** 움찔: 하던 동작 위에 움찔을 덧입혀 재생합니다. */
	void PlayFlinch();

	/** 가드 움찔: 가드 자세 위에 가드 움찔을 덧입혀 재생합니다. 가드 움찔 몽타주가 없는 캐릭터는 재생하지 않습니다. */
	void PlayGuardFlinch();

	/**
	 * 사망 처리: 모든 행동과 입력, 이동을 멈추고, 이 캐릭터의 판정 구간을 없애고, 캐릭터 간 충돌을 끈 뒤 PushVelocity로
	 * 밀리는 래그돌이 됩니다. 끝에 OnDied를 알립니다. DeathAbility는 사망을 처리하는 어빌리티라 취소하지 않습니다.
	 * bRagdoll이 거짓이면 지금 모션을 멈추지 않고(처형 사망), 래그돌은 호출한 쪽이 나중에 시작합니다.
	 */
	virtual void Die(const FVector& PushVelocity, UGameplayAbility* DeathAbility = nullptr, bool bRagdoll = true);

	/** 사망 처리: 죽은 캐릭터를 래그돌로 바꾸고 PushVelocity로 밉니다. */
	void StartRagdoll(const FVector& PushVelocity);

	/** HP를 0으로 만드는 GE를 적용해 사망 판정을 일으킵니다. (Kill Z 낙하, 테스트용 콘솔 명령) */
	void Kill();

	/** 시간 정지 중에 이 캐릭터만 정상 속도로 돌도록 시간 배율에 곱하는 값. 시간 서브시스템이 정하고, 평소에는 1입니다. */
	void SetTimeCompensation(float Compensation);

	/** 사망할 때 한 번 알립니다. */
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

	/** 캐릭터 데이터의 어빌리티 목록(Abilities)의 항목마다 그 클래스를 부여합니다. */
	virtual void GrantAbilities();

	/** 캐릭터 데이터의 시작할 때 거는 GameplayEffect(Effects)를 겁니다. */
	void ApplyEffects();

	/** MOV에 곱하는 이동 속도 배율. 플레이어는 가드와 달리기가 바꿉니다. */
	virtual float GetMoveSpeedMultiplier() const
	{
		return 1.f;
	}

	/** 일반 회전의 대상. 보스는 AI 대상을 반환합니다. */
	virtual AActor* GetFacingTarget() const
	{
		return nullptr;
	}

	/** MOV × GetMoveSpeedMultiplier()를 캐릭터 무브먼트의 최대 걷기 속도에 넣습니다. */
	void UpdateMoveSpeed();

	/** 리젠 대기를 처음부터 다시 셉니다: 도는 대기를 지우고 새로 적용합니다. */
	void RestartRegenDelay(FActiveGameplayEffectHandle& Handle, const FGameplayTag& DelayTag, float Duration);

	/** DA_Player 또는 DA_Boss. 기획 값은 그 JSON(Player.json, Boss.json)에서 채웁니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Data")
	TObjectPtr<UAgCharacterData> CharacterData;

private:
	/** 스탯을 캐릭터 데이터의 초기값으로 정합니다. */
	void InitializeAttributes();

	void HandleMOVChanged(const FOnAttributeChangeData& Data);

	/** PP 리젠: PP가 줄 때마다 리젠 대기를 다시 시작합니다. */
	void HandlePPChanged(const FOnAttributeChangeData& Data);

	/** 히트스톱 태그가 붙고 빠질 때 이 캐릭터의 시간 배율을 바꿉니다. */
	void HandleHitstopChanged(const FGameplayTag Tag, int32 NewCount);

	/** 시간 배율 = 히트스톱 배율 × 시간 정지 보정 */
	void UpdateTimeDilation();

	/** SetTimeCompensation으로 정한 시간 정지 보정 */
	float TimeCompensation = 1.f;

	FTransform ClampRootMotion(const FTransform& WorldRootMotion, UCharacterMovementComponent* Movement, float DeltaSeconds);

	/** 회전: 바라보는 방향이 고정되지 않은 동안 GetFacingTarget을 향해 돕니다. */
	void UpdateFacing(float DeltaSeconds);

	TWeakObjectPtr<UObject> MotionConstraintOwner;
	FAgCharacterMotionConstraints MotionConstraints;

	UPROPERTY(VisibleAnywhere, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UAgAttributeSet> AttributeSet;

	/** 무기 메시. 블루프린트에서 메시와 붙일 소켓(Parent Socket)을 정합니다. */
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> WeaponComponent;

	/** 공격 중 이동 (Motion Warping) */
	UPROPERTY(VisibleAnywhere, Category = "Movement")
	TObjectPtr<UMotionWarpingComponent> MotionWarping;

	/** 몽타주 시간표 판정 */
	UPROPERTY(VisibleAnywhere, Category = "Animation")
	TObjectPtr<UAgMontageTimelineComponent> MontageTimeline;

	FActiveGameplayEffectHandle PPRegenDelay;

	/** 어빌리티 목록으로 부여한 스펙마다 그 어빌리티 ID */
	TMap<FGameplayAbilitySpecHandle, FName> AbilityIds;
};
