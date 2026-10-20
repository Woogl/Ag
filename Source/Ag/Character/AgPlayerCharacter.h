// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/AgCharacterBase.h"
#include "GameplayTagContainer.h"
#include "AgPlayerCharacter.generated.h"

class UCameraComponent;
class UGameplayAbility;
class UInputAction;
class UInputMappingContext;
class UPostProcessComponent;
class USpringArmComponent;
struct FAbilityEndedData;
struct FInputActionValue;

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

	/** 한 캐릭터에서 태그마다 행동이 하나 이하입니다(데이터 검증이 같은 표를 읽음). */
	static TConstArrayView<FGameplayTag> GetInputAbilityTags();

	static const FName JumpMontageId;
	static const FName JumpStartSection;
	static const FName JumpLandSection;

	void UpdateRotationMode();

	const UInputMappingContext* GetInputMapping() const
	{
		return InputMapping;
	}

	USpringArmComponent* GetCameraBoom() const
	{
		return CameraBoom;
	}

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

	virtual void OnJumped_Implementation() override;

	virtual void Landed(const FHitResult& Hit) override;

private:
	void Move(const FInputActionValue& Value);

	void StopLandingMotion();
	void MoveReleased();
	void Look(const FInputActionValue& Value);

	void HandleInputPressed(FGameplayTag InputTag);

	void RunInput(const FGameplayTag& InputTag);

	void HandleInputReleased(FGameplayTag InputTag);

	void ActivateByTag(const FGameplayTag& AbilityTag);

	void TryJump();

	void SetTriggerTag(const FGameplayTag& TriggerTag, const FGameplayTag& AbilityTag, bool bWant);

	bool IsActionInputBlocked(const FGameplayTag& InputTag) const;

	void HandleInputBlockChanged(const FGameplayTag Tag, int32 NewCount);
	void TryRunBufferedInput();

	bool CanMove() const;

	void HandleAbilityEnded(const FAbilityEndedData& EndedData);

	void TryResumeGuard();

	void HandleAbilityActivated(UGameplayAbility* Ability);

	void UpdateSprint();

	void UpdateLockOn();

	void ToggleLockOn();

	void UpdateLockOnCamera(const AAgCharacterBase& Target, float DeltaTime);

	void HandleMoveStateChanged(const FGameplayTag Tag, int32 NewCount);

	void UpdateTimeStopScreen(bool bStopped);

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, Category = "Time Stop")
	TObjectPtr<UPostProcessComponent> TimeStopScreen;

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

	FGameplayTag BufferedInput;
	double BufferedInputTime = 0.0;

	bool bGuardHeld = false;
	bool bDodgeHeld = false;

	TWeakObjectPtr<AAgCharacterBase> LockOnCameraTarget;

	float LockOnPitchRate = 0.f;
	float LockOnYawRate = 0.f;

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
