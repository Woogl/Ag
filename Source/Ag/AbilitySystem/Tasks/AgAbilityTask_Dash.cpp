// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_Dash.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgJsonRead.h"
#include "Data/AgMontageTimeline.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"

namespace
{
	constexpr float BlockedMoveRatio = 0.7f;

	/** 한 틱만 짧은 것은 턱을 오른 것일 수 있으므로 막힌 틱이 여러 번 이어질 때만 멈춥니다. */
	constexpr int32 BlockedTicksToStop = 3;

	constexpr float MinJudgedMove = 2.f;
}

void FAgDashParams::GetFieldRules(FAgJsonFieldRules& Rules) const
{
	Rules.Optional.Add(GET_MEMBER_NAME_CHECKED(FAgDashParams, Next));
	Rules.Optional.Add(TEXT("Next.BlendTime"));
	Rules.Optional.Add(GET_MEMBER_NAME_CHECKED(FAgDashParams, HomingTurnRate));
	Rules.Optional.Add(GET_MEMBER_NAME_CHECKED(FAgDashParams, Overshoot));
	Rules.Optional.Add(GET_MEMBER_NAME_CHECKED(FAgDashParams, MaxDistance));
}

void FAgDashParams::Validate(const FJsonObject& Json, EAgTaskPlacement Placement, const FString& Path, TArray<FString>& Errors)
{
	bHasMaxDistance = Json.HasField(TEXT("MaxDistance"));
	if (Speed <= 0.f)
	{
		Errors.Add(Path + TEXT(".Speed: must be positive"));
	}
	if (To != EAgDashTo::Target &&
		(Json.HasField(TEXT("HomingTurnRate")) || Json.HasField(TEXT("Overshoot")) || Json.HasField(TEXT("MaxDistance"))))
	{
		Errors.Add(Path + TEXT(": HomingTurnRate, Overshoot and MaxDistance are only for To Target"));
	}
}

void FAgDashParams::ValidateLinks(const FAgTaskLinks& Links, const FString& Path, TArray<FString>& Errors) const
{
	if (Links.Montage && AgMontageTimeline::GivesRootMotion(*Links.Montage, Links.Start, Links.End))
	{
		Errors.Add(Path + TEXT(": a dash window needs no montage root motion (a root motion off notify over the window)"));
	}
}

UAbilityTask* FAgDashParams::MakeTask(const FAgTaskContext& Context) const
{
	return UAgAbilityTask_Dash::Dash(&Context.Runner, *this, Context.MontageInstanceId);
}

UAgAbilityTask_Dash* UAgAbilityTask_Dash::Dash(UGameplayAbility* Owner, const FAgDashParams& InParams, int32 InMontageInstanceId)
{
	UAgAbilityTask_Dash* Task = NewAbilityTask<UAgAbilityTask_Dash>(Owner, TEXT("AgDash"));
	Task->Params = InParams;
	Task->MontageInstanceId = InMontageInstanceId;
	return Task;
}

void UAgAbilityTask_Dash::Activate()
{
	const ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
	const UAgGameplayAbility* Runner = Cast<UAgGameplayAbility>(Ability);
	const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Movement || !Runner || !AbilitySystemComponent.IsValid())
	{
		Stop();
		return;
	}

	FVector Velocity = FVector::ZeroVector;
	if (Params.To == EAgDashTo::Down)
	{
		if (!Movement->IsFalling())
		{
			Stop();
			return;
		}
		Velocity = FVector(Movement->Velocity.X, Movement->Velocity.Y, -Params.Speed);
	}
	else
	{
		const FVector Forward = Character->GetActorForwardVector().GetSafeNormal2D();
		Target = Runner->GetActionTarget();
		Distance = Target.IsValid()
		               ? static_cast<float>(FVector::Dist2D(Character->GetActorLocation(), Target->GetActorLocation())) + Params.Overshoot
		               : 0.f;
		Distance = Params.bHasMaxDistance ? (Target.IsValid() ? FMath::Min(Distance, Params.MaxDistance) : Params.MaxDistance) : Distance;
		if (Distance <= 0.f || Forward.IsNearlyZero())
		{
			Stop();
			return;
		}
		Velocity = Forward * Params.Speed;
		LastLocation = Character->GetActorLocation();
		BlockedHandle = AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_AttackBlocked)
		                    .AddUObject(this, &ThisClass::HandleAttackBlocked);
	}

	// 엔진 Task의 생성 함수가 하는 설정입니다. 몽타주 루트 모션이 있으면 소스가 먹지 않으므로 검증이 이 구간의
	// 몽타주 루트 모션을 막습니다.
	ForceName = TEXT("AgDash");
	WorldDirection = Velocity.GetSafeNormal();
	Strength = static_cast<float>(Velocity.Size());
	Duration = -1.f;
	bIsAdditive = false;
	bEnableGravity = false;
	FinishVelocityMode = ERootMotionFinishVelocityMode::SetVelocity;
	FinishSetVelocity = FVector::ZeroVector;
	SharedInitAndApply();
}

void UAgAbilityTask_Dash::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);
	if (IsFinished())
	{
		return;
	}
	if (!GetAvatarActor() || !MovementComponent.IsValid())
	{
		Stop();
		return;
	}
	if (Params.To == EAgDashTo::Down)
	{
		if (!MovementComponent->IsFalling())
		{
			Stop();
		}
		return;
	}
	TickTarget(DeltaTime);
}

void UAgAbilityTask_Dash::OnDestroy(bool bInOwnerFinished)
{
	if (BlockedHandle.IsValid() && AbilitySystemComponent.IsValid())
	{
		AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_AttackBlocked).Remove(BlockedHandle);
	}
	BlockedHandle.Reset();
	Super::OnDestroy(bInOwnerFinished);
}

void UAgAbilityTask_Dash::TickTarget(float DeltaTime)
{
	AActor* Avatar = GetAvatarActor();
	const FVector Location = Avatar->GetActorLocation();
	const FVector Travel = (Location - LastLocation) * FVector(1.f, 1.f, 0.f);
	TraveledDistance += Travel.Size();
	if (TraveledDistance >= Distance)
	{
		Stop();
		return;
	}

	// 비스듬히 부딪히면 벽을 따라 미끄러지므로 돌진 방향으로 나아간 거리만 봅니다. 히트스톱은 주인의 시간을
	// 늦추므로 그동안의 틱은 예상 이동이 거의 없어 판단하지 않습니다.
	const float Expected = Params.Speed * DeltaTime;
	if (Expected >= MinJudgedMove)
	{
		const float Progress = FVector::DotProduct(Travel, WorldDirection);
		BlockedTicks = Progress < Expected * BlockedMoveRatio ? BlockedTicks + 1 : 0;
	}
	LastLocation = Location;
	if (BlockedTicks >= BlockedTicksToStop)
	{
		Stop();
		return;
	}

	// 이번 이동의 막힘을 이전 방향으로 판정한 뒤, 다음 이동에 쓸 방향을 갱신합니다.
	if (Params.HomingTurnRate > 0.f && Target.IsValid() && DeltaTime > 0.f)
	{
		const FVector ToTarget = (Target->GetActorLocation() - Location).GetSafeNormal2D();
		const TSharedPtr<FRootMotionSource> Source = MovementComponent->GetRootMotionSourceByID(RootMotionSourceID);
		if (!ToTarget.IsNearlyZero() && Source.IsValid())
		{
			const float Yaw = FMath::FixedTurn(WorldDirection.Rotation().Yaw, ToTarget.Rotation().Yaw, Params.HomingTurnRate * DeltaTime);
			WorldDirection = FRotator(0.f, Yaw, 0.f).Vector();
			StaticCastSharedPtr<FRootMotionSource_ConstantForce>(Source)->Force = WorldDirection * Strength;
			FRotator Rotation = Avatar->GetActorRotation();
			Rotation.Yaw = Yaw;
			Avatar->SetActorRotation(Rotation);
		}
	}
}

void UAgAbilityTask_Dash::HandleAttackBlocked(const FGameplayEventData* Payload)
{
	Stop();
}

void UAgAbilityTask_Dash::Stop()
{
	if (IsFinished())
	{
		return;
	}
	// 전환이 이 구간을 끝내므로 전환하지 않았을 때만 스스로 끝냅니다. 소스는 끝날 때 엔진 Task가 지웁니다.
	if (UAgGameplayAbility* Runner = Cast<UAgGameplayAbility>(Ability); Runner && Params.Next.IsSet())
	{
		Runner->RequestTransition(Params.Next, MontageInstanceId);
	}
	if (!IsFinished())
	{
		EndTask();
	}
}
