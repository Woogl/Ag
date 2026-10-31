// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/AgCharacterBase.h"
#include "GameplayTagContainer.h"
#include "AgPlayerCharacter.generated.h"

class AAgPlayerCameraManager;
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

	AAgPlayerCameraManager* GetPlayerCameraManager() const;

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

	void UpdateSprint();

	void ToggleLockOn();

	void HandleMoveStateChanged(const FGameplayTag Tag, int32 NewCount);

	void HandlePerfectDodgeWindowChanged(const FGameplayTag Tag, int32 NewCount);

	void HandleTimeStopChanged(const FGameplayTag Tag, int32 NewCount);

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	TObjectPtr<UCapsuleComponent> PerfectDodgeCapsule;

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
