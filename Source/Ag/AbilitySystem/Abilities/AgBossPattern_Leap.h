// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AgBossPatternAbility.h"
#include "AgBossPattern_Leap.generated.h"

/**
 * A4 도약 내려찍기 (보스 사양 'A4'): 엔진 기본 중력으로 포물선을 그리며 체공 시간 뒤 착지합니다. 최고점에서 수평 속도를
 * 한 번 다시 주어, 그때의 플레이어 위치로 착지 목표를 고칩니다. 착지하면 End 구간을 재생하고, 착지 지점 반경 안에서 땅에
 * 있는 대상을 한 번 판정합니다(점프 중이면 맞지 않음).
 * 회전 정지와 최고점은 보스의 수직 속도로 판정하므로, 히트스톱으로 비행이 멈추면 함께 늦어집니다. 붉은 섬광은 몽타주의
 * 노티파이가 켭니다.
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

	/** 착지까지 남은 체공 시간이 RemainingTime초가 되는 순간의 하강 속도 */
	float GetDescentSpeedAt(float RemainingTime) const;

	/** 떠오르면 회전 정지와 최고점을 기다립니다. */
	UFUNCTION()
	void HandleTakeOff(EMovementMode NewMovementMode);

	/** 최고점: 남은 체공 시간 안에 지금의 착지 목표에 닿는 수평 속도를 줍니다. 수직 속도는 그대로 두어 체공 시간과 높이가 바뀌지 않습니다. */
	UFUNCTION()
	void HandleApex();

	UFUNCTION()
	void HandleRotationStop();

	UFUNCTION()
	void HandleLanded(EMovementMode NewMovementMode);

	FVector LeapStart = FVector::ZeroVector;

	/** 체공 시간(2페이즈에서는 짧아짐)과 중력 가속도 */
	float AirTime = 0.f;
	float Gravity = 0.f;

	/** 착지보다 회전 정지가 앞서는 시간 */
	float RotationStopLead = 0.f;

	bool bRotationLocked = false;
	bool bLanded = false;
};
