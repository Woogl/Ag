// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Data/AgCameraData.h"
#include "AgAnimNotifyState_CameraLookAt.generated.h"

/** 모션 주인을 당긴 구도로 비춥니다. 구간이 끝나거나 몽타주가 끊기면 돌아옵니다. */
UCLASS(const, hidecategories = Object, collapsecategories)
class UAgAnimNotifyState_CameraLookAt : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	UPROPERTY(EditAnywhere, Category = "Camera Look At")
	FAgCameraLookAt LookAt;

	/** 플레이어 카메라가 모션 주인에게서 이보다 멀면 비추지 않습니다. */
	UPROPERTY(EditAnywhere, Category = "Camera Look At", meta = (ClampMin = "0"))
	float Radius = 4000.f;
};
