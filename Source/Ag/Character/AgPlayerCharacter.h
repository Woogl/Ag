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

/** 플레이어 캐릭터(플레이어 사양). 카메라와 락온을 가지고, 입력을 이동과 행동으로 바꿉니다. */
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

	/**
	 * 대상 찾기(구현 계획서 '대상 찾기'): TP_Enemy가 모은 후보 가운데 적대 관계이고 살아 있고, 캡슐 중심 사이 수평 거리가
	 * Range 안이며 Condition을 만족하는 가장 가까운 적입니다. 없으면 null입니다.
	 */
	AAgCharacterBase* FindEnemy(float Range, TFunctionRef<bool(const AAgCharacterBase&)> Condition) const;

	/** 대상 찾기: TP_Enemy가 모은 후보 가운데 적대 관계이고 살아 있고, 캡슐 중심 사이 수평 거리가 Range 안인 적 모두입니다. */
	TArray<AAgCharacterBase*> FindEnemies(float Range) const;

	/** 공격 중 이동의 대상은 거리와 관계없이 가장 가까운 적입니다. 타겟팅 범위는 워핑 Modifier가 공격마다 따집니다. */
	virtual AAgCharacterBase* FindAttackTarget() const override;

	/** 범위 안에 공격 대상이 없는 타는 락온 중일 때만 락온 대상 쪽으로 돕니다. */
	virtual AActor* GetFallbackFacingTarget() const override;

	/** 실행 중인 락온 Task가 보관한 대상입니다. */
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
	FVector GetMoveInputDirection() const
	{
		return MoveInputDirection;
	}

	bool IsGuardHeld() const
	{
		return bGuardHeld;
	}

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual float GetMoveSpeedMultiplier() const override;

private:
	void Move(const FInputActionValue& Value);

	/** 점프: 제자리 착지 뒤 움직이면 착지 모션을 끝냅니다. */
	void StopLandingMotion();
	void MoveReleased();
	void Look(const FInputActionValue& Value);

	/** 누른 행동 키를 입력 태그로 처리합니다(플레이어 입력 '키 배치', '공통 입력 처리'). 입력 바인딩이 태그를 함께 넘깁니다. */
	void HandleInputPressed(FGameplayTag InputTag);

	/** 행동 키를 처리합니다. 그 키를 받는 어빌리티가 실행 중이면 그 어빌리티가 받고, 아니면 허용될 때 그 행동을 시작합니다. */
	void RunInput(const FGameplayTag& InputTag);

	/** 뗀 키를 처리합니다(가드 키와 회피·달리기 키만 봅니다). */
	void HandleInputReleased(FGameplayTag InputTag);

	/** 선입력: 이 행동 키가 지금 허용되지 않으면 참입니다(후딜 전의 행동, 피격 반응, 공중에서는 평타 키 밖의 키). */
	bool IsActionInputBlocked(const FGameplayTag& InputTag) const;

	/** 선입력: 행동 키가 다시 허용되면 기억한 키를 실행합니다. 가드 키를 누른 채 후딜이 시작되면 가드를 시작합니다. */
	void HandleInputBlockChanged(const FGameplayTag Tag, int32 NewCount);
	void TryRunBufferedInput();

	/** 이동할 수 있으면 참입니다. 행동 모션, 가드 시작 직후, 피격 반응, 그로기, 처형, 사망 중에는 이동 입력을 무시합니다. */
	bool CanMove() const;

	/** 가드 시작: 가드 키를 누른 채 행동이나 피격 반응이 끝나면 가드를 다시 시작합니다. 가드 밀림 뒤에는 패리 구간을 열지 않습니다. */
	void HandleAbilityEnded(const FAbilityEndedData& EndedData);
	void TryResumeGuard(bool bAfterGuardPushback);

	/** 회피 반격 기회: 이동 외의 행동이나 넉업·넉백이 시작되면 사라집니다. */
	void HandleAbilityActivated(UGameplayAbility* Ability);

	/** 달리기: 회피 후딜에 Shift와 이동 입력이 있으면 시작하고, Shift를 떼거나 이동 입력이 없거나 SP가 0이 되면 끝냅니다. */
	void UpdateSprint();

	/** 가드, 달리기, 피격 반응 태그가 바뀌면 바라보는 방향과 이동 속도를 다시 정합니다. */
	void HandleMoveStateChanged(const FGameplayTag Tag, int32 NewCount);

	/** 자원 회복: SP가 줄 때마다 SP 리젠 대기를 다시 시작합니다. SP가 0이 되면 더 긴 대기입니다. */
	void HandleSPChanged(const FOnAttributeChangeData& Data);

	/** 시간 정지 동안 화면을 흑백으로 합니다. */
	void HandleTimeStopChanged(bool bStopped);

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

#if WITH_EDITORONLY_DATA
	/**
	 * 에디터 전용 확인용: PIE 중 에디터 도구로 넣으면 이 행동 키를 한 번 눌렀다 뗍니다(예: Input.Attack). 키보드 없이
	 * 행동을 확인합니다. 쓰고 나면 비웁니다.
	 */
	UPROPERTY(EditInstanceOnly, Transient, Category = "Dev", meta = (Categories = "Input"))
	FGameplayTag DevPressInput;

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
