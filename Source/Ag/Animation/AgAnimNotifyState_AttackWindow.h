// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AgAnimNotifyState_AttackWindow.generated.h"

class UAnimInstance;

/**
 * 판정 구간: 켜진 동안 주인의 무기 날을 스윕합니다. 판정 구간 하나가 타 하나이고, HitIndex가 그 타의 공격 데이터를
 * 고릅니다. 행동의 마지막 판정 구간이 끝나는 때가 후딜의 시작입니다.
 */
UCLASS(meta = (DisplayName = "Attack Window"))
class UAgAnimNotifyState_AttackWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	/** 이 판정 구간이 행동의 몇 번째 타인지 (0부터) */
	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = 0))
	int32 HitIndex = 0;

	/**
	 * 재생 중인 공격 몽타주에서, 지금 위치부터 다음 판정 구간이 시작될 때까지 몽타주 자체의 루트 모션이 수평으로 가는
	 * 거리. 남은 판정 구간이 없으면 0입니다.
	 */
	static float GetTravelToNextWindow(const UAnimInstance& AnimInstance);

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
