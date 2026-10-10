// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AgLockOnComponent.generated.h"

class AAgBossCharacter;

DECLARE_MULTICAST_DELEGATE(FAgLockOnChangedSignature);

/**
 * 락온 (플레이어 사양 '락온', 카메라 '락온 카메라') on the player.
 * Picks the target, ends the lock-on by its release conditions, and turns the camera toward the target's lock-on point.
 */
UCLASS()
class UAgLockOnComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAgLockOnComponent();

	/** 락온 / 락온 해제: the lock-on key. Locks on to a living boss within range and on screen, or releases. */
	void ToggleLockOn();

	void ReleaseLockOn();

	bool IsLockedOn() const { return Target.IsValid(); }

	AAgBossCharacter* GetTarget() const { return Target.Get(); }

	/** Broadcast when a lock-on starts or ends. */
	FAgLockOnChangedSignature OnLockOnChanged;

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** 락온 대상: a living boss within the lock-on range and on screen, or null. */
	AAgBossCharacter* FindTarget() const;

	/** Where the camera looks: the target's lock-on bone (chest height). */
	FVector GetLockOnPoint() const;

	/** 락온 카메라 '구도', '회전': turns the control rotation toward the target. */
	void UpdateCamera(float DeltaTime);

	TWeakObjectPtr<AAgBossCharacter> Target;
};
