// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_Launch.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/AgCharacterBase.h"
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
	return UAgAbilityTask_Launch::Launch(&Context.Ability, *this);
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
	const UAgGameplayAbility* AgAbility = Cast<UAgGameplayAbility>(Ability);
	const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Movement || !AgAbility)
	{
		EndTask();
		return;
	}

	Rotation = Character->GetActorRotation();
	Distance = 0.f;
	const AActor* Target = Params.To ? AgAbility->GetActionTarget() : nullptr;
	if (Target)
	{
		const FVector Delta = (Target->GetActorLocation() - Character->GetActorLocation()) * FVector(1.f, 1.f, 0.f);
		Distance = FMath::Max(0.f, static_cast<float>(Delta.Size()) - AgAbility->GetStopDistance());
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
	AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor());
	if (IsFinished() || !Character || !MovementComponent.IsValid())
	{
		if (!IsFinished())
		{
			Finish();
		}
		return;
	}

	// 부모 틱은 기억한 조기 착지만으로 Task를 끝내므로, 현재 접지를 먼저 확인하고 착지 알림까지 이어 줍니다.
	const bool bOnGround = MovementComponent->IsMovingOnGround();
	if (GetWorld()->GetTimeSeconds() >= StartTime + MinimumLandedTriggerTime && bOnGround)
	{
		Finish();
		Character->NotifyLanded();
		return;
	}
	if (!bOnGround)
	{
		bHasLanded = false;
	}
	Super::TickTask(DeltaTime);
	if (IsFinished())
	{
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
}
