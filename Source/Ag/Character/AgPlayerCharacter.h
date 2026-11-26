// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/AgCharacterBase.h"
#include "GameplayTagContainer.h"
#include "AgPlayerCharacter.generated.h"

class UAgCameraData;
class UAgCombatStatsComponent;
class UAgGameplayAbility;
class UAgInteractionComponent;
class UCameraComponent;
class UGameplayAbility;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
struct FInputActionValue;

UCLASS(Abstract)
class AAgPlayerCharacter : public AAgCharacterBase
{
	GENERATED_BODY()

public:
	AAgPlayerCharacter();

	USpringArmComponent* GetCameraBoom() const
	{
		return CameraBoom;
	}

	UAgCombatStatsComponent* GetCombatStats() const
	{
		return CombatStats;
	}

	virtual FVector GetMoveInputDirection() const override
	{
		return MoveInputDirection;
	}

	virtual bool IsMoveInputIgnored() const override;

	void FindInputAbilities(const FGameplayTag& InputTag, TArray<FGameplayAbilitySpecHandle>& OutHandles) const;

	/** 입력 하나에 어빌리티가 하나뿐인 곳(HUD의 스킬·퀵슬롯 버튼)에서 그 어빌리티를 찾습니다. */
	const UAgGameplayAbility* FindInputAbility(const FGameplayTag& InputTag) const;

	/** 락온 어빌리티의 대상을 화면에서 Direction 쪽(양수 = 오른쪽)의 가장 가까운 후보로 바꿉니다. 없으면 그대로 둡니다. */
	void SwitchLockOnTarget(float Direction);

	/** 체크포인트에서 휴식할 때의 회복입니다. HP와 휴식으로 채우는 아이템을 최대로 채우고, 남은 쿨다운을 없앱니다. */
	void Rest();

	const UAgCameraData& GetCameraData() const;

protected:
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	virtual bool CanJumpInternal_Implementation() const override;

	virtual void ApplyDeathState() override;

private:
	void UpdateRotationMode();

	void Move(const FInputActionValue& Value);

	void MoveReleased();
	void Look(const FInputActionValue& Value);

	void HandleInputPressed(FGameplayTag InputTag);

	void RunInput(const FGameplayTag& InputTag);

	bool ActivateInputAbilities(const FGameplayTag& InputTag);

	void HandleInputReleased(FGameplayTag InputTag);

	/** 이 입력에 키를 떼면 끝나는 어빌리티가 묶여 있습니다. */
	bool IsReleaseInput(const FGameplayTag& InputTag) const;

	void ActivateByTag(const FGameplayTag& AbilityTag);

	bool IsActionInputBlocked(const FGameplayTag& InputTag) const;

	void HandleInputBlockChanged(const FGameplayTag Tag, int32 NewCount);
	void HandleInputUnblocked();

	bool HasBufferedInput() const;
	void TryRunBufferedInput();

	void ReturnToGuardFromParry();

	void HandleAbilityActivated(UGameplayAbility* Ability);

	void TryMoveCancel();

	void ToggleLockOn();

	void HandleMoveStateChanged(const FGameplayTag Tag, int32 NewCount);

	void HandleGuardingChanged(const FGameplayTag Tag, int32 NewCount);

	void HandleTimeStopChanged(const FGameplayTag Tag, int32 NewCount);

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	/** 이 캐릭터가 붙잡고 있는 동안 카메라 반응의 셰이크 클래스가 메모리에 남습니다. */
	UPROPERTY(Transient)
	TObjectPtr<UAgCameraData> CameraData;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	TObjectPtr<UAgCombatStatsComponent> CombatStats;

	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	TObjectPtr<UAgInteractionComponent> Interaction;

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
	TObjectPtr<UInputAction> Skill1Action;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> Skill2Action;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> Skill3Action;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> Skill4Action;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> UltimateAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LockOnAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> PotionAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	float InputBufferTime = 0.3f;

	FVector MoveInputDirection = FVector::ZeroVector;

	FGameplayTag BufferedInput;
	double BufferedInputTime = 0.0;

	TSet<FGameplayTag> HeldInputs;
};
