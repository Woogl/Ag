// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AgLockOnComponent.generated.h"

class AAgCharacterBase;

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
	/** 락온 대상: 락온 타겟팅 범위 안에서 화면 안에 있는 가장 가까운 적('대상 찾기'). 없으면 null입니다. */
	AAgCharacterBase* FindTarget() const;

	/** NewTarget에 락온하고, null이면 락온을 끝냅니다. */
	void SetTarget(AAgCharacterBase* NewTarget);

	/** 락온 해제 조건 other than the key: the target died or went beyond the release range. */
	bool IsTargetLost() const;

	bool IsLockedOn() const { return Target.IsValid(); }

	AAgCharacterBase* GetTarget() const { return Target.Get(); }

	/** Where the camera looks: the target's lock-on bone (chest height). */
	FVector GetLockOnPoint() const;

	/** Broadcast when a lock-on starts or ends. */
	FAgLockOnChangedSignature OnLockOnChanged;

private:
	TWeakObjectPtr<AAgCharacterBase> Target;
};
