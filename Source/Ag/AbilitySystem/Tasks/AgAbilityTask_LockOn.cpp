// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_LockOn.h"

#include "Character/AgLockOnComponent.h"
#include "Character/AgPlayerCharacter.h"
#include "Core/AgSettings.h"
#include "Data/AgCameraData.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"

namespace
{
	/**
	 * Moves Angle toward Target on a critically damped spring (it eases in and out), never faster than MaxSpeed. Long
	 * frames are split, because the spring is only accurate for steps under half the smoothing time.
	 */
	float SpringToward(float Angle, float& Rate, float Target, float DeltaTime, float SmoothingTime, float MaxSpeed)
	{
		const int32 Steps = SmoothingTime > 0.f ? FMath::Clamp(FMath::CeilToInt(DeltaTime / (0.5f * SmoothingTime)), 1, 8) : 1;
		const float StepTime = DeltaTime / Steps;
		for (int32 Step = 0; Step < Steps; ++Step)
		{
			const float Start = Angle;
			FMath::CriticallyDampedSmoothing(Angle, Rate, Target, 0.f, StepTime, SmoothingTime);
			Angle = Start + FMath::Clamp(Angle - Start, -MaxSpeed * StepTime, MaxSpeed * StepTime);
			Rate = FMath::Clamp(Rate, -MaxSpeed, MaxSpeed);
		}
		return Angle;
	}
}

UAgAbilityTask_LockOn::UAgAbilityTask_LockOn()
{
	bTickingTask = true;
}

UAgAbilityTask_LockOn* UAgAbilityTask_LockOn::FollowTarget(UGameplayAbility* OwningAbility, UAgLockOnComponent* LockOn)
{
	UAgAbilityTask_LockOn* Task = NewAbilityTask<UAgAbilityTask_LockOn>(OwningAbility);
	Task->LockOn = LockOn;
	return Task;
}

void UAgAbilityTask_LockOn::TickTask(float DeltaTime)
{
	const UAgLockOnComponent* LockOnComponent = LockOn.Get();
	if (!LockOnComponent || LockOnComponent->IsTargetLost())
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnTargetLost.Broadcast();
		}
		EndTask();
		return;
	}

	// The camera isn't part of the character's motion, so hitstop (the owner's time dilation) doesn't pause it.
	UpdateCamera(GetWorld()->GetDeltaSeconds());
}

void UAgAbilityTask_LockOn::UpdateCamera(float DeltaTime)
{
	const AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(GetAvatarActor());
	APlayerController* PlayerController = Player ? Cast<APlayerController>(Player->GetController()) : nullptr;
	if (!PlayerController)
	{
		return;
	}
	const UAgCameraData& CameraData = UAgSettings::Get()->GetCameraData();

	// 구도: 카메라 중심(스프링 암의 시작점)에서 락온 지점을 보되, 피치를 제한한 뒤 더 내려다봅니다.
	const FVector Pivot = Player->GetCameraBoom()->GetComponentLocation();
	FRotator Goal = (LockOn->GetLockOnPoint() - Pivot).Rotation();
	Goal.Pitch = FMath::Clamp(FRotator::NormalizeAxis(Goal.Pitch), CameraData.LockOnPitchMin, CameraData.LockOnPitchMax) - CameraData.LockOnLookDown;
	Goal.Roll = 0.f;

	// 회전: 목표로 부드럽게 가속·감속하며 최대 속도보다 빨리 돌지 않으므로, 락온을 걸거나 머리 위로 돌진·도약이 지나가도
	// 화면이 튀지 않습니다. Yaw는 짧은 쪽으로 돕니다.
	const FRotator Current = PlayerController->GetControlRotation();
	const float CurrentPitch = static_cast<float>(FRotator::NormalizeAxis(Current.Pitch));
	const float CurrentYaw = static_cast<float>(Current.Yaw);
	const float GoalYaw = CurrentYaw + static_cast<float>(FMath::FindDeltaAngleDegrees(Current.Yaw, Goal.Yaw));
	const float Pitch = SpringToward(CurrentPitch, PitchRate, static_cast<float>(Goal.Pitch), DeltaTime, CameraData.LockOnSmoothingTime, CameraData.LockOnRotationSpeed);
	const float Yaw = SpringToward(CurrentYaw, YawRate, GoalYaw, DeltaTime, CameraData.LockOnSmoothingTime, CameraData.LockOnRotationSpeed);
	PlayerController->SetControlRotation(FRotator(Pitch, Yaw, 0.f));
}
