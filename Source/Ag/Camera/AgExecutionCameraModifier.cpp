// Copyright Woogle. All Rights Reserved.

#include "Camera/AgExecutionCameraModifier.h"

#include "Core/AgAssetManager.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UAgExecutionCameraModifier::UAgExecutionCameraModifier(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 셰이크 모디파이어(127)보다 먼저 적용해야 처형 구도 위에 셰이크가 더해집니다.
	Priority = 64;
	bDisabled = true;
}

void UAgExecutionCameraModifier::Start(const AActor& InPlayer, const AActor& InTarget)
{
	Elapsed = 0.f;
	Settings = UAgAssetManager::Get().GetCameraData().Execution;
	AlphaInTime = Settings.AlphaInTime;
	AlphaOutTime = Settings.AlphaOutTime;

	// 카메라 자리는 처형을 시작할 때 정하고, 오른쪽만 막혔으면 좌우를 뒤집습니다. 양쪽 모두 막히면 오른쪽에 남습니다.
	const FVector PlayerLocation = InPlayer.GetActorLocation();
	const FVector TargetLocation = InTarget.GetActorLocation();
	const float FacingYaw = (TargetLocation - PlayerLocation).GetSafeNormal2D().Rotation().Yaw;
	const FVector LookAt = (PlayerLocation + TargetLocation) * 0.5 + FVector(0., 0., Settings.Height);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ExecutionCamera), false);
	Params.AddIgnoredActor(&InPlayer);
	Params.AddIgnoredActor(&InTarget);
	bool bRightBlocked = false;
	CameraLocation = SweepCameraLocation(LookAt, FacingYaw + Settings.Yaw, Params, bRightBlocked);
	if (bRightBlocked)
	{
		bool bLeftBlocked = false;
		const FVector LeftLocation = SweepCameraLocation(LookAt, FacingYaw - Settings.Yaw, Params, bLeftBlocked);
		if (!bLeftBlocked)
		{
			CameraLocation = LeftLocation;
		}
	}
	CameraRotation = (LookAt - CameraLocation).Rotation();

	EnableModifier();
}

void UAgExecutionCameraModifier::Stop()
{
	DisableModifier(false);
}

void UAgExecutionCameraModifier::ModifyCamera(float DeltaTime, FVector ViewLocation, FRotator ViewRotation, float FOV, FVector& NewViewLocation, FRotator& NewViewRotation, float& NewFOV)
{
	Elapsed += DeltaTime;
	if (Elapsed >= Settings.ReturnTime)
	{
		Stop();
	}

	const float ZoomAlpha = Settings.ZoomDuration > 0.f ? FMath::Min(Elapsed / Settings.ZoomDuration, 1.f) : 1.f;
	const float CompositionFOV = FMath::InterpEaseOut(Settings.FieldOfView, Settings.ZoomFieldOfView, ZoomAlpha, 2.f);

	// 엔진의 알파는 선형이라, 옮겨 가기 시작하고 멈추는 순간이 튀지 않게 이즈 인·아웃을 겁니다.
	const float Weight = FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f);
	NewViewLocation = FMath::Lerp(ViewLocation, CameraLocation, Weight);
	NewViewRotation = FQuat::Slerp(ViewRotation.Quaternion(), CameraRotation.Quaternion(), Weight).Rotator();
	NewFOV = FMath::Lerp(FOV, CompositionFOV, Weight);
}

FVector UAgExecutionCameraModifier::SweepCameraLocation(const FVector& LookAt, float CameraYaw, const FCollisionQueryParams& Params, bool& bOutBlocked) const
{
	const FVector Desired = LookAt + FRotator(0., CameraYaw, 0.).Vector() * Settings.Distance + FVector(0., 0., Settings.CameraHeight);
	FHitResult Hit;
	bOutBlocked = GetWorld()->SweepSingleByChannel(Hit, LookAt, Desired, FQuat::Identity, ECC_Camera, FCollisionShape::MakeSphere(Settings.ProbeSize), Params);
	return bOutBlocked ? Hit.Location : Desired;
}
