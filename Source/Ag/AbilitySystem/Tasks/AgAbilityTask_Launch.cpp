// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_Launch.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "Ag.h"

namespace
{
	/** 띄운 직후 아직 땅에 있는 것을 착지로 보지 않도록, 체공 시간의 이 비율이 지난 뒤의 착지만 봅니다. */
	constexpr float MinLandedAirTimeRatio = 0.5f;
}

UAbilityTask* FAgLaunchParams::MakeTask(const FAgTaskContext& Context) const
{
	return UAgAbilityTask_Launch::Launch(&Context.Runner, *this);
}

const FName UAgAbilityTask_Launch::SourceName(TEXT("AgLaunch"));

UAgAbilityTask_Launch* UAgAbilityTask_Launch::Launch(UGameplayAbility* Owner, const FAgLaunchParams& InParams)
{
	UAgAbilityTask_Launch* Task = NewAbilityTask<UAgAbilityTask_Launch>(Owner, SourceName);
	Task->Params = InParams;
	return Task;
}

void UAgAbilityTask_Launch::Activate()
{
	const ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
	const UAgGameplayAbility* Runner = Cast<UAgGameplayAbility>(Ability);
	const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Movement || !Runner)
	{
		EndTask();
		return;
	}

	Rotation = Character->GetActorRotation();
	Distance = 0.f;
	const AActor* Target = Params.To ? Runner->GetActionTarget() : nullptr;
	if (Target)
	{
		const FVector Delta = (Target->GetActorLocation() - Character->GetActorLocation()) * FVector(1.f, 1.f, 0.f);
		Distance = FMath::Max(0.f, static_cast<float>(Delta.Size()) - Runner->GetStopDistance());
		Distance = Params.MaxDistance ? FMath::Min(Distance, *Params.MaxDistance) : Distance;
		if (!Delta.IsNearlyZero())
		{
			Rotation = Delta.Rotation();
		}
	}
	Height = Params.Height ? *Params.Height : -Movement->GetGravityZ() * FMath::Square(Params.AirTime) / 8.f;

	// 엔진 Task의 생성 함수가 하는 설정입니다.
	ForceName = SourceName;
	Duration = Params.AirTime;
	MinimumLandedTriggerTime = Params.AirTime * MinLandedAirTimeRatio;
	bFinishOnLanded = true;
	FinishVelocityMode = ERootMotionFinishVelocityMode::SetVelocity;
	FinishSetVelocity = FVector::ZeroVector;
	SharedInitAndApply();
	Super::Activate();
}

void UAgAbilityTask_Launch::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);
	const ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
	if (IsFinished() || !Character || !MovementComponent.IsValid())
	{
		if (!IsFinished())
		{
			Finish();
		}
		return;
	}
	// 캐릭터 무브먼트는 몽타주가 루트 모션을 주면(루트 모션이 없는 섹션이라도 몽타주에 루트 모션이 있고 끄기
	// 노티파이 밖이면) 루트 모션 소스를 쓰지 않습니다.
	const FAnimMontageInstance* Instance = Character->GetRootMotionAnimMontageInstance();
	if (!bReportedRootMotion && Instance && Instance->Montage && Instance->Montage->HasRootMotion() && !Instance->IsRootMotionDisabled())
	{
		bReportedRootMotion = true;
		UE_LOG(LogAg, Error, TEXT("Launch: %s gives montage root motion at %.3f, so the launch does not move %s"),
			*GetNameSafe(Instance->Montage), Instance->GetPosition(), *Character->GetName());
	}
	// 떠오르지 못하고 땅에 남았으면(낮은 천장 등) 착지 알림이 오지 않아 엔진 Task가 끝나지 않으므로, 착지를 보는
	// 때부터 땅에 있으면 끝냅니다.
	if (GetWorld()->GetTimeSeconds() >= StartTime + MinimumLandedTriggerTime && !MovementComponent->IsFalling())
	{
		Finish();
	}
}
