// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_RootMotion.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystem/AgTargeting.h"
#include "AbilitySystem/Tasks/AgAbilityTask_PlayMontage.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/AgCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MotionWarpingComponent.h"
#include "RootMotionModifier_SkewWarp.h"

UAbilityTask* FAgRootMotionParams::MakeTask(const FAgTaskContext& Context) const
{
	UAgAbilityTask_RootMotion* Task = UAbilityTask::NewAbilityTask<UAgAbilityTask_RootMotion>(&Context.Ability);
	Task->Params = *this;
	Task->Window = Context.Window;
	Task->Playback = Context.Playback;
	return Task;
}

UAgAbilityTask_RootMotion::UAgAbilityTask_RootMotion()
{
	bTickingTask = true;
}

void UAgAbilityTask_RootMotion::Activate()
{
	const UAgGameplayAbility* AgAbility = CastChecked<UAgGameplayAbility>(Ability);
	AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor());
	const UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (!Character || !ASC || !ASC->HasAllMatchingGameplayTags(Params.RequiredTags) ||
		ASC->HasAnyMatchingGameplayTags(Params.BlockedTags))
	{
		EndTask();
		return;
	}
	if (Params.Direction == EAgRootMotionDirection::Target)
	{
		// 피격 방향은 이벤트의 공격자이므로 추적 거리로 제한하지 않습니다.
		if (Window)
		{
			TargetingRange = FMath::Min(Params.TargetingRange.Get(AgAbility->GetTargetingRange()), AgAbility->GetTargetingRange());
		}
		AActor* ActionTarget = AgAbility->GetActionTarget();
		bFacingOnly = !ActionTarget || (Window && !AgTargeting::IsInRange(*Character, *ActionTarget, TargetingRange));
		FacingTarget = bFacingOnly ? Character->GetLockOnTarget() : ActionTarget;
	}
	else if (Params.Direction == EAgRootMotionDirection::MoveInput)
	{
		MoveInputDirection = Character->GetMoveInputDirection().GetSafeNormal2D();
		bFacingOnly = true;
	}

	// 구간이 없는 발동 Task는 즉시 돕니다. 함께 시작하는 PlayMontage보다 먼저 시작할 수 있어 몽타주를 찾기 전에 돕니다.
	if (!Window)
	{
		const FVector Direction = GetFacingDirection();
		if (!Direction.IsNearlyZero())
		{
			Character->SetActorRotation(FRotator(0.f, Direction.Rotation().Yaw, 0.f));
		}
		EndTask();
		return;
	}

	const FAnimMontageInstance* Instance = Playback.IsValid() ? Playback->GetMontageInstance() : nullptr;
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
	// 바라보기만 하거나 대상과 이미 간격 안이면 지금 자리를 목표로 둬 다가가지 않습니다.
	WarpGoal = Location;
	if (Params.Direction == EAgRootMotionDirection::Back)
	{
		WarpGoal -= Character->GetActorForwardVector().GetSafeNormal2D() * Params.Distance;
	}
	else if (!bFacingOnly && Character->GetHorizontalDistanceTo(FacingTarget.Get()) > AgAbility->GetTargetSpacing())
	{
		const FVector TargetLocation = FacingTarget->GetActorLocation();
		WarpGoal = TargetLocation - (TargetLocation - Location).GetSafeNormal2D() * AgAbility->GetTargetSpacing();
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
	// 재생을 시작할 때 모두 등록하면 끊긴 재생의 대기 Modifier가 같은 몽타주의 다음 재생에서 함께 켜져 워핑이 두 번 걸리므로,
	// 구간마다 등록합니다. 켜기·지우기와 루트 모션 보정은 엔진 Motion Warping이 재생 위치로 합니다.
	Modifier = URootMotionModifier_SkewWarp::AddRootMotionModifierSkewWarp(Warping, Instance->Montage.Get(), Window->Min, Window->Max, WarpTargetName,
		EWarpPointAnimProvider::None, FTransform::Identity, NAME_None, Params.bWarpTranslation && !bFacingOnly, true,
		Params.Direction != EAgRootMotionDirection::Back, EMotionWarpRotationType::Default, Params.RotationMethod, 1.f, MaxRotationRate);
}

void UAgAbilityTask_RootMotion::TickTask(float DeltaTime)
{
	AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor());
	const FAnimMontageInstance* Instance = Playback.IsValid() ? Playback->GetMontageInstance() : nullptr;
	if (bStopped || !Character || !Instance || !Instance->Montage)
	{
		return;
	}
	const float Previous = LastPosition;
	const float Position = Instance->GetPosition();
	LastPosition = Position;

	if (Params.Direction == EAgRootMotionDirection::Target)
	{
		const AActor* Goal = FacingTarget.Get();
		bStopped = !Goal ||
		           (!bFacingOnly && !AgTargeting::IsInRange(*Character, *Goal, TargetingRange));
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

	// 루트 모션 소스로 돌리지 않는 것은, 엔진이 우선순위가 가장 높은 Override 소스 하나의 회전만 그 속도와 함께 적용해
	// Launch·Dash의 이동과 부딪히기 때문입니다. ConstantRate의 틱 시간에는 주인의 시간 배율이 이미 들어 있습니다.
	const float CurrentYaw = static_cast<float>(Character->GetActorRotation().Yaw);
	const float GoalYaw = static_cast<float>(Direction.Rotation().Yaw);
	float Yaw = CurrentYaw;
	if (Params.RotationMethod == EMotionWarpRotationMethod::Slerp)
	{
		const float Remaining = Window->Max - Previous;
		const float Alpha =
			Remaining > UE_SMALL_NUMBER ? FMath::Clamp((FMath::Min(Position, Window->Max) - Previous) / Remaining, 0.f, 1.f) : 1.f;
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
	RemoveWarpTarget();
	Super::OnDestroy(bInOwnerFinished);
}

bool UAgAbilityTask_RootMotion::HasGoal() const
{
	return Params.Direction == EAgRootMotionDirection::Back ||
	       (Params.Direction == EAgRootMotionDirection::Target ? FacingTarget.IsValid() : !MoveInputDirection.IsNearlyZero());
}

FVector UAgAbilityTask_RootMotion::GetFacingDirection() const
{
	const AActor* Avatar = GetAvatarActor();
	if (Params.Direction == EAgRootMotionDirection::Target && FacingTarget.IsValid() && Avatar)
	{
		return (FacingTarget->GetActorLocation() - Avatar->GetActorLocation()).GetSafeNormal2D();
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
