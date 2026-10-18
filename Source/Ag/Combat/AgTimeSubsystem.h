// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AgTimeSubsystem.generated.h"

class AAgCharacterBase;

DECLARE_MULTICAST_DELEGATE_OneParam(FAgTimeStopChanged, bool /*bStopped*/);

/**
 * 게임 전체의 시간 배율을 맡습니다(플레이어 사양 '슬로우모션', '시간 정지'). 캐릭터마다 멈추는 히트스톱은 캐릭터의 태그가 맡습니다.
 * 시간 정지는 게임 전체 시간 배율을 거의 0으로 낮추고, 시간을 멈춘 캐릭터만 역배율로 정상 속도로 돌립니다.
 * 그래서 다른 캐릭터의 모션·이동·AI와 함께 엔진·GAS 타이머(그로기 시간, 쿨다운, 리젠)도 멈춥니다.
 */
UCLASS()
class UAgTimeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * 실제 시간으로 RealDuration초 동안 게임 전체를 Speed배로 돌립니다. 도는 중에 다시 시작하면 처음부터 다시 셉니다. 시간 정지
	 * 중이면 시간 정지가 끝날 때 시작합니다(궁극기 마무리 1타가 시간 정지 끝보다 먼저 처리된 경우).
	 */
	void StartSlowMotion(float Speed, float RealDuration);

	/** 시간 정지를 시작합니다. Exempt만 정상 속도로 돌고, 도는 슬로우모션과 기다리던 슬로우모션은 끝납니다. */
	bool StartTimeStop(UObject& Requester, AAgCharacterBase& Exempt);

	/** 시간 정지를 끝냅니다. 그동안 시작한 슬로우모션이 있으면 그 배율로, 없으면 원래 속도로 돌립니다. */
	void EndTimeStop(UObject& Requester);

	bool IsTimeStopped() const
	{
		return bTimeStopped;
	}

	/** 시간 정지 중에 정상 속도로 돌려면 곱하는 배율(게임 전체 시간 배율의 역수). 시간 정지 중이 아니면 1입니다. */
	float GetTimeStopCompensation() const;

	/** 시간 정지가 시작되거나 끝났습니다(화면 흑백). */
	FAgTimeStopChanged OnTimeStopChanged;

private:
	/** 슬로우모션 배율을 거는 유일한 곳. 배율과 끝 타이머를 함께 겁니다. 시간 정지가 아닐 때만 부릅니다. */
	void ApplySlowMotion(float Speed, float RealDuration);

	void EndSlowMotion();

	FTimerHandle SlowMotionTimer;

	/** 시간 정지 중에 시작해 시간 정지가 끝나기를 기다리는 슬로우모션. 길이가 0이면 없습니다. */
	float PendingSlowMotionSpeed = 1.f;
	float PendingSlowMotionDuration = 0.f;

	bool bTimeStopped = false;
	TWeakObjectPtr<UObject> TimeStopRequester;

	TWeakObjectPtr<AAgCharacterBase> TimeStopExempt;
};
