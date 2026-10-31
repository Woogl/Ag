// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_Launch.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "Character/AgCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"

namespace
{
	/** 띄운 직후 아직 땅에 있는 것을 착지로 보지 않도록, 체공 시간의 이 비율이 지난 뒤의 착지만 봅니다. */
	constexpr float MinLandedAirTimeRatio = 0.5f;
}

UAbilityTask* FAgLaunchParams::MakeTask(const FAgTaskContext& Context) const
{
	UAgAbilityTask_Launch* Task = UAbilityTask::NewAbilityTask<UAgAbilityTask_Launch>(&Context.Ability, UAgAbilityTask_Launch::SourceName);
	Task->Params = *this;
	return Task;
}

const FName UAgAbilityTask_Launch::SourceName(TEXT("AgLaunch"));

void UAgAbilityTask_Launch::Activate()
{
	AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor());
	const UAgGameplayAbility* AgAbility = CastChecked<UAgGameplayAbility>(Ability);
	const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Movement)
	{
		EndTask();
		return;
	}

	Rotation = Character->GetActorRotation();
	Distance = 0.f;
	if (Params.Direction == EAgLaunchDirection::Back)
	{
		// 함께 시작하는 몸 돌리기와 순서가 없어도 같은 방향이 되도록, 대상이 있으면 정면 대신 대상의 반대쪽으로 띄웁니다.
		const AActor* ActionTarget = AgAbility->GetActionTarget();
		const FVector Away = ActionTarget ? (Character->GetActorLocation() - ActionTarget->GetActorLocation()).GetSafeNormal2D() : FVector::ZeroVector;
		Rotation.Yaw = Away.IsNearlyZero() ? Rotation.Yaw + 180.f : Away.Rotation().Yaw;
		Distance = Params.Distance;
	}
	const AActor* Target = Params.Direction == EAgLaunchDirection::Target ? AgAbility->GetActionTarget() : nullptr;
	if (Target)
	{
		const FVector Delta = (Target->GetActorLocation() - Character->GetActorLocation()) * FVector(1.f, 1.f, 0.f);
		Distance = FMath::Max(0.f, static_cast<float>(Delta.Size()) - AgAbility->GetTargetSpacing());
		if (Params.MaxDistance)
		{
			Distance = FMath::Min(Distance, *Params.MaxDistance);
		}
		Rotation = Delta.IsNearlyZero() ? Character->GetActorRotation() : Delta.Rotation();
	}
	const float Gravity = -Movement->GetGravityZ();
	Height = Params.Height ? *Params.Height : Gravity * FMath::Square(Params.AirTime.Get(0.f)) / 8.f;
	if (Params.Altitude)
	{
		FFindFloorResult Floor;
		Movement->ComputeFloorDist(Movement->UpdatedComponent->GetComponentLocation(), *Params.Altitude, *Params.Altitude, Floor, Character->GetCapsuleComponent()->GetScaledCapsuleRadius());
		Height = FMath::Max(Height, *Params.Altitude - (Floor.bBlockingHit ? Floor.FloorDist : *Params.Altitude));
	}
	const float AirTime = Params.AirTime.Get(FMath::Sqrt(8.f * Height / Gravity));

	// 엔진 Task의 생성 함수가 하는 설정입니다.
	ForceName = SourceName;
	Duration = AirTime;
	MinimumLandedTriggerTime = AirTime * MinLandedAirTimeRatio;
	bFinishOnLanded = true;
	FinishVelocityMode = ERootMotionFinishVelocityMode::SetVelocity;
	FinishSetVelocity = FVector::ZeroVector;
	SharedInitAndApply();

	// 엔진은 정점을 알리면 이 값을 끄고, 이미 공중이면 이동 모드도 바뀌지 않으므로 공중에서 다시 띄울 때도 켭니다.
	Character->GetCharacterMovement()->bNotifyApex = true;

	bMontageRootMotionDisabled = true;
	Character->PushDisableMontageRootMotion();
	Super::Activate();
}

void UAgAbilityTask_Launch::TickTask(float DeltaTime)
{
	AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor());
	if (IsFinished() || !Character || !MovementComponent.IsValid())
	{
		if (!IsFinished())
		{
			Finish();
		}
		return;
	}

	// 궤적을 마친 뒤에는 소스가 없어 엔진 착지가 착지를 알리므로, 땅에 닿으면 끝내기만 합니다.
	const bool bOnGround = MovementComponent->IsMovingOnGround();
	if (bArcEnded)
	{
		if (bOnGround)
		{
			Finish();
		}
		return;
	}

	// 부모 틱은 기억한 조기 착지만으로 Task를 끝내므로, 현재 접지를 먼저 확인하고 착지 알림까지 이어 줍니다.
	if (GetWorld()->GetTimeSeconds() >= StartTime + MinimumLandedTriggerTime && bOnGround)
	{
		Finish();
		Character->NotifyLanded();
		return;
	}
	if (!bOnGround)
	{
		bHasLanded = false;
		// 착지로 끝내는 엔진 소스는 체공 시간이 지나면 끝 속도로 등속 이동하므로, 공중에서 소스를 지워 그 속도로
		// 엔진 중력을 따라 떨어지게 합니다. 착지할 때까지 몽타주 루트 모션은 계속 끕니다.
		const TSharedPtr<FRootMotionSource> Source = MovementComponent->GetRootMotionSourceByID(RootMotionSourceID);
		if (Source.IsValid() && Source->GetTime() >= Source->Duration)
		{
			Source->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::MaintainLastRootMotionVelocity;
			MovementComponent->RemoveRootMotionSourceByID(RootMotionSourceID);
			bArcEnded = true;
			return;
		}
	}
	Super::TickTask(DeltaTime);
}

void UAgAbilityTask_Launch::OnDestroy(bool bInOwnerFinished)
{
	AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor());
	if (MovementComponent.IsValid())
	{
		// 엔진의 착지 콜백은 이동 모드가 걷기로 바뀌기 전에 Task를 끝내므로, 착지는 bHasLanded로도 봅니다.
		// 엔진 착지는 수평 속도를 남겨 바닥 마찰로 미끄러지므로, 착지해 끝나면 속도를 0으로 둡니다.
		if (bHasLanded || MovementComponent->IsMovingOnGround())
		{
			MovementComponent->StopMovementImmediately();
		}
		else if (const TSharedPtr<FRootMotionSource> Source = MovementComponent->GetRootMotionSourceByID(RootMotionSourceID))
		{
			// 공중에서 취소되어도(저글이 넉업을 이어받는 등) 그 속도로 떨어지게 합니다.
			Source->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::MaintainLastRootMotionVelocity;
		}
	}
	if (bMontageRootMotionDisabled && Character)
	{
		Character->PopDisableMontageRootMotion();
	}
	Super::OnDestroy(bInOwnerFinished);
}
