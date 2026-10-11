// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/AgCharacterBase.h"
#include "GameplayTagContainer.h"
#include "AgPlayerCharacter.generated.h"

class UAgLockOnComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UPostProcessComponent;
class USpringArmComponent;
struct FAbilityEndedData;
struct FInputActionValue;

/** The player character (플레이어 사양). Owns the camera and lock-on, and turns input into movement and actions. */
UCLASS(Abstract)
class AAgPlayerCharacter : public AAgCharacterBase
{
	GENERATED_BODY()

public:
	AAgPlayerCharacter();

	/**
	 * 대상 찾기(구현 계획서 '대상 찾기'): TP_Enemy가 모은 후보 가운데 적대 관계이고 살아 있고, 캡슐 중심 사이 수평 거리가
	 * Range 안이며 Condition을 만족하는 가장 가까운 적입니다. 없으면 null입니다.
	 */
	AAgCharacterBase* FindEnemy(float Range, TFunctionRef<bool(const AAgCharacterBase&)> Condition) const;

	/** 대상 찾기: TP_Enemy가 모은 후보 가운데 적대 관계이고 살아 있고, 캡슐 중심 사이 수평 거리가 Range 안인 적 모두입니다. */
	TArray<AAgCharacterBase*> FindEnemies(float Range) const;

	/** 공격 중 이동의 대상은 거리와 관계없이 가장 가까운 적입니다. 타겟팅 범위는 공격 중 이동 태스크가 공격마다 따집니다. */
	virtual AAgCharacterBase* FindAttackTarget() const override;

	/** 범위 안에 공격 대상이 없는 타는 락온 중일 때만 락온 대상 쪽으로 돕니다. */
	virtual AActor* GetFallbackFacingTarget() const override;

	UAgLockOnComponent* GetLockOn() const { return LockOn; }

	/** IMC_Ag: 전투 HUD의 조작 안내가 이 매핑을 표시합니다. */
	const UInputMappingContext* GetInputMapping() const { return InputMapping; }

	/** The camera's spring arm. Its origin is the camera's 회전 중심. */
	USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** The direction the movement keys point, relative to the camera (zero without movement input). */
	FVector GetMoveInputDirection() const { return MoveInputDirection; }

	bool IsGuardHeld() const { return bGuardHeld; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual float GetMoveSpeedMultiplier() const override;

private:
	void Move(const FInputActionValue& Value);

	/** 점프: moving after a standing landing ends the landing motion. */
	void StopLandingMotion();
	void MoveReleased();
	void Look(const FInputActionValue& Value);

	/** 누른 행동 키를 입력 태그로 처리합니다(플레이어 입력 '키 배치', '공통 입력 처리'). 입력 바인딩이 태그를 함께 넘깁니다. */
	void HandleInputPressed(FGameplayTag InputTag);

	/** 행동 키를 처리합니다. 그 키를 받는 어빌리티가 실행 중이면 그 어빌리티가 받고, 아니면 허용될 때 그 행동을 시작합니다. */
	void RunInput(const FGameplayTag& InputTag);

	/** 뗀 키를 처리합니다(가드 키와 회피·달리기 키만 봅니다). */
	void HandleInputReleased(FGameplayTag InputTag);

	/** 선입력: 이 행동 키가 지금 허용되지 않으면 true입니다(후딜 전의 행동, 피격 반응, 공중에서는 평타 키 밖의 키). */
	bool IsActionInputBlocked(const FGameplayTag& InputTag) const;

	/** 선입력: once action keys are allowed again, the remembered key runs. */
	void HandleInputBlockChanged(const FGameplayTag Tag, int32 NewCount);
	void TryRunBufferedInput();

	/** False while an action motion, a hit reaction, groggy, execution or death ignores movement input. */
	bool CanMove() const;

	/** 가드 시작: 가드 키를 누른 채 행동이나 피격 반응이 끝나면 가드를 다시 시작합니다. 가드 밀림 뒤에는 패리 구간을 열지 않습니다. */
	void HandleAbilityEnded(const FAbilityEndedData& EndedData);
	void TryResumeGuard(bool bAfterGuardPushback);

	/** 회피 반격 기회: 이동 외의 행동이나 넉업·넉백이 시작되면 사라집니다. */
	void HandleAbilityActivated(UGameplayAbility* Ability);

	/** 달리기: 회피 후딜에 Shift와 이동 입력이 있으면 시작하고, Shift를 떼거나 이동 입력이 없거나 SP가 0이 되면 끝냅니다. */
	void UpdateSprint();

	/**
	 * Faces the movement direction normally; faces the camera (the lock-on target) while locked on or guarding.
	 * A sprint faces the movement direction even while locked on.
	 */
	void UpdateRotationMode();

	/** Guarding and sprinting change the facing and the movement speed. */
	void HandleMoveStateChanged(const FGameplayTag Tag, int32 NewCount);

	/** 자원 회복: any SP use restarts the SP regen delay, a longer one when SP reaches 0. */
	void HandleSPChanged(const FOnAttributeChangeData& Data);

	/** 시간 정지 동안 화면을 흑백으로 합니다. */
	void HandleTimeStopChanged(bool bStopped);

	/**
	 * 카메라 '자유 시점': distance, pivot height and follow lag are set on BP_AgPlayer, like the camera's FOV.
	 * Pulls the camera in when terrain blocks it (카메라 '카메라 충돌').
	 */
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, Category = "Lock-On")
	TObjectPtr<UAgLockOnComponent> LockOn;

	/** 플레이어 사양 '시간 정지'의 화면 흑백. 흑백 설정은 BP_AgPlayer에 두고, 코드는 시간 정지 동안 가중치만 1로 올립니다. */
	UPROPERTY(VisibleAnywhere, Category = "Time Stop")
	TObjectPtr<UPostProcessComponent> TimeStopScreen;

	/** IMC_Ag: the key layout of 플레이어 입력 '키 배치'. */
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

	/** 선입력: the key pressed while it wasn't allowed, and when (world time). */
	FGameplayTag BufferedInput;
	double BufferedInputTime = 0.0;

	bool bGuardHeld = false;
	bool bDodgeHeld = false;
	FActiveGameplayEffectHandle SPRegenDelay;

#if WITH_EDITORONLY_DATA
	/**
	 * Editor-only check helper: set from the editor tools during PIE to press an action once (for example Input.Attack),
	 * so actions can be checked without a keyboard. Cleared after use.
	 */
	UPROPERTY(EditInstanceOnly, Transient, Category = "Dev", meta = (Categories = "Input"))
	FGameplayTag DevPressInput;

	/** Editor-only check helper: presses this action every frame while set (mashing), for checking chains like the combo. */
	UPROPERTY(EditInstanceOnly, Transient, Category = "Dev", meta = (Categories = "Input"))
	FGameplayTag DevMashInput;

	/** Editor-only check helper: holds this action's key while set (for example Input.Guard), and releases it when cleared. */
	UPROPERTY(EditInstanceOnly, Transient, Category = "Dev", meta = (Categories = "Input"))
	FGameplayTag DevHoldInput;

	/** Editor-only check helper: movement input applied every frame while not zero (X right, Y forward). */
	UPROPERTY(EditInstanceOnly, Transient, Category = "Dev")
	FVector2D DevMoveInput = FVector2D::ZeroVector;

	FGameplayTag DevHeldInput;
	bool bDevMoving = false;
#endif
};
