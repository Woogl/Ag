// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "Data/AgCameraData.h"
#include "GameplayTagContainer.h"
#include "AgPlayerCameraManager.generated.h"

class AAgPlayerCharacter;
class UAgAbilitySystemComponent;
class UAgExecutionCameraModifier;
class UAgFieldOfViewCameraModifier;
class UAgLookAtCameraModifier;
class UGameplayAbility;
struct FAbilityEndedData;

UCLASS(NotBlueprintable)
class AAgPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

public:
	AAgPlayerCameraManager();

	virtual void InitializeFor(APlayerController* PC) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void ProcessViewRotation(float DeltaTime, FRotator& OutViewRotation, FRotator& OutDeltaRot) override;

	void PlayCameraReaction(EAgCameraSituation Situation, const AActor* TurnTarget);

	/** 대상을 당긴 구도로 비춥니다. 처형 카메라 중이면 비추지 않습니다. */
	void StartLookAt(const AActor& Target, const FAgCameraLookAt& LookAt);

	/** 그 대상을 비추는 중일 때만 돌아오기를 시작합니다. */
	void StopLookAt(const AActor& Target);

protected:
	virtual void PostInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	AAgPlayerCharacter* GetPlayerCharacter() const;
	void SetObservedAbilitySystem(UAgAbilitySystemComponent* NewAbilitySystem);
	void ResetCameraState();

	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	void UpdateLockOnCamera(AAgPlayerCharacter& Player, const AActor& Target, const FVector& PivotLocation, float DeltaTime);
	void UpdateLockOnFlick(AAgPlayerCharacter& Player, float YawDelta);
	void StartCameraTurn(const AAgPlayerCharacter& Player, const AActor& Opponent, float Angle);
	void UpdateCameraTurn(const AAgPlayerCharacter& Player, const FVector& PivotLocation, float DeltaTime);
	bool IsExecutionCameraActive() const;
	void HandleAbilityCommitted(UGameplayAbility* Ability);
	void HandleAbilityEnded(const FAbilityEndedData& EndedData);
	void UpdateSprintFieldOfView();
	void HandleSprintFieldOfViewChanged(const FGameplayTag Tag, int32 NewCount);
	void HandleCutsceneChanged(const FGameplayTag Tag, int32 NewCount);

	UPROPERTY(Transient)
	TObjectPtr<UAgExecutionCameraModifier> ExecutionCamera;

	UPROPERTY(Transient)
	TObjectPtr<UAgFieldOfViewCameraModifier> FieldOfViewCamera;

	UPROPERTY(Transient)
	TObjectPtr<UAgLookAtCameraModifier> LookAtCamera;

	TWeakObjectPtr<UAgAbilitySystemComponent> ObservedAbilitySystem;
	TWeakObjectPtr<const AActor> PreviousCameraTarget;
	float LockOnPitchRate = 0.f;
	float LockOnYawRate = 0.f;
	TArray<TPair<double, float>> FlickSamples;
	/** 마지막으로 바꾼 쪽(+1 오른쪽, −1 왼쪽). 0이면 다음 전환을 받습니다. */
	float FlickDirection = 0.f;
	TWeakObjectPtr<const AActor> TurnOpponent;
	float TurnAngle = 0.f;
	float TurnYawRate = 0.f;
	uint64 TurnStartFrame = 0;
	uint64 LookAtStartFrame = 0;
};
