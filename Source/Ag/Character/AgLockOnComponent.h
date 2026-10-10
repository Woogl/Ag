// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AgLockOnComponent.generated.h"

class AAgBossCharacter;

DECLARE_MULTICAST_DELEGATE(FAgLockOnChangedSignature);

/**
 * 락온 (플레이어 사양 '락온') on the player: picks the target, checks the release conditions and holds the target while
 * the lock-on ability runs. The ability's task turns the camera (카메라 '락온 카메라').
 */
UCLASS()
class UAgLockOnComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** 락온 대상: a living boss within the lock-on range and on screen, or null. */
	AAgBossCharacter* FindTarget() const;

	/** Starts the lock-on on NewTarget, or ends it with null. */
	void SetTarget(AAgBossCharacter* NewTarget);

	/** 락온 해제 조건 other than the key: the target died or went beyond the release range. */
	bool IsTargetLost() const;

	bool IsLockedOn() const { return Target.IsValid(); }

	AAgBossCharacter* GetTarget() const { return Target.Get(); }

	/** Where the camera looks: the target's lock-on bone (chest height). */
	FVector GetLockOnPoint() const;

	/** Broadcast when a lock-on starts or ends. */
	FAgLockOnChangedSignature OnLockOnChanged;

private:
	TWeakObjectPtr<AAgBossCharacter> Target;
};
