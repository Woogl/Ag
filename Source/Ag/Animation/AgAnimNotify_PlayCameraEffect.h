// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Camera/AgFieldOfViewCameraModifier.h"
#include "AgAnimNotify_PlayCameraEffect.generated.h"

class UCameraShakeBase;

/** 모션 주인의 위치에서 재생해, 플레이어 카메라가 멀수록 약해집니다. */
UCLASS(const, hidecategories = Object, collapsecategories)
class UAgAnimNotify_PlayCameraEffect : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	UPROPERTY(EditAnywhere, Category = "Camera Effect")
	TArray<TSubclassOf<UCameraShakeBase>> CameraShakes;

	UPROPERTY(EditAnywhere, Category = "Camera Effect")
	FAgFieldOfViewEffect FieldOfView;

	UPROPERTY(EditAnywhere, Category = "Camera Effect", meta = (ClampMin = "0"))
	float InnerRadius = 1500.f;

	UPROPERTY(EditAnywhere, Category = "Camera Effect", meta = (ClampMin = "0"))
	float OuterRadius = 4000.f;
};
