// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_Dash.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "Character/AgCharacterBase.h"
#include "Data/AgCharacterData.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "Ag.h"

namespace
{
	/** 돌진 방향으로 예상 이동의 이 비율보다 덜 나아간 틱은 막힌 것으로 봅니다. */
	constexpr float BlockedMoveRatio = 0.7f;

	/** 막힌 틱이 이만큼 이어지면 돌진을 멈춥니다. 한 틱만 짧은 것은 턱을 오른 것일 수 있습니다. */
	constexpr int32 BlockedTicksToStop = 3;

	/** 예상 이동(cm)이 이보다 짧은 틱은 막혔는지 판단하지 않습니다(히트스톱으로 돌진이 거의 멈춘 틱). */
	constexpr float MinJudgedMove = 2.f;
}

void FAgDashParams::Validate(const FJsonObject& Json, const FString& Path, TArray<FString>& Errors)
{
	if (Speed <= 0.f || MaxDistance <= 0.f)
	{
		Errors.Add(Path + TEXT(": Speed and MaxDistance must be positive"));
	}
}

UAbilityTask* FAgDashParams::MakeTask(const FAgTaskContext& Context) const
{
	AAgCharacterBase* Character = Cast<AAgCharacterBase>(Context.Runner.GetAvatarActorFromActorInfo());
	AActor* Target = Character ? Character->FindAttackTarget() : nullptr;
	if (!Target)
	{
		UE_LOG(LogAg, Error, TEXT("%s Abilities.%s Dash: missing target input (play %d)"),
		    Character ? *Character->GetCharacterData()->GetName() : TEXT("None"), *Context.Runner.GetAbilityId().ToString(),
		    Context.MontageInstanceId);
		return nullptr;
	}
	return UAgAbilityTask_Dash::Dash(&Context.Runner, *this, Character->GetActorForwardVector(), Target->GetActorLocation(), Target);
}

UAgAbilityTask_Dash::UAgAbilityTask_Dash()
{
	bTickingTask = true;
}

UAgAbilityTask_Dash* UAgAbilityTask_Dash::Dash(
    UGameplayAbility* OwningAbility, const FVector& Direction, float Speed, float Distance, AActor* HomingTarget, float HomingTurnRate)
{
	UAgAbilityTask_Dash* Task = NewAbilityTask<UAgAbilityTask_Dash>(OwningAbility);
	Task->Direction = Direction.GetSafeNormal2D();
	Task->Speed = Speed;
	Task->Distance = Distance;
	Task->HomingTarget = HomingTarget;
	Task->HomingTurnRate = FMath::Max(0.f, HomingTurnRate);
	return Task;
}

UAgAbilityTask_Dash* UAgAbilityTask_Dash::Dash(
    UGameplayAbility* Owner, const FAgDashParams& Params, const FVector& InDirection, const FVector& TargetLocation, AActor* Target)
{
	const float DistanceToTarget = FVector::Dist2D(Owner->GetAvatarActorFromActorInfo()->GetActorLocation(), TargetLocation);
	return Dash(Owner, InDirection, Params.Speed, FMath::Min(DistanceToTarget + Params.Overshoot, Params.MaxDistance), Target,
	    Params.HomingTurnRate);
}

void UAgAbilityTask_Dash::Activate()
{
	const ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
	Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Movement.IsValid() || Speed <= 0.f || Distance <= 0.f || Direction.IsNearlyZero())
	{
		Finish();
		return;
	}

	if (Movement->CurrentRootMotion.HasOverrideVelocity())
	{
		EndTask();
		return;
	}
	LastLocation = Character->GetActorLocation();

	// 제거할 때까지 다른 속도를 덮어씁니다. 모션의 루트 이동은 덮지 못하므로 쓰는 쪽에서 꺼야 합니다.
	TSharedPtr<FRootMotionSource_ConstantForce> Source = MakeShared<FRootMotionSource_ConstantForce>();
	Source->InstanceName = TEXT("AgDash");
	Source->AccumulateMode = ERootMotionAccumulateMode::Override;
	Source->Priority = 500;
	Source->Force = Direction * Speed;
	Source->Duration = -1.f;
	Source->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
	Source->FinishVelocityParams.SetVelocity = FVector::ZeroVector;
	SourceID = Movement->ApplyRootMotionSource(Source);
}

void UAgAbilityTask_Dash::TickTask(float DeltaTime)
{
	AActor* Avatar = GetAvatarActor();
	if (!Avatar || IsFinished())
	{
		return;
	}

	const FVector Location = Avatar->GetActorLocation();
	const FVector Travel = (Location - LastLocation) * FVector(1.f, 1.f, 0.f);
	TraveledDistance += Travel.Size();
	if (TraveledDistance >= Distance)
	{
		Finish();
		return;
	}

	// 벽에 막히면 그 자리에서 멈춥니다. 비스듬히 부딪히면 벽을 따라 미끄러지므로 돌진 방향으로 나아간 거리만 봅니다.
	// 히트스톱은 주인의 시간을 늦추므로 그동안의 틱은 예상 이동이 거의 없습니다.
	const float Expected = Speed * DeltaTime;
	if (Expected >= MinJudgedMove)
	{
		const float Progress = FVector::DotProduct(Travel, Direction);
		BlockedTicks = Progress < Expected * BlockedMoveRatio ? BlockedTicks + 1 : 0;
	}
	LastLocation = Location;
	if (BlockedTicks >= BlockedTicksToStop)
	{
		Finish();
		return;
	}

	// 이번 이동의 막힘을 이전 방향으로 판정한 뒤, 다음 이동에 쓸 방향을 갱신합니다.
	if (HomingTurnRate > 0.f && HomingTarget.IsValid() && Movement.IsValid() && DeltaTime > 0.f)
	{
		const FVector ToTarget = (HomingTarget->GetActorLocation() - Location).GetSafeNormal2D();
		if (!ToTarget.IsNearlyZero())
		{
			const float Yaw = FMath::FixedTurn(Direction.Rotation().Yaw, ToTarget.Rotation().Yaw, HomingTurnRate * DeltaTime);
			Direction = FRotator(0.f, Yaw, 0.f).Vector();
			const TSharedPtr<FRootMotionSource> Source = Movement->GetRootMotionSourceByID(SourceID);
			if (Source.IsValid())
			{
				StaticCastSharedPtr<FRootMotionSource_ConstantForce>(Source)->Force = Direction * Speed;
				FRotator Rotation = Avatar->GetActorRotation();
				Rotation.Yaw = Yaw;
				Avatar->SetActorRotation(Rotation);
			}
		}
	}
}

void UAgAbilityTask_Dash::Finish()
{
	RemoveSource();
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnFinished.Broadcast();
	}
	EndTask();
}

void UAgAbilityTask_Dash::RemoveSource()
{
	if (SourceID != 0 && Movement.IsValid())
	{
		Movement->RemoveRootMotionSourceByID(SourceID);
		Movement->Velocity = FVector::ZeroVector;
	}
	SourceID = 0;
}

void UAgAbilityTask_Dash::OnDestroy(bool bInOwnerFinished)
{
	RemoveSource();
	Super::OnDestroy(bInOwnerFinished);
}
