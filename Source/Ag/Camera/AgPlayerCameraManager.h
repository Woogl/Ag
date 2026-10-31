// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/AgCameraReactionSettings.h"
#include "GameplayTagContainer.h"
#include "AgPlayerCameraManager.generated.h"

class AAgCharacterBase;
class AAgPlayerCharacter;
class UAgAbilitySystemComponent;
class UAgExecutionCameraModifier;
class UAgFieldOfViewCameraModifier;
class UGameplayAbility;
struct FAbilityEndedData;

UCLASS(NotBlueprintable)
class AAgPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

public:
	AAgPlayerCameraManager();

	virtual void InitializeFor(APlayerController* PC) override;

	void UpdateGameplayRotation(const AAgPlayerCharacter& Player, const FVector& PivotLocation, float DeltaTime);
	void ApplyLookInput(const FVector2D& Input);
	void PlayCameraReaction(EAgCameraSituation Situation, const AActor* TurnTarget);

protected:
	virtual void PostInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	AAgPlayerCharacter* GetPlayerCharacter() const;
	void SetObservedAbilitySystem(UAgAbilitySystemComponent* NewAbilitySystem);
	void ResetCameraState();

	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	void UpdateLockOnCamera(const AAgCharacterBase& Target, const FVector& PivotLocation, float DeltaTime);
	void StartCameraTurn(const AActor& Opponent, float Angle);
	void UpdateCameraTurn(const FVector& PivotLocation, float DeltaTime);
	bool IsExecutionCameraActive() const;
	void HandleAbilityCommitted(UGameplayAbility* Ability);
	void HandleAbilityEnded(const FAbilityEndedData& EndedData);
	void HandleSprintFieldOfViewChanged(const FGameplayTag Tag, int32 NewCount);
	void HandleCutsceneChanged(const FGameplayTag Tag, int32 NewCount);

#if WITH_EDITOR
	void HandleCameraDataReloaded();
#endif

	UPROPERTY(Transient)
	TObjectPtr<UAgExecutionCameraModifier> ExecutionCamera;

	UPROPERTY(Transient)
	TObjectPtr<UAgFieldOfViewCameraModifier> FieldOfViewCamera;

	TWeakObjectPtr<UAgAbilitySystemComponent> ObservedAbilitySystem;
	TWeakObjectPtr<AAgCharacterBase> PreviousLockOnTarget;
	float LockOnPitchRate = 0.f;
	float LockOnYawRate = 0.f;
	TWeakObjectPtr<const AActor> TurnOpponent;
	float TurnAngle = 0.f;
	float TurnYawRate = 0.f;
	uint64 TurnStartFrame = 0;
};
