// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_Track.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "Animation/AgRootMotionModifier_MotionWarping.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/AgCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MotionWarpingComponent.h"

UAbilityTask* FAgTrackParams::MakeTask(const FAgTaskContext& Context) const
{
	return UAgAbilityTask_Track::Track(&Context.Runner, *this, Context.MontageInstanceId, Context.Start, Context.End);
}

UAgAbilityTask_Track::UAgAbilityTask_Track()
{
	bTickingTask = true;
}

UAgAbilityTask_Track* UAgAbilityTask_Track::Track(UGameplayAbility* Owner, const FAgTrackParams& InParams, int32 InMontageInstanceId, float InStartTime, float InEndTime)
{
	UAgAbilityTask_Track* Task = NewAbilityTask<UAgAbilityTask_Track>(Owner);
	Task->Params = InParams;
	Task->MontageInstanceId = InMontageInstanceId;
	Task->StartTime = InStartTime;
	Task->EndTime = InEndTime;
	return Task;
}

void UAgAbilityTask_Track::Activate()
{
	const UAgGameplayAbility* Runner = Cast<UAgGameplayAbility>(Ability);
	AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor());
	UAnimInstance* Anim = Character ? Character->GetMesh()->GetAnimInstance() : nullptr;
	const FAnimMontageInstance* Instance = Anim ? Anim->GetMontageInstanceForID(MontageInstanceId) : nullptr;
	if (!Runner || !Instance || !Instance->Montage)
	{
		EndTask();
		return;
	}
	AActor* ActionTarget = Runner->GetActionTarget();
	bFacingOnly = !ActionTarget || (Params.TargetingRange && Character->GetHorizontalDistanceTo(ActionTarget) > *Params.TargetingRange);
	Target = bFacingOnly ? Character->GetFallbackFacingTarget() : ActionTarget;
	LastPosition = Instance->GetPosition();
	if (!Target.IsValid())
	{
		EndTask();
		return;
	}

	// Modifier를 켜고 지우는 것과 루트 모션 보정은 엔진 Motion Warping이 재생 위치로 합니다.
	UMotionWarpingComponent* Warping = Character->GetMotionWarping();
	if (Warping && Instance->Montage->HasRootMotion())
	{
		UAgRootMotionModifier_MotionWarping* NewModifier = NewObject<UAgRootMotionModifier_MotionWarping>(Warping);
		NewModifier->Animation = Instance->Montage.Get();
		NewModifier->StartTime = StartTime;
		NewModifier->EndTime = EndTime;
		NewModifier->SetTrack(Params, Target.Get(), Runner->GetStopDistance(), bFacingOnly);
		Warping->AddModifier(NewModifier);
		Modifier = NewModifier;
	}
}

void UAgAbilityTask_Track::TickTask(float DeltaTime)
{
	AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor());
	UAnimInstance* Anim = Character ? Character->GetMesh()->GetAnimInstance() : nullptr;
	const FAnimMontageInstance* Instance = Anim ? Anim->GetMontageInstanceForID(MontageInstanceId) : nullptr;
	if (bStopped || !Instance || !Instance->Montage)
	{
		return;
	}
	const float Previous = LastPosition;
	const float Position = Instance->GetPosition();
	LastPosition = Position;

	// 몽타주가 루트 모션을 주는 동안은 Modifier가 돌립니다.
	if (Instance->Montage->HasRootMotion() && !Instance->IsRootMotionDisabled())
	{
		return;
	}

	const AActor* Goal = Target.Get();
	bStopped = !Goal ||
	           (!bFacingOnly && Params.TargetingRange && Character->GetHorizontalDistanceTo(Goal) > *Params.TargetingRange);
	const FVector ToTarget = Goal ? (Goal->GetActorLocation() - Character->GetActorLocation()).GetSafeNormal2D() : FVector::ZeroVector;
	if (bStopped || ToTarget.IsNearlyZero())
	{
		return;
	}

	// ConstantRate의 틱 시간에는 주인의 시간 배율이 이미 들어 있습니다.
	const float CurrentYaw = static_cast<float>(Character->GetActorRotation().Yaw);
	const float GoalYaw = static_cast<float>(ToTarget.Rotation().Yaw);
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

void UAgAbilityTask_Track::OnDestroy(bool bInOwnerFinished)
{
	// 엔진 Motion Warping이 다음 갱신에서 지우도록 표시만 합니다.
	if (UAgRootMotionModifier_MotionWarping* Warp = Modifier.Get())
	{
		Warp->SetState(ERootMotionModifierState::MarkedForRemoval);
	}
	Modifier.Reset();
	Super::OnDestroy(bInOwnerFinished);
}
