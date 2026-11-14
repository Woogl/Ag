// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraModifier.h"
#include "Data/AgCameraData.h"
#include "AgFieldOfViewCameraModifier.generated.h"

UCLASS(NotBlueprintable)
class UAgFieldOfViewCameraModifier : public UCameraModifier
{
	GENERATED_BODY()

public:
	UAgFieldOfViewCameraModifier(const FObjectInitializer& ObjectInitializer);

	/** 값이 같은 연출이 재생 중이면 겹치지 않고 지금 세기에서 들어가기부터 다시 시작합니다. */
	void Play(const FAgFieldOfViewEffect& Effect, float Scale = 1.f);

	/** Release를 부를 때까지 유지합니다. 유지하는 연출은 하나라, 다시 부르면 앞서 유지하던 연출은 빠져나갑니다. */
	void Hold(const FAgFieldOfViewEffect& Effect);

	void Release();

	void StopAll();

protected:
	virtual void ModifyCamera(float DeltaTime, FVector ViewLocation, FRotator ViewRotation, float FOV, FVector& NewViewLocation, FRotator& NewViewRotation, float& NewFOV) override;

private:
	enum class EAgPhase : uint8
	{
		BlendIn,
		Hold,
		BlendOut,
	};

	struct FAgActiveEffect
	{
		FAgFieldOfViewEffect Effect;
		float Scale = 1.f;
		EAgPhase Phase = EAgPhase::BlendIn;
		float Weight = 0.f;
		float HoldRemaining = 0.f;
		bool bHold = false;
	};

	void Start(const FAgFieldOfViewEffect& Effect, float Scale, bool bHold);

	TArray<FAgActiveEffect> ActiveEffects;
};
