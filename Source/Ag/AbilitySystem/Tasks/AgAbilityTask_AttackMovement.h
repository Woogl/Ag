// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AgAbilityTask_AttackMovement.generated.h"

class AAgCharacterBase;

/**
 * 공격 중 이동(구현 계획서 '공격 보정', '정지 간격'): 도는 동안 매 프레임 Motion Warping 목표를 대상 쪽으로 간격만큼 앞
 * 지점에 두고 대상을 바라보게 하며, 캐릭터가 바라보는 방향이 고정되면(AAgCharacterBase::IsFacingFixed) 목표도 그 자리에
 * 고정합니다. 언제 다가가고 방향도 돌릴지는 몽타주의 Motion Warping 구간이 정합니다.
 * 끝나면(어빌리티가 끝날 때 포함) 워프 목표와 정지 간격을 지웁니다. 캐릭터마다 하나만 돌리고, 바꿀 때는 이전 것을 먼저 끝냅니다.
 */
UCLASS()
class UAgAbilityTask_AttackMovement : public UAbilityTask
{
	GENERATED_BODY()

public:
	UAgAbilityTask_AttackMovement();

	/**
	 * 간격 유지: 루트 모션이 InTarget에 InGap보다 가까이 데려가지 않고(정지 간격), 접근 목표는 InTarget 앞 InGap 지점입니다.
	 * 이미 더 가까우면 그 자리에 머뭅니다. InMaxTravel(음수면 제한 없음)은 접근 목표가 시작 위치에서 떨어질 수 있는 최대
	 * 거리입니다. InTarget이 InMaxRange(음수면 제한 없음)를 벗어나면 따라가기를 멈추고 워프 목표를 지웁니다. 범위 0은 어떤
	 * 대상도 따라가지 않는다는 뜻입니다(타겟팅 범위 0).
	 */
	static UAgAbilityTask_AttackMovement* KeepGap(UGameplayAbility* OwningAbility, AActor* InTarget, float InGap, float InMaxTravel, float InMaxRange = -1.f);

	/** 간격 맞춤(처형): 접근 목표가 InTarget 앞 InGap 지점이며, 더 가까우면 그 간격까지 물러납니다. 정지 간격은 걸지 않습니다. */
	static UAgAbilityTask_AttackMovement* FitGap(UGameplayAbility* OwningAbility, AActor* InTarget, float InGap);

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	/** 접근 목표를 지금 위치에 맞춰 옮깁니다. Target이 없거나 범위를 벗어나면 따라가기를 멈추고 워프 목표를 지웁니다. */
	void UpdateWarpTarget();

	TWeakObjectPtr<AAgCharacterBase> Character;
	TWeakObjectPtr<AActor> Target;
	FVector Origin = FVector::ZeroVector;
	float Gap = 0.f;
	float MaxTravel = -1.f;
	float MaxRange = -1.f;

	/** 간격 유지면 참, 간격 맞춤이면 거짓입니다. */
	bool bKeepGap = true;

	/** 접근 목표를 따라가는 중입니다. 방향이 고정되거나 대상이 범위를 벗어나면 꺼집니다. */
	bool bFollowing = false;
};
