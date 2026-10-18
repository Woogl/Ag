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

DECLARE_MULTICAST_DELEGATE_OneParam(FAgCharacterDiedSignature, AAgCharacterBase* /*DeadCharacter*/);

/**
 * 플레이어와 보스의 공통 부모. 어빌리티 시스템과 스탯, 무기, 캐릭터 데이터를 맡고, 행동이 아닌 캐릭터 규칙(구현 계획서
 * '어빌리티와 AbilityTask'의 '주인 객체의 규칙')을 처리합니다: 공중 상태(State.Airborne)와 이동 사건(Event.Landed,
 * Event.Apex), 행동하지 않을 때의 자동 회전, 재생 중인 행동의 대상과의 최소 간격, 상태 태그(State.PassThrough, State.TimeStop),
 * 사망(Event.Death, 전투 시스템 '사망 처리').
 */
UCLASS(Abstract)
class AAgCharacterBase : public ACharacter, public IAbilitySystemInterface, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

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

	/** 어빌리티 ID로 부여된 어빌리티의 스펙. 없으면 null입니다. 보스 AI처럼 데이터에 적힌 ID로 찾을 때 씁니다. */
	FGameplayAbilitySpec* FindAbilitySpec(FName Id) const;

	/** 부여된 스펙(Handle)의 어빌리티 ID. 어빌리티 목록으로 부여한 스펙이 아니면 None입니다. */
	FName FindAbilityId(const FGameplayAbilitySpecHandle Handle) const;

	/**
	 * 부여된 스펙(Handle)의 어빌리티 항목(캐릭터 데이터의 어빌리티 목록). 어빌리티 목록으로 부여하지 않았으면 null입니다.
	 * 데이터를 다시 읽으면 바뀌므로 포인터는 그 함수 안에서만 씁니다(구현 계획서 '다시 읽기').
	 */
	const FAgAbilityData* FindAbility(const FGameplayAbilitySpecHandle Handle) const;

	UMotionWarpingComponent* GetMotionWarping() const
	{
		return MotionWarping;
	}

	/** 블루프린트의 무기 메시 컴포넌트(WeaponTag). 없으면 null입니다. */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	USceneComponent* GetWeaponComponent() const
	{
		return WeaponComponent;
	}

	/** 사망 처리를 시작했습니다(State.Dead). */
	bool IsDead() const;

	/** PP를 잃을 수 있습니다: 그로기 면역(State.GroggyImmune)도 그로기 중도 아닙니다(전투 시스템 'PP 감소량 계산식'). */
	bool CanLosePP() const;

	/** 플레이어 사양 '공격 방향': 추적 구간에 행동 대상이 없거나 타겟팅 범위 밖일 때 향해 돌 대상입니다. 없으면 null(정면)입니다. */
	virtual AActor* GetFallbackFacingTarget() const
	{
		return nullptr;
	}

	/** 이동 입력의 방향(카메라 기준). 이동 입력이 없거나 입력을 받지 않는 캐릭터는 0입니다. */
	virtual FVector GetMoveInputDirection() const
	{
		return FVector::ZeroVector;
	}

	/** 자동 회전을 하지 않습니다: 행동이 실행 중이거나(재생 중인 Runner) 비전투입니다. */
	bool IsFacingFixed() const;

	/** 움찔: 하던 동작 위에 움찔을 덧입혀 재생합니다. */
	void PlayFlinch();

	/** 가드 움찔: 가드 자세 위에 가드 움찔을 덧입혀 재생합니다. 가드 움찔 몽타주가 없는 캐릭터는 재생하지 않습니다. */
	void PlayGuardFlinch();

	/**
	 * 사망 처리: 모든 행동과 입력, 이동을 멈추고, 이 캐릭터의 판정 구간을 없애고, 캐릭터 간 충돌을 끈 뒤 PushVelocity로
	 * 밀리는 래그돌이 됩니다. 끝에 OnDied를 알립니다. bRagdoll이 거짓이면 지금 모션을 멈추지 않고(사망 모션), 래그돌은
	 * 사망 모션이 끝날 때 시작합니다.
	 */
	virtual void Die(const FVector& PushVelocity, bool bRagdoll = true);

	/** 사망 처리: 죽은 캐릭터를 래그돌로 바꾸고 PushVelocity로 밉니다. */
	void StartRagdoll(const FVector& PushVelocity);

	/** HP를 0으로 만드는 GE를 적용해 사망 판정을 일으킵니다. (Kill Z 낙하, 테스트용 콘솔 명령) */
	void Kill();

	/** 시간 정지 중에 이 캐릭터만 정상 속도로 돌도록 시간 배율에 곱하는 값. 시간 서브시스템이 정하고, 평소에는 1입니다. */
	void SetTimeCompensation(float Compensation);

	/** 사망할 때 한 번 알립니다. */
	FAgCharacterDiedSignature OnDied;

protected:
	virtual void PreInitializeComponents() override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;

	/**
	 * 플레이가 끝나면 어빌리티를 모두 취소하고 직접 재생한 몽타주의 콜백을 끊어, 컴포넌트가 등록 해제되는 동안 어빌리티와
	 * 사망 모션이 돌지 않게 합니다.
	 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Tick(float DeltaSeconds) override;
	virtual void PossessedBy(AController* NewController) override;

	/** 착지하면 자기에게 Event.Landed를 보냅니다. 이동 모드가 바뀌기 전(아직 공중 상태)에 불립니다. */
	virtual void Landed(const FHitResult& Hit) override;

	/** 공중 상태(State.Airborne): 이동 모드가 떨어지는 중인 동안입니다. */
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;

	/** 레벨의 Kill Z 아래로 떨어지면 HP를 0으로 만들어 사망 처리합니다. (전투 시스템 '사망') */
	virtual void FellOutOfWorld(const UDamageType& DamageType) override;

	/** 캐릭터 데이터의 어빌리티 목록(Abilities)의 항목마다 Runner 스펙을 부여하고, 행동 정의의 트리거를 스펙에 넣습니다. */
	virtual void GrantAbilities();

	/** 캐릭터 데이터의 시작할 때 거는 GameplayEffect(Effects)를 겁니다. */
	void ApplyEffects();

	/** 자기에게 게임플레이 이벤트를 보냅니다(이동 사건). */
	void SendEventToSelf(const FGameplayTag& EventTag);

	/** MOV에 곱하는 이동 속도 배율. 플레이어는 가드와 달리기가 바꿉니다. */
	virtual float GetMoveSpeedMultiplier() const
	{
		return 1.f;
	}

	/** 자동 회전의 대상. 보스는 AI 대상을 반환합니다. */
	virtual AActor* GetFacingTarget() const
	{
		return nullptr;
	}

	/** MOV × GetMoveSpeedMultiplier()를 캐릭터 무브먼트의 최대 걷기 속도에 넣습니다. */
	void UpdateMoveSpeed();

	/** 리젠 대기를 처음부터 다시 셉니다: 도는 대기를 지우고 대기 효과(Effects.json)를 새로 겁니다. */
	void RestartRegenDelay(FActiveGameplayEffectHandle& Handle, EAgEffect DelayEffect);

	/** Characters.json의 캐릭터 ID. 목록에는 이 캐릭터 클래스의 데이터 클래스에 맞는 ID만 나옵니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Data", meta = (GetOptions = "GetCharacterDataIds"))
	FName CharacterDataId;

	/** 이 캐릭터 클래스가 쓰는 데이터 클래스. 캐릭터 코드가 그 클래스의 필드를 읽으므로 생성자에서 정합니다. */
	TSubclassOf<UAgCharacterData> CharacterDataClass;

	/** CharacterDataId의 데이터. 컴포넌트를 초기화하기 전에 연결합니다. */
	UPROPERTY(Transient)
	TObjectPtr<UAgCharacterData> CharacterData;

	/** EndPlay가 시작됐습니다. 그 뒤의 몽타주 콜백은 무시합니다. */
	bool bEndingPlay = false;

private:
	/** 블루프린트에서 CharacterDataId를 고르는 목록 */
	UFUNCTION()
	TArray<FString> GetCharacterDataIds() const;

	/** 스탯을 캐릭터 데이터의 초기값으로 정합니다. */
	void InitializeAttributes();

	void HandleMOVChanged(const FOnAttributeChangeData& Data);

	/** PP 리젠: PP가 줄 때마다 리젠 대기를 다시 시작합니다. */
	void HandlePPChanged(const FOnAttributeChangeData& Data);

	/** 히트스톱 태그가 붙고 빠질 때 이 캐릭터의 시간 배율을 바꿉니다. */
	void HandleHitstopChanged(const FGameplayTag Tag, int32 NewCount);

	/** 캐릭터 통과(State.PassThrough): 그 태그가 있는 동안 캡슐이 Pawn 채널을 무시합니다. */
	void HandlePassThroughChanged(const FGameplayTag Tag, int32 NewCount);

	/** 시간 정지(State.TimeStop): 그 태그가 있는 동안 시간 서브시스템에 자기를 뺀 시간 정지를 요청합니다. */
	void HandleTimeStopChanged(const FGameplayTag Tag, int32 NewCount);

	/**
	 * 사망(Event.Death): 캐릭터 데이터의 사망 모션(DeathMotions)의 Montage를 재생 중이고 공격 쪽 태그에 KilledBy가 있으면 그
	 * 위치에서 Death 모션을 이어 재생한 뒤 래그돌이 되고, 아니면 공격 쪽에서 밀려나는 래그돌이 됩니다.
	 */
	void HandleDeathEvent(const FGameplayEventData* Payload);
	void HandleDeathMotionBlendingOut(UAnimMontage* Montage, bool bInterrupted);

	/** 시간 배율 = 히트스톱 배율 × 시간 정지 보정 */
	void UpdateTimeDilation();

	/** SetTimeCompensation으로 정한 시간 정지 보정 */
	float TimeCompensation = 1.f;

	/** 최소 간격: 재생 중인 행동에 Targeting 대상이 있으면 루트 모션 이동이 그 행동의 간격보다 가까이 가지 않게 자릅니다. */
	FTransform ClampRootMotion(const FTransform& WorldRootMotion, UCharacterMovementComponent* Movement, float DeltaSeconds);

	/** 자동 회전: 행동하지 않는 동안 GetFacingTarget을 향해 돕니다. */
	void UpdateFacing(float DeltaSeconds);

	/** 공중에서 수직 속도가 오름에서 내림으로 바뀌면 Event.Apex를 보냅니다. */
	void UpdateApex();

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

	FActiveGameplayEffectHandle PPRegenDelay;

	/** 어빌리티 목록으로 부여한 스펙마다 그 어빌리티 ID */
	TMap<FGameplayAbilitySpecHandle, FName> AbilityIds;

	/** 캐릭터 통과가 끝나면 되돌릴 캡슐의 Pawn 채널 응답 */
	TEnumAsByte<ECollisionResponse> PawnResponse = ECR_Block;

	/** 앞 틱의 수직 속도(공중일 때만) */
	float PreviousVelocityZ = 0.f;

	/** 재생 중인 사망 모션. 없으면 INDEX_NONE입니다. */
	int32 DeathMotionInstanceId = INDEX_NONE;
	FDelegateHandle DeathEventHandle;
};
