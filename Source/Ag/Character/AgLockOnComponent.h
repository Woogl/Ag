// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AgLockOnComponent.generated.h"

class AAgCharacterBase;

DECLARE_MULTICAST_DELEGATE(FAgLockOnChangedSignature);

/**
 * 플레이어의 락온(플레이어 사양 '락온'): 대상을 고르고 해제 조건을 검사하며, 락온 어빌리티가 도는 동안 대상을 들고
 * 있습니다. 카메라는 그 어빌리티의 태스크가 돌립니다(카메라 '락온 카메라').
 */
UCLASS()
class UAgLockOnComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** 락온 대상: 락온 타겟팅 범위 안에서 화면 안에 있는 가장 가까운 적(구현 계획서 '대상 찾기'). 없으면 null입니다. */
	AAgCharacterBase* FindTarget() const;

	/** NewTarget에 락온하고, null이면 락온을 끝냅니다. */
	void SetTarget(AAgCharacterBase* NewTarget);

	/** 락온 키 외의 락온 해제 조건(대상 사망, 해제 거리 밖)이 되면 참입니다. */
	bool IsTargetLost() const;

	bool IsLockedOn() const { return Target.IsValid(); }

	AAgCharacterBase* GetTarget() const { return Target.Get(); }

	/** 락온 카메라가 바라보는 락온 지점: 대상의 락온 본(가슴 높이) 위치입니다. */
	FVector GetLockOnPoint() const;

	/** 락온이 시작되거나 끝날 때 알립니다. */
	FAgLockOnChangedSignature OnLockOnChanged;

private:
	TWeakObjectPtr<AAgCharacterBase> Target;
};
