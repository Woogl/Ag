// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraModifier.h"
#include "AgFieldOfViewCameraModifier.generated.h"

USTRUCT()
struct FAgFieldOfViewEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float Offset = 0.f;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0"))
	float BlendInTime = 0.f;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0"))
	float HoldTime = 0.f;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0"))
	float BlendOutTime = 0.f;
};

UCLASS()
class UAgFieldOfViewCameraModifier : public UCameraModifier
{
	GENERATED_BODY()

public:
	UAgFieldOfViewCameraModifier(const FObjectInitializer& ObjectInitializer);

	void Play(const FAgFieldOfViewEffect& Effect, float Scale = 1.f);

	void Hold(const FAgFieldOfViewEffect& Effect);

	void Release(const FAgFieldOfViewEffect& Effect);

	void StopAll();

protected:
	virtual void ModifyCamera(float DeltaTime, FVector ViewLocation, FRotator ViewRotation, float FOV, FVector& NewViewLocation, FRotator& NewViewRotation, float& NewFOV) override;

private:
	enum class EPhase : uint8
	{
		BlendIn,
		Hold,
		BlendOut,
	};

	struct FActiveEffect
	{
		// 같은 정의에서 재생한 것을 같은 연출로 보므로, 정의의 주소는 비교에만 씁니다.
		const FAgFieldOfViewEffect* Source = nullptr;
		FAgFieldOfViewEffect Effect;
		float Scale = 1.f;
		EPhase Phase = EPhase::BlendIn;
		float Weight = 0.f;
		float HoldRemaining = 0.f;
		bool bHold = false;
	};

	void Start(const FAgFieldOfViewEffect& Effect, float Scale, bool bHold);

	TArray<FActiveEffect> ActiveEffects;
};
