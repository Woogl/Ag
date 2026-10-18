// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_Launch.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Data/AgJsonRead.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "Ag.h"

namespace
{
	/** 띄운 직후 아직 땅에 있는 것을 착지로 보지 않도록, 체공 시간의 이 비율이 지난 뒤의 착지만 봅니다. */
	constexpr float MinLandedAirTimeRatio = 0.5f;
}

void FAgLaunchParams::GetFieldRules(FAgJsonFieldRules& Rules) const
{
	Rules.Optional.Add(GET_MEMBER_NAME_CHECKED(FAgLaunchParams, Height));
	Rules.Optional.Add(GET_MEMBER_NAME_CHECKED(FAgLaunchParams, To));
	Rules.Optional.Add(GET_MEMBER_NAME_CHECKED(FAgLaunchParams, MaxDistance));
}

void FAgLaunchParams::Validate(const FJsonObject& Json, EAgTaskPlacement Placement, const FString& Path, TArray<FString>& Errors)
{
	bHasHeight = Json.HasField(TEXT("Height"));
	bHasTo = Json.HasField(TEXT("To"));
	bHasMaxDistance = Json.HasField(TEXT("MaxDistance"));
	if (AirTime <= 0.f)
	{
		Errors.Add(Path + TEXT(".AirTime: must be positive"));
	}
	if (bHasHeight && Height <= 0.f)
	{
		Errors.Add(Path + TEXT(".Height: must be positive"));
	}
	if (bHasMaxDistance && !bHasTo)
	{
		Errors.Add(Path + TEXT(".MaxDistance: only with To"));
	}
}

UAbilityTask* FAgLaunchParams::MakeTask(const FAgTaskContext& Context) const
{
	return UAgAbilityTask_Launch::Launch(&Context.Runner, *this);
}

const FName UAgAbilityTask_Launch::SourceName(TEXT("AgLaunch"));

UAgAbilityTask_Launch::UAgAbilityTask_Launch()
{
	bTickingTask = true;
}

UAgAbilityTask_Launch* UAgAbilityTask_Launch::Launch(UGameplayAbility* Owner, const FAgLaunchParams& InParams)
{
	UAgAbilityTask_Launch* Task = NewAbilityTask<UAgAbilityTask_Launch>(Owner);
	Task->Params = InParams;
	return Task;
}

void UAgAbilityTask_Launch::Activate()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
	const UAgGameplayAbility* Runner = Cast<UAgGameplayAbility>(Ability);
	Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Movement.IsValid() || !Runner)
	{
		EndTask();
		return;
	}

	// To Target이면 대상 앞, 그 행동의 간격만큼 떨어진 곳에 내립니다. 대상이 없으면 제자리에서 띄웁니다.
	FRotator Rotation = Character->GetActorRotation();
	float Distance = 0.f;
	const AActor* Target = Params.bHasTo ? Runner->GetActionTarget() : nullptr;
	if (Target)
	{
		const FVector Delta = (Target->GetActorLocation() - Character->GetActorLocation()) * FVector(1.f, 1.f, 0.f);
		Distance = FMath::Max(0.f, static_cast<float>(Delta.Size()) - Runner->GetStopDistance());
		Distance = Params.bHasMaxDistance ? FMath::Min(Distance, Params.MaxDistance) : Distance;
		if (!Delta.IsNearlyZero())
		{
			Rotation = Delta.Rotation();
		}
	}
	const float Height = Params.bHasHeight ? Params.Height : -Movement->GetGravityZ() * FMath::Square(Params.AirTime) / 8.f;

	// 착지할 때까지 다른 속도를 덮어쓰고, 지울 때 속도를 0으로 둡니다.
	TSharedPtr<FRootMotionSource_JumpForce> JumpForce = MakeShared<FRootMotionSource_JumpForce>();
	JumpForce->InstanceName = SourceName;
	JumpForce->AccumulateMode = ERootMotionAccumulateMode::Override;
	JumpForce->Priority = 500;
	JumpForce->Duration = Params.AirTime;
	JumpForce->Rotation = Rotation;
	JumpForce->Distance = Distance;
	JumpForce->Height = Height;
	JumpForce->bDisableTimeout = true;
	JumpForce->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
	JumpForce->FinishVelocityParams.SetVelocity = FVector::ZeroVector;
	SourceID = Movement->ApplyRootMotionSource(JumpForce);
	Character->LandedDelegate.AddDynamic(this, &ThisClass::HandleLanded);
}

void UAgAbilityTask_Launch::TickTask(float DeltaTime)
{
	Elapsed += DeltaTime;
	const ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
	if (!Character || !Movement.IsValid())
	{
		EndTask();
		return;
	}
	// 캐릭터 무브먼트는 몽타주가 루트 모션을 주면(루트 모션이 없는 섹션이라도 몽타주에 루트 모션이 있고 끄기 노티파이 밖이면)
	// 루트 모션 소스를 쓰지 않습니다.
	const FAnimMontageInstance* Instance = Character->GetRootMotionAnimMontageInstance();
	if (!bReportedRootMotion && Instance && Instance->Montage && Instance->Montage->HasRootMotion() && !Instance->IsRootMotionDisabled())
	{
		bReportedRootMotion = true;
		UE_LOG(LogAg, Error, TEXT("Launch: %s gives montage root motion at %.3f, so the launch does not move %s"),
			*GetNameSafe(Instance->Montage), Instance->GetPosition(), *Character->GetName());
	}
	// 떠오르지 못하고 땅에 남았으면(낮은 천장 등) 착지 알림이 오지 않으므로 여기서 끝냅니다.
	if (Elapsed >= Params.AirTime * MinLandedAirTimeRatio && !Movement->IsFalling())
	{
		Finish();
	}
}

void UAgAbilityTask_Launch::OnDestroy(bool bInOwnerFinished)
{
	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActor()))
	{
		Character->LandedDelegate.RemoveDynamic(this, &ThisClass::HandleLanded);
	}
	if (SourceID != 0 && Movement.IsValid())
	{
		Movement->RemoveRootMotionSourceByID(SourceID);
		Movement->Velocity = FVector::ZeroVector;
	}
	SourceID = 0;
	Super::OnDestroy(bInOwnerFinished);
}

void UAgAbilityTask_Launch::HandleLanded(const FHitResult& Hit)
{
	if (Elapsed >= Params.AirTime * MinLandedAirTimeRatio)
	{
		Finish();
	}
}

void UAgAbilityTask_Launch::Finish()
{
	if (!IsFinished())
	{
		EndTask();
	}
}
