// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/AgCharacterBase.h"
#include "GameplayTagContainer.h"
#include "AgPlayerCharacter.generated.h"

class AAgBossCharacter;
class UAgLockOnComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
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

	/** The living boss whose capsule center is within Range (horizontal), found through TP_Boss. */
	AAgBossCharacter* FindBoss(float Range) const;

	/**
	 * 공격 방향과 공격 중 이동 for one hit: keeps the stop distance from the boss, and when the boss is within
	 * TargetingRange, closes in on it and turns toward it until the attack window starts.
	 */
	void PrepareAttackMovement(float TargetingRange);

	/** Ends the attack movement of the finished action. */
	void ClearAttackMovement();

	UAgLockOnComponent* GetLockOn() const { return LockOn; }

	/** The direction the movement keys point, relative to the camera (zero without movement input). */
	FVector GetMoveInputDirection() const { return MoveInputDirection; }

	bool IsGuardHeld() const { return bGuardHeld; }

	/**
	 * True once when the guard starts again after a parry motion or a guard pushback with the key held,
	 * which opens no parry window (플레이어 사양 '패리 구간').
	 */
	bool TakeGuardResume();

	virtual void PlayGuardFlinch() override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void StartRegen() override;
	virtual float GetMoveSpeedMultiplier() const override;

private:
	/** Applies DA_Camera's 자유 시점 values to the spring arm and camera. */
	void ApplyCameraData();

	void Move(const FInputActionValue& Value);
	void MoveReleased();
	void Look(const FInputActionValue& Value);
	void AttackPressed();
	void GuardPressed();
	void GuardReleased();
	void DodgePressed();
	void SkillPressed();
	void UltimatePressed();
	void ExecutePressed();
	void LockOnPressed();

	/** Handles a pressed action key by its input tag (플레이어 사양 '조작', '공통 입력 처리'). */
	void HandleInputPressed(const FGameplayTag& InputTag);

	/** Handles a released key (only the guard key cares). */
	void HandleInputReleased(const FGameplayTag& InputTag);

	/** False while an action motion, a hit reaction, groggy, execution or death ignores movement input. */
	bool CanMove() const;

	/** 가드 시작: with the guard key held, the guard starts again once an action or a hit reaction ends. */
	void HandleAbilityEnded(const FAbilityEndedData& EndedData);
	void TryResumeGuard();

	/** Faces the movement direction normally; faces the camera (the lock-on target) while locked on or guarding. */
	void UpdateRotationMode();

	void HandleGuardingChanged(const FGameplayTag Tag, int32 NewCount);

	/** 자원 회복: any SP use restarts the SP regen delay, a longer one when SP reaches 0. */
	void HandleSPChanged(const FOnAttributeChangeData& Data);

	/** Pulls the camera in when terrain blocks it (카메라 '카메라 충돌'). */
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, Category = "Lock-On")
	TObjectPtr<UAgLockOnComponent> LockOn;

	/** IMC_Ag: the key layout of 플레이어 사양 '조작'. */
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
	TObjectPtr<UInputAction> SkillAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> UltimateAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ExecuteAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LockOnAction;

	FVector MoveInputDirection = FVector::ZeroVector;
	bool bGuardHeld = false;
	bool bGuardResumeWithoutParry = false;
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
