// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_LockOn.h"

#include "Character/AgLockOnComponent.h"
#include "Character/AgPlayerCharacter.h"
#include "Core/AgAssetManager.h"
#include "Data/AgCameraData.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"

namespace
{
	/**
	 * Angle을 임계 감쇠 스프링으로 Target 쪽으로 옮기되(부드럽게 가속·감속) MaxSpeed보다 빨리 돌지 않습니다. 스프링은
	 * 한 스텝이 SmoothingTime의 절반보다 짧을 때만 정확하므로 긴 프레임은 나눠 계산합니다.
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

	// 카메라는 캐릭터의 모션이 아니므로 히트스톱(주인의 시간 배율)에 멈추지 않습니다.
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
	const UAgCameraData& CameraData = UAgAssetManager::Get().GetCameraData();

	// 구도: 회전 중심(스프링 암의 시작점)에서 락온 지점을 보되, 피치를 제한한 뒤 더 내려다봅니다.
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
