// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/AgCharacterBase.h"
#include "GameplayTagContainer.h"
#include "AgPlayerCharacter.generated.h"

class AAgBossCharacter;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
struct FInputActionValue;

/** The player character (플레이어 사양). Owns the camera and turns input into movement and actions. */
UCLASS(Abstract)
class AAgPlayerCharacter : public AAgCharacterBase
{
	GENERATED_BODY()

public:
	AAgPlayerCharacter();

	/** The living boss whose capsule center is within Range (horizontal), found through TP_Boss. */
	AAgBossCharacter* FindBoss(float Range) const;

	/**
	 * 공격 방향과 공격 중 이동 for one action: keeps the stop distance from the boss, and when the boss is within
	 * TargetingRange, closes in on it and turns toward it until the attack window starts.
	 */
	void PrepareAttackMovement(float TargetingRange);

	/** Ends the attack movement of the finished action. */
	void ClearAttackMovement();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	/** Applies DA_Camera's 자유 시점 values to the spring arm and camera. */
	void ApplyCameraData();

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void AttackPressed();

	/** Handles a pressed action key by its input tag (플레이어 사양 '조작'). */
	void HandleInputPressed(const FGameplayTag& InputTag);

	/** False while an action motion, a hit reaction, groggy, execution or death ignores movement input. */
	bool CanMove() const;

	/** Pulls the camera in when terrain blocks it (카메라 '카메라 충돌'). */
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	/** IMC_Ag: the key layout of 플레이어 사양 '조작'. */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> InputMapping;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> AttackAction;

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

	/** Editor-only check helper: movement input applied every frame while not zero (X right, Y forward). */
	UPROPERTY(EditInstanceOnly, Transient, Category = "Dev")
	FVector2D DevMoveInput = FVector2D::ZeroVector;
#endif
};
