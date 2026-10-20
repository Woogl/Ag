// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_RootMotion.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/AgCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MotionWarpingComponent.h"
#include "RootMotionModifier_SkewWarp.h"

namespace
{
	// 구간 Task는 재생이 바뀌면 끝나므로, 켜져 있는 동안 행동의 지금 몽타주 재생은 늘 이 구간의 재생입니다.
	const FAnimMontageInstance* FindPlayingInstance(const UGameplayAbility* Ability, const AAgCharacterBase* Character)
	{
		const UAnimMontage* Montage = Ability ? Ability->GetCurrentMontage() : nullptr;
		const UAnimInstance* Anim = Character ? Character->GetMesh()->GetAnimInstance() : nullptr;
		return Montage && Anim ? Anim->GetActiveInstanceForMontage(Montage) : nullptr;
	}
}

UAbilityTask* FAgRootMotionParams::MakeTask(const FAgTaskContext& Context) const
{
	return UAgAbilityTask_RootMotion::RootMotion(&Context.Ability, *this, Context.Start, Context.End);
}

UAgAbilityTask_RootMotion::UAgAbilityTask_RootMotion()
{
	bTickingTask = true;
}

UAgAbilityTask_RootMotion* UAgAbilityTask_RootMotion::RootMotion(UGameplayAbility* Owner, const FAgRootMotionParams& InParams, float InStartTime, float InEndTime)
{
	UAgAbilityTask_RootMotion* Task = NewAbilityTask<UAgAbilityTask_RootMotion>(Owner);
	Task->Params = InParams;
	Task->StartTime = InStartTime;
	Task->EndTime = InEndTime;
	return Task;
}

void UAgAbilityTask_RootMotion::Activate()
{
	const UAgGameplayAbility* AgAbility = Cast<UAgGameplayAbility>(Ability);
	AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor());
	if (!AgAbility || !Character)
	{
		EndTask();
		return;
	}
	const bool bHasDuration = EndTime > StartTime;
	if (Params.To == EAgRootMotionTo::Target)
	{
		// 피격 방향은 이벤트의 공격자이므로 추적 거리로 제한하지 않습니다.
		if (bHasDuration)
		{
			Params.TargetingRange = FMath::Min(Params.TargetingRange.Get(AgAbility->GetTargetingRange()), AgAbility->GetTargetingRange());
		}
		AActor* ActionTarget = AgAbility->GetActionTarget();
		bFacingOnly = !ActionTarget || (bHasDuration && Params.TargetingRange && Character->GetHorizontalDistanceTo(ActionTarget) > *Params.TargetingRange);
		Target = bFacingOnly ? Character->GetLockOnTarget() : ActionTarget;
	}
	else if (Params.To == EAgRootMotionTo::MoveInput)
	{
		MoveInputDirection = Character->GetMoveInputDirection().GetSafeNormal2D();
		bFacingOnly = true;
	}

	// 발동 Task도 즉시 회전할 수 있도록 몽타주를 찾기 전에 적용합니다.
	if (!bHasDuration)
	{
		const FVector Direction = GetFacingDirection();
		if (!Direction.IsNearlyZero())
		{
			Character->SetActorRotation(FRotator(0.f, Direction.Rotation().Yaw, 0.f));
		}
		EndTask();
		return;
	}

	const FAnimMontageInstance* Instance = FindPlayingInstance(Ability, Character);
	if (!HasGoal() || !Instance || !Instance->Montage)
	{
		EndTask();
		return;
	}
	LastPosition = Instance->GetPosition();

	UMotionWarpingComponent* Warping = Character->GetMotionWarping();
	if (!Warping || !Instance->Montage->HasRootMotion())
	{
		return;
	}
	const FVector Location = Character->GetActorLocation();
	WarpGoal = Location;
	if (Params.To == EAgRootMotionTo::Direction)
	{
		WarpGoal += Character->GetActorForwardVector().GetSafeNormal2D().RotateAngleAxis(Params.Angle, FVector::UpVector) * Params.Distance;
	}
	else if (!bFacingOnly && Character->GetHorizontalDistanceTo(Target.Get()) > AgAbility->GetStopDistance())
	{
		const FVector TargetLocation = Target->GetActorLocation();
		const FVector Approach = TargetLocation - (TargetLocation - Location).GetSafeNormal2D() * AgAbility->GetStopDistance();
		WarpGoal = Params.MaxDistance ? Location + (Approach - Location).GetClampedToMaxSize2D(*Params.MaxDistance) : Approach;
	}
	const FVector Direction = GetFacingDirection();
	// 실행마다 자기 목표 이름을 써서 다른 구간의 목표를 덮거나 지우지 않습니다.
	WarpTargetName = FName(TEXT("AgRootMotion"), static_cast<int32>(GetUniqueID()));
	Warping->AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName, WarpGoal,
		Direction.IsNearlyZero() ? Character->GetActorRotation() : Direction.Rotation());

	// 엔진 ConstantRate의 프레임당 회전 한도는 이동 틱의 시간 × 재생 배율(Rate Scale × 재생 속도) × 회전 한도라,
	// 재생 배율로 나눠 재생 배율과 관계없이 캐릭터 회전 속도로 돕니다.
	const float PlayRate = FMath::Abs(Instance->Montage->RateScale * Instance->GetPlayRate());
	float MaxRotationRate = static_cast<float>(Character->GetCharacterMovement()->RotationRate.Yaw);
	if (PlayRate > UE_SMALL_NUMBER)
	{
		MaxRotationRate /= PlayRate;
	}
	// 켜기·지우기와 루트 모션 보정은 엔진 Motion Warping이 재생 위치로 합니다.
	Modifier = URootMotionModifier_SkewWarp::AddRootMotionModifierSkewWarp(Warping, Instance->Montage.Get(), StartTime, EndTime, WarpTargetName,
		EWarpPointAnimProvider::None, FTransform::Identity, NAME_None, Params.bWarpTranslation && !bFacingOnly, true,
		Params.To != EAgRootMotionTo::Direction, EMotionWarpRotationType::Default, Params.RotationMethod, 1.f, MaxRotationRate);
}

void UAgAbilityTask_RootMotion::TickTask(float DeltaTime)
{
	AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor());
	const FAnimMontageInstance* Instance = FindPlayingInstance(Ability, Character);
	if (bStopped || !Instance || !Instance->Montage)
	{
		return;
	}
	const float Previous = LastPosition;
	const float Position = Instance->GetPosition();
	LastPosition = Position;

	if (Params.To == EAgRootMotionTo::Target)
	{
		const AActor* Goal = Target.Get();
		bStopped = !Goal ||
		           (!bFacingOnly && Params.TargetingRange && Character->GetHorizontalDistanceTo(Goal) > *Params.TargetingRange);
	}
	if (bStopped)
	{
		// 워프 목표가 없어진 엔진 Modifier는 스스로 꺼지고, 그 뒤는 원래 루트 모션을 씁니다.
		RemoveWarpTarget();
		return;
	}
	const FVector Direction = GetFacingDirection();
	if (Direction.IsNearlyZero())
	{
		return;
	}

	// 몽타주가 루트 모션을 주는 동안은 엔진 Modifier가 워프 목표의 회전으로 돌립니다.
	if (Instance->Montage->HasRootMotion() && !Instance->IsRootMotionDisabled())
	{
		if (!WarpTargetName.IsNone())
		{
			Character->GetMotionWarping()->AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName, WarpGoal, Direction.Rotation());
		}
		return;
	}

	// ConstantRate의 틱 시간에는 주인의 시간 배율이 이미 들어 있습니다.
	const float CurrentYaw = static_cast<float>(Character->GetActorRotation().Yaw);
	const float GoalYaw = static_cast<float>(Direction.Rotation().Yaw);
	float Yaw = CurrentYaw;
	if (Params.RotationMethod == EMotionWarpRotationMethod::Slerp)
	{
		const float Remaining = EndTime - Previous;
		const float Alpha =
			Remaining > UE_SMALL_NUMBER ? FMath::Clamp((FMath::Min(Position, EndTime) - Previous) / Remaining, 0.f, 1.f) : 1.f;
		Yaw = CurrentYaw + FMath::FindDeltaAngleDegrees(CurrentYaw, GoalYaw) * Alpha;
	}
	else
	{
		Yaw = FMath::FixedTurn(CurrentYaw, GoalYaw, static_cast<float>(Character->GetCharacterMovement()->RotationRate.Yaw) * DeltaTime);
	}
	FRotator Rotation = Character->GetActorRotation();
	Rotation.Yaw = Yaw;
	Character->SetActorRotation(Rotation);
}

void UAgAbilityTask_RootMotion::OnDestroy(bool bInOwnerFinished)
{
	// 엔진 Motion Warping이 다음 갱신에서 지우도록 표시만 합니다.
	if (URootMotionModifier_SkewWarp* Warp = Modifier.Get())
	{
		Warp->SetState(ERootMotionModifierState::MarkedForRemoval);
	}
	Modifier.Reset();
	RemoveWarpTarget();
	Super::OnDestroy(bInOwnerFinished);
}

bool UAgAbilityTask_RootMotion::HasGoal() const
{
	return Params.To == EAgRootMotionTo::Direction ||
	       (Params.To == EAgRootMotionTo::Target ? Target.IsValid() : !MoveInputDirection.IsNearlyZero());
}

FVector UAgAbilityTask_RootMotion::GetFacingDirection() const
{
	const AActor* Avatar = GetAvatarActor();
	if (Params.To == EAgRootMotionTo::Target && Target.IsValid() && Avatar)
	{
		return (Target->GetActorLocation() - Avatar->GetActorLocation()).GetSafeNormal2D();
	}
	return MoveInputDirection;
}

void UAgAbilityTask_RootMotion::RemoveWarpTarget()
{
	const AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor());
	if (Character && !WarpTargetName.IsNone())
	{
		Character->GetMotionWarping()->RemoveWarpTarget(WarpTargetName);
	}
	WarpTargetName = NAME_None;
}
