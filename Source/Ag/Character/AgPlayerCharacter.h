// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/AgCharacterBase.h"
#include "Camera/AgCameraEffect.h"
#include "GameplayTagContainer.h"
#include "AgPlayerCharacter.generated.h"

class UAgExecutionCameraModifier;
class UAgFieldOfViewCameraModifier;
class UCameraComponent;
class UCapsuleComponent;
class UGameplayAbility;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
struct FAbilityEndedData;
struct FInputActionValue;

UCLASS(Abstract)
class AAgPlayerCharacter : public AAgCharacterBase
{
	GENERATED_BODY()

public:
	AAgPlayerCharacter();

	void PlayCameraEffect(EAgCameraSituation Situation, const AActor* Opponent);

	/** 플레이어가 공격자이거나 대상인 피격 결과에만 연출을 재생합니다. Reaction은 대상에게 실제로 보낸 피격 반응 이벤트입니다. */
	void PlayDamageCameraEffects(const FGameplayTagContainer& SpecTags, const FGameplayTag& Reaction, const AAgCharacterBase& Attacker, const AAgCharacterBase& Target);

	virtual FVector GetMoveInputDirection() const override
	{
		return MoveInputDirection;
	}

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	virtual void OnJumped_Implementation() override;

	virtual void Landed(const FHitResult& Hit) override;

	virtual void ApplyDeathState() override;

private:
	void UpdateRotationMode();

	void Move(const FInputActionValue& Value);

	void StopLandingMotion();
	void MoveReleased();
	void Look(const FInputActionValue& Value);

	void HandleInputPressed(FGameplayTag InputTag);

	void RunInput(const FGameplayTag& InputTag);

	void HandleInputReleased(FGameplayTag InputTag);

	void ActivateByTag(const FGameplayTag& AbilityTag);

	void TryJump();

	bool IsActionInputBlocked(const FGameplayTag& InputTag) const;

	void HandleInputBlockChanged(const FGameplayTag Tag, int32 NewCount);
	void TryRunBufferedInput();

	bool CanMove() const;

	void HandleAbilityEnded(const FAbilityEndedData& EndedData);

	void TryRunPendingInput();

	void HandleAbilityActivated(UGameplayAbility* Ability);

	void HandleAbilityCommitted(UGameplayAbility* Ability);

	void UpdateSprint();

	void UpdateLockOn();

	void ToggleLockOn();

	void UpdateLockOnCamera(const AAgCharacterBase& Target, float DeltaTime);

	void StartCameraTurn(const AActor& Opponent, float Angle);

	void UpdateCameraTurn(float DeltaTime);

	bool IsExecutionCameraActive() const;

	void HandleMoveStateChanged(const FGameplayTag Tag, int32 NewCount);

	void HandleSprintFieldOfViewChanged(const FGameplayTag Tag, int32 NewCount);

	void HandlePerfectDodgeWindowChanged(const FGameplayTag Tag, int32 NewCount);

	void HandleTimeStopChanged(const FGameplayTag Tag, int32 NewCount);

	void HandleCutsceneChanged(const FGameplayTag Tag, int32 NewCount);

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	TObjectPtr<UCapsuleComponent> PerfectDodgeCapsule;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Lock-On")
	float LockOnPitchMin = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Lock-On")
	float LockOnPitchMax = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Lock-On")
	float LockOnLookDown = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Lock-On")
	float LockOnRotationSpeed = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Lock-On")
	float LockOnSmoothingTime = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Combat")
	TMap<EAgCameraSituation, FAgCameraEffect> CameraEffects;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Sprint")
	FAgFieldOfViewEffect SprintFieldOfView;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Execution")
	TSubclassOf<UAgExecutionCameraModifier> ExecutionCameraClass;

	UPROPERTY(Transient)
	TObjectPtr<UAgExecutionCameraModifier> ExecutionCamera;

	UPROPERTY(Transient)
	TObjectPtr<UAgFieldOfViewCameraModifier> FieldOfViewCamera;

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

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	float InputBufferTime = 0.3f;

	FVector MoveInputDirection = FVector::ZeroVector;

	FGameplayTag BufferedInput;
	double BufferedInputTime = 0.0;

	bool bGuardHeld = false;
	bool bDodgeHeld = false;

	TWeakObjectPtr<AAgCharacterBase> LockOnCameraTarget;

	float LockOnPitchRate = 0.f;
	float LockOnYawRate = 0.f;

	TWeakObjectPtr<const AActor> TurnOpponent;
	float TurnAngle = 0.f;
	float TurnYawRate = 0.f;
	uint64 TurnStartFrame = 0;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditInstanceOnly, Transient, Category = "Dev", meta = (Categories = "Input"))
	FGameplayTag DevPressInput;

	FGameplayTag DevReleaseInput;

	UPROPERTY(EditInstanceOnly, Transient, Category = "Dev", meta = (Categories = "Input"))
	FGameplayTag DevMashInput;

	UPROPERTY(EditInstanceOnly, Transient, Category = "Dev", meta = (Categories = "Input"))
	FGameplayTag DevHoldInput;

	UPROPERTY(EditInstanceOnly, Transient, Category = "Dev")
	FVector2D DevMoveInput = FVector2D::ZeroVector;

	FGameplayTag DevHeldInput;
	bool bDevMoving = false;
#endif
};
