// Copyright Woogle. All Rights Reserved.

#include "Camera/AgExecutionCameraModifier.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"

UAgExecutionCameraModifier::UAgExecutionCameraModifier(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 셰이크 모디파이어(127)보다 먼저 적용해야 처형 구도 위에 셰이크가 더해집니다.
	Priority = 64;
	AlphaInTime = 0.8f;
	AlphaOutTime = 0.8f;
	bDisabled = true;
}

void UAgExecutionCameraModifier::Start(const AActor& InPlayer, const AActor& InTarget)
{
	Player = &InPlayer;
	Target = &InTarget;
	TargetStartLocation = InTarget.GetActorLocation();
	Elapsed = 0.f;

	// 구도의 방향은 처형을 시작할 때 정하고, 오른쪽이 막혔으면 좌우를 뒤집습니다.
	const float FacingYaw = (InTarget.GetActorLocation() - InPlayer.GetActorLocation()).GetSafeNormal2D().Rotation().Yaw;
	bool bBlocked = false;
	SweepCameraLocation(GetLookAtLocation(), FacingYaw + Yaw, bBlocked);
	CompositionYaw = bBlocked ? FacingYaw - Yaw : FacingYaw + Yaw;

	EnableModifier();
}

void UAgExecutionCameraModifier::Stop()
{
	DisableModifier(false);
}

void UAgExecutionCameraModifier::ModifyCamera(float DeltaTime, FVector ViewLocation, FRotator ViewRotation, float FOV, FVector& NewViewLocation, FRotator& NewViewRotation, float& NewFOV)
{
	Elapsed += DeltaTime;
	if (Elapsed >= ReturnTime)
	{
		Stop();
	}
	if (!Player.IsValid() || !Target.IsValid())
	{
		return;
	}

	const FVector LookAt = GetLookAtLocation();
	bool bBlocked = false;
	const FVector CameraLocation = SweepCameraLocation(LookAt, CompositionYaw, bBlocked);
	const FRotator CameraRotation = (LookAt - CameraLocation).Rotation();
	const float ZoomAlpha = ZoomDuration > 0.f ? FMath::Min(Elapsed / ZoomDuration, 1.f) : 1.f;
	const float CompositionFOV = FMath::Lerp(FieldOfView, ZoomFieldOfView, FMath::InterpEaseOut(0.f, 1.f, ZoomAlpha, 2.f));

	// 엔진의 알파는 선형이라, 옮겨 가기 시작하고 멈추는 순간이 튀지 않게 이즈 인·아웃을 겁니다.
	const float Weight = FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f);
	NewViewLocation = FMath::Lerp(ViewLocation, CameraLocation, Weight);
	NewViewRotation = FQuat::Slerp(ViewRotation.Quaternion(), CameraRotation.Quaternion(), Weight).Rotator();
	NewFOV = FMath::Lerp(FOV, CompositionFOV, Weight);
}

FVector UAgExecutionCameraModifier::GetLookAtLocation() const
{
	return (Player->GetActorLocation() + TargetStartLocation) * 0.5 + FVector(0., 0., Height);
}

FVector UAgExecutionCameraModifier::SweepCameraLocation(const FVector& LookAt, float CameraYaw, bool& bOutBlocked) const
{
	const FVector Desired = LookAt + FRotator(0., CameraYaw, 0.).Vector() * Distance + FVector(0., 0., CameraHeight);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ExecutionCamera), false);
	Params.AddIgnoredActor(Player.Get());
	Params.AddIgnoredActor(Target.Get());
	FHitResult Hit;
	bOutBlocked = GetWorld()->SweepSingleByChannel(Hit, LookAt, Desired, FQuat::Identity, ECC_Camera, FCollisionShape::MakeSphere(ProbeSize), Params);
	return bOutBlocked ? Hit.Location : Desired;
}
