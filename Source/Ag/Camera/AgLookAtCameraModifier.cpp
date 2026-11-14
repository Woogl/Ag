// Copyright Woogle. All Rights Reserved.

#include "Camera/AgLookAtCameraModifier.h"

#include "GameFramework/Actor.h"

UAgLookAtCameraModifier::UAgLookAtCameraModifier(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 처형 카메라(64) 뒤, 시야각 모디파이어(96)와 셰이크 모디파이어(127) 앞이라 당긴 구도 위에 시야각 변화와 셰이크가 더해집니다.
	Priority = 80;
	bDisabled = true;
}

void UAgLookAtCameraModifier::Start(const AActor& InTarget, const FAgCameraLookAt& LookAt)
{
	// 엔진은 바로 끌 때(컷신 시작, 대상 소멸) 알파를 그대로 두므로, 꺼져 있었으면 0에서부터 옮겨 갑니다.
	if (IsDisabled())
	{
		Alpha = 0.f;
	}
	Target = &InTarget;
	Distance = LookAt.Distance;
	AlphaInTime = LookAt.BlendInTime;
	AlphaOutTime = LookAt.BlendOutTime;
	EnableModifier();
}

void UAgLookAtCameraModifier::Stop()
{
	DisableModifier(false);
}

const AActor* UAgLookAtCameraModifier::GetLookAtTarget() const
{
	return IsDisabled() || bPendingDisable ? nullptr : Target.Get();
}

void UAgLookAtCameraModifier::ModifyCamera(float DeltaTime, FVector ViewLocation, FRotator ViewRotation, float FOV, FVector& NewViewLocation, FRotator& NewViewRotation, float& NewFOV)
{
	const AActor* LookAtTarget = Target.Get();
	if (!LookAtTarget)
	{
		DisableModifier(true);
		return;
	}

	const FVector FocusPoint = LookAtTarget->GetTargetLocation();
	const FVector ToFocus = FocusPoint - ViewLocation;
	const double Length = ToFocus.Size();
	if (Length <= UE_KINDA_SMALL_NUMBER)
	{
		return;
	}
	const FVector Direction = ToFocus / Length;
	const FVector PulledLocation = Length > Distance ? FocusPoint - Direction * Distance : ViewLocation;

	// 엔진의 알파는 선형이라, 옮겨 가기 시작하고 멈추는 순간이 튀지 않게 이즈 인·아웃을 겁니다.
	const float Weight = FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f);
	NewViewLocation = FMath::Lerp(ViewLocation, PulledLocation, Weight);
	NewViewRotation = FQuat::Slerp(ViewRotation.Quaternion(), Direction.Rotation().Quaternion(), Weight).Rotator();
}
