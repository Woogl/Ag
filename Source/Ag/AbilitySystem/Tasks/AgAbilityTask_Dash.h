// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AgAbilityTask_Dash.generated.h"

class UCharacterMovementComponent;

/**
 * Moves the owner in a straight line at a constant speed with a root motion source (A3 돌진), until it has covered
 * the distance or something blocks it. Collision still applies, so a wall stops it where it is.
 */
UCLASS()
class UAgAbilityTask_Dash : public UAbilityTask
{
	GENERATED_BODY()

public:
	UAgAbilityTask_Dash();

	UPROPERTY(BlueprintAssignable)
	FGenericGameplayTaskDelegate OnFinished;

	static UAgAbilityTask_Dash* Dash(UGameplayAbility* OwningAbility, const FVector& Direction, float Speed, float Distance);

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	void Finish();
	void RemoveSource();

	FVector Direction = FVector::ForwardVector;
	float Speed = 0.f;
	float Distance = 0.f;
	FVector StartLocation = FVector::ZeroVector;
	FVector LastLocation = FVector::ZeroVector;
	int32 BlockedTicks = 0;
	uint16 SourceID = 0;
	TWeakObjectPtr<UCharacterMovementComponent> Movement;
};
