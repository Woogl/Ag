// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AgBossPatternAbility.h"
#include "AgBossPattern_Leap.generated.h"

class UAbilityTask_Repeat;

/**
 * A4 도약 내려찍기 (보스 사양 'A4'): 엔진 기본 중력으로 포물선을 그리며 체공 시간 뒤 착지합니다. 섬광이 켜지기 전까지는
 * 공중에서도 수평 속도를 다시 주어, 그때의 플레이어 위치로 정한 착지 목표를 따라갑니다. 착지하면 End 구간을 재생하고,
 * 착지 지점 반경 안에서 땅에 있는 대상을 한 번 판정합니다(점프 중이면 맞지 않음).
 */
UCLASS()
class UAgBossPattern_Leap : public UAgBossPatternAbility
{
	GENERATED_BODY()

protected:
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual bool UsesSharedMovement() const override { return false; }
	virtual void OnPatternStarted() override;

private:
	void SetRotationLocked(bool bLocked);

	/** 착지 목표: 지금 플레이어 위치에서 도약한 위치 쪽으로 떨어진 지점. 도약한 위치에서 최대 도약 거리를 넘지 않습니다. */
	FVector GetLandingTarget() const;

	/** 남은 체공 시간 안에 착지 목표에 닿는 수평 속도를 줍니다. 수직 속도는 그대로 두어 체공 시간과 높이가 바뀌지 않습니다. */
	UFUNCTION()
	void HandleAim(int32 ActionNumber);

	/** 섬광을 켜고 착지 목표를 고정합니다. */
	UFUNCTION()
	void HandleTelegraphTime();

	UFUNCTION()
	void HandleRotationStop();

	UFUNCTION()
	void HandleLanded(EMovementMode NewMovementMode);

	void StopAiming();

	UPROPERTY()
	TObjectPtr<UAbilityTask_Repeat> AimTask;

	FVector LeapStart = FVector::ZeroVector;

	/** 착지할 월드 시간 */
	double LandingTime = 0.0;

	bool bRotationLocked = false;
	bool bLanded = false;
};
