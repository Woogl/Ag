// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/AgCharacterBase.h"
#include "GameplayTagContainer.h"
#include "AgPlayerCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UPostProcessComponent;
class USpringArmComponent;
struct FAbilityEndedData;
struct FInputActionValue;

/**
 * 플레이어 캐릭터(플레이어 사양). 카메라와 락온 카메라를 가지고, 입력 계층으로서 입력을 이동과 행동으로 바꿉니다. 행동은 행동 ID 대신
 * 행동 태그로 찾고(구현 계획서 '입력 계층의 행동 찾기'), 토글형 행동은 트리거 태그(Input.Guard, Input.Sprint, Input.LockOn)를
 * 붙이고 떼며, 점프는 엔진 점프와 점프 모션(State.Jumping)을 맡습니다.
 */
UCLASS(Abstract)
class AAgPlayerCharacter : public AAgCharacterBase
{
	GENERATED_BODY()

public:
	AAgPlayerCharacter();

	virtual FGenericTeamId GetGenericTeamId() const override
	{
		return FGenericTeamId(0);
	}

	/** 입력 계층이 행동을 찾는 행동 태그. 한 캐릭터에서 태그마다 행동이 하나 이하입니다(데이터 검증이 같은 표를 읽음). */
	static TConstArrayView<FGameplayTag> GetInputAbilityTags();

	/**
	 * 직접 재생하는 점프 몽타주(캐릭터 데이터 Montages의 ID)와 섹션(애니메이션과 이동 '점프'). 점프할 때 처음부터, 제자리 착지에
	 * Land부터 재생하며, 데이터 검증이 두 섹션이 있는지 확인합니다.
	 */
	static const FName JumpMontageId;
	static const FName JumpStartSection;
	static const FName JumpLandSection;

	/** 추적 구간에 범위 안의 행동 대상이 없는 타는 락온 중일 때만 락온 대상 쪽으로 돕니다. */
	virtual AActor* GetFallbackFacingTarget() const override;

	/** 실행 중인 락온 행동의 대상입니다. 락온 중이 아니면 null입니다. */
	AAgCharacterBase* GetLockOnTarget() const;

	/** 락온·가드·달리기·피격 상태에 맞춰 이동 회전 방식을 갱신합니다. */
	void UpdateRotationMode();

	/** IMC_Ag: 전투 HUD의 조작 안내가 이 매핑을 표시합니다. */
	const UInputMappingContext* GetInputMapping() const
	{
		return InputMapping;
	}

	/** 카메라의 스프링 암. 그 위치가 카메라의 회전 중심입니다. */
	USpringArmComponent* GetCameraBoom() const
	{
		return CameraBoom;
	}

	/** 이동 키가 가리키는 방향(카메라 기준). 이동 입력이 없으면 0입니다. */
	virtual FVector GetMoveInputDirection() const override
	{
		return MoveInputDirection;
	}

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual float GetMoveSpeedMultiplier() const override;

	/** 점프가 실제로 시작됨: State.Jumping을 붙여 점프 행동을 시작하고 점프 모션의 Start 섹션을 재생합니다. */
	virtual void OnJumped_Implementation() override;

	/** 착지: 점프 행동이 남아 있으면 제자리 착지에 점프 모션의 Land 섹션을 재생하고, State.Jumping을 뗍니다. */
	virtual void Landed(const FHitResult& Hit) override;

private:
	void Move(const FInputActionValue& Value);

	/** 점프: 제자리 착지 뒤 움직이면 착지 모션을 끝냅니다. */
	void StopLandingMotion();
	void MoveReleased();
	void Look(const FInputActionValue& Value);

	/** 누른 행동 키를 입력 태그로 처리합니다(플레이어 입력 '키 배치', '공통 입력 처리'). 입력 바인딩이 태그를 함께 넘깁니다. */
	void HandleInputPressed(FGameplayTag InputTag);

	/** 행동 키를 처리합니다. 그 키를 받는 행동이 실행 중이면 그 행동에 입력 이벤트를 보내고, 아니면 허용될 때 그 행동을 시작합니다. */
	void RunInput(const FGameplayTag& InputTag);

	/** 뗀 키를 처리합니다(가드, 회피·달리기, 점프 키만 봅니다). */
	void HandleInputReleased(FGameplayTag InputTag);

	/** 행동 태그의 행동을 시작합니다. 없으면 아무것도 하지 않습니다. */
	void ActivateByTag(const FGameplayTag& AbilityTag);

	/** 점프 행동을 시작할 수 있을 때만 엔진 Jump를 부릅니다(트리거 태그 State.Jumping은 점프가 실제로 시작될 때 붙음). */
	void TryJump();

	/**
	 * 트리거 태그(OwnedTagPresent)의 세 규칙(구현 계획서 '어빌리티와 AbilityTask'의 '트리거'): 1. bWant면 AbilityTag의 행동을
	 * 시작할 수 있을 때만 TriggerTag를 붙이고, 2. 붙였는데 시작하지 않았으면 바로 떼며, 3. 붙어 있는데 그 행동이 끝났거나 bWant가
	 * 거짓이면 뗍니다.
	 */
	void SetTriggerTag(const FGameplayTag& TriggerTag, const FGameplayTag& AbilityTag, bool bWant);

	/** 선입력: 이 행동 키가 지금 허용되지 않으면 참입니다(후딜 전의 행동, 피격 반응, 공중에서는 평타 키 밖의 키). */
	bool IsActionInputBlocked(const FGameplayTag& InputTag) const;

	/** 선입력: 행동 키가 다시 허용되면 기억한 키를 실행합니다. 가드 키를 누른 채 후딜이 시작되면 가드로 돌아옵니다. */
	void HandleInputBlockChanged(const FGameplayTag Tag, int32 NewCount);
	void TryRunBufferedInput();

	/** 이동할 수 있으면 참입니다. 행동 모션(State.Acting), 가드 시작 직후, 피격 반응, 그로기, 처형, 사망 중에는 이동 입력을 무시합니다. */
	bool CanMove() const;

	/** 가드 키를 누른 채 행동이나 피격 반응이 끝나면 가드로 돌아오고, 끝난 토글형 행동의 트리거 태그를 뗍니다. */
	void HandleAbilityEnded(const FAbilityEndedData& EndedData);

	/** 가드 키를 누르고 있으면 가드를 시작하고, 가드 중이면 후딜에 든 패리 모션을 끝내 가드 자세로 돌아옵니다. */
	void TryResumeGuard();

	/** 회피 반격 기회: 이동 외의 행동이나 넉업·넉백이 시작되면 사라집니다. */
	void HandleAbilityActivated(UGameplayAbility* Ability);

	/** 달리기: 회피 후딜에 Shift와 이동 입력이 있고 SP가 있으면 시작하고, Shift를 떼거나 이동 입력이 없거나 SP가 0이 되면 끝냅니다. */
	void UpdateSprint();

	/** 락온: 대상이 죽거나 해제 거리 밖으로 나가면 끝내고, 락온 중이면 카메라를 대상 쪽으로 돌립니다. */
	void UpdateLockOn();

	/** 락온 키(토글): 락온 중이면 끝내고, 아니면 시작할 수 있을 때 시작합니다. */
	void ToggleLockOn();

	/** 컨트롤 회전을 락온 대상의 락온 지점 쪽으로 돌립니다(카메라 '구도', '회전'). */
	void UpdateLockOnCamera(const AAgCharacterBase& Target, float DeltaTime);

	/** 가드, 달리기, 피격 반응 태그가 바뀌면 바라보는 방향과 이동 속도를 다시 정합니다. */
	void HandleMoveStateChanged(const FGameplayTag Tag, int32 NewCount);

	/** 자원 회복: SP가 줄 때마다 SP 리젠 대기를 다시 시작합니다. SP가 0이 되면 더 긴 대기입니다. */
	void HandleSPChanged(const FOnAttributeChangeData& Data);

	/** 시간 정지 동안 화면을 흑백으로 합니다. */
	void UpdateTimeStopScreen(bool bStopped);

	/**
	 * 카메라 '자유 시점': 거리, 회전 중심 높이, 위치 추적 지연은 카메라의 FOV처럼 BP_AgPlayer에 적습니다. 지형에 막히면
	 * 카메라를 당깁니다(카메라 '카메라 충돌').
	 */
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	/** 플레이어 사양 '시간 정지'의 화면 흑백. 흑백 설정은 BP_AgPlayer에 두고, 코드는 시간 정지 동안 가중치만 1로 올립니다. */
	UPROPERTY(VisibleAnywhere, Category = "Time Stop")
	TObjectPtr<UPostProcessComponent> TimeStopScreen;

	/** IMC_Ag: 플레이어 입력 '키 배치'의 매핑 */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> InputMapping;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> AttackAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> GuardAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> DodgeAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SkillAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> UltimateAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ExecuteAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LockOnAction;

	FVector MoveInputDirection = FVector::ZeroVector;

	/** 선입력: 허용되지 않을 때 누른 키와 누른 시각(월드 시간) */
	FGameplayTag BufferedInput;
	double BufferedInputTime = 0.0;

	bool bGuardHeld = false;
	bool bDodgeHeld = false;
	FActiveGameplayEffectHandle SPRegenDelay;

	/** 카메라가 따라가는 락온 대상. 바뀌면 카메라 회전 속도를 0에서 다시 시작합니다. */
	TWeakObjectPtr<AAgCharacterBase> LockOnCameraTarget;

	/** 락온 카메라의 회전 속도 (deg/s) */
	float LockOnPitchRate = 0.f;
	float LockOnYawRate = 0.f;

#if WITH_EDITORONLY_DATA
	/**
	 * 에디터 전용 확인용: PIE 중 에디터 도구로 넣으면 이 행동 키를 한 번 눌렀다 다음 프레임에 뗍니다(예: Input.Attack). 키보드
	 * 없이 행동을 확인합니다. 쓰고 나면 비웁니다.
	 */
	UPROPERTY(EditInstanceOnly, Transient, Category = "Dev", meta = (Categories = "Input"))
	FGameplayTag DevPressInput;

	/** DevPressInput으로 누른 키. 다음 프레임에 뗍니다. */
	FGameplayTag DevReleaseInput;

	/** 에디터 전용 확인용: 넣어 둔 동안 이 행동 키를 매 프레임 누릅니다(연타). 평타 콤보 같은 연계를 확인합니다. */
	UPROPERTY(EditInstanceOnly, Transient, Category = "Dev", meta = (Categories = "Input"))
	FGameplayTag DevMashInput;

	/** 에디터 전용 확인용: 넣어 둔 동안 이 행동 키를 누르고 있고(예: Input.Guard), 비우면 뗍니다. */
	UPROPERTY(EditInstanceOnly, Transient, Category = "Dev", meta = (Categories = "Input"))
	FGameplayTag DevHoldInput;

	/** 에디터 전용 확인용: 0이 아닌 동안 매 프레임 넣는 이동 입력 (X 오른쪽, Y 앞) */
	UPROPERTY(EditInstanceOnly, Transient, Category = "Dev")
	FVector2D DevMoveInput = FVector2D::ZeroVector;

	FGameplayTag DevHeldInput;
	bool bDevMoving = false;
#endif
};
