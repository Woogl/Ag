// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AgAbilityTask_AttackMovement.generated.h"

class AAgCharacterBase;
struct FGameplayEventData;

/**
 * 공격 중 이동(전투 시스템과 구현 계획서의 '공격 중 이동'): 타를 시작할 때마다 대상을 다시 정해 정지 간격을 걸고,
 * 대상이 타겟팅 범위 안이면 캐릭터가 그쪽으로 돌게 하며 매 프레임 Motion Warping 목표를 대상 앞 정지 간격 지점에 둡니다.
 * 범위 안에 대상이 없으면 캐릭터가 정한 대체 대상(플레이어는 락온 대상) 쪽으로 돌게 합니다. 캐릭터가 바라보는 방향이
 * 고정되면(AAgCharacterBase::IsFacingFixed) 목표도 그 자리에 고정합니다. 언제 다가갈지는 몽타주의 Motion Warping 구간이
 * 정합니다. 끝나면(어빌리티가 끝날 때 포함) 워프 목표, 정지 간격, 회전 대상을 지웁니다.
 * 캐릭터마다 하나만 돌리고, 바꿀 때는 이전 것을 먼저 끝냅니다. 공격 몽타주를 재생한 뒤에 시작합니다.
 */
UCLASS()
class UAgAbilityTask_AttackMovement : public UAbilityTask
{
	GENERATED_BODY()

public:
	UAgAbilityTask_AttackMovement();

	/**
	 * 공격 하나(타 InNumHits개). 판정 구간이 끝날 때마다 다음 타를 시작합니다. 대상이 InTargetingRange(음수면 제한 없음) 안에
	 * 있을 때만 돌고 다가가며, 정지 간격은 범위와 관계없이 지킵니다. 다가가는 거리는 판정 구간 시작까지 모션 자체의 전진에
	 * InMaxApproachDistance를 더한 만큼을 넘지 않습니다.
	 */
	static UAgAbilityTask_AttackMovement* Attack(UGameplayAbility* OwningAbility, int32 InNumHits, float InTargetingRange, float InMaxApproachDistance);

	/** 간격 맞춤(처형): 접근 목표가 InTarget 앞 InGap 지점이며, 더 가까우면 그 간격까지 물러납니다. 정지 간격과 회전 대상은 걸지 않습니다. */
	static UAgAbilityTask_AttackMovement* FitGap(UGameplayAbility* OwningAbility, AAgCharacterBase* InTarget, float InGap);

	/** 이번 타의 대상(정지 간격의 대상). 없으면 null입니다. */
	AAgCharacterBase* GetTarget() const { return Target.Get(); }

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	/** 타를 시작합니다: 대상을 다시 정하고, 정지 간격과 회전 대상(범위 밖이면 대체 대상)을 걸고, 접근 상한을 다시 잽니다. */
	void StartHit();

	void HandleAttackWindowEnded(const FGameplayEventData* Payload);

	/** 접근 목표를 지금 위치에 맞춰 옮깁니다. 대상이 없거나 범위를 벗어나면 추적을 멈춥니다. */
	void UpdateWarpTarget();

	/** 이번 타의 추적을 멈춥니다: 회전 대상과 워프 목표를 지웁니다. 모션은 만든 그대로 마지막 방향으로 계속합니다. */
	void StopTracking();

	TWeakObjectPtr<AAgCharacterBase> Character;
	TWeakObjectPtr<AAgCharacterBase> Target;
	FVector Origin = FVector::ZeroVector;
	float Gap = 0.f;
	float MaxTravel = -1.f;
	float TargetingRange = -1.f;
	float MaxApproachDistance = 0.f;
	int32 NumHits = 0;

	/** 공격이면 참, 간격 맞춤(처형)이면 거짓입니다. */
	bool bKeepGap = true;

	bool bTracking = false;

	FDelegateHandle WindowEndedHandle;
};
