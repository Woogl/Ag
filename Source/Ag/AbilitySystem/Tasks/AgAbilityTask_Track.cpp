// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_Track.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "Animation/AgRootMotionModifier_MotionWarping.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/AgCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/AgJsonRead.h"
#include "Data/AgMontageTimeline.h"
#include "Dom/JsonObject.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MotionWarpingComponent.h"

void FAgTrackParams::GetFieldRules(FAgJsonFieldRules& Rules) const
{
	Rules.Nullable.Add(GET_MEMBER_NAME_CHECKED(FAgTrackParams, TargetingRange));
	Rules.Nullable.Add(GET_MEMBER_NAME_CHECKED(FAgTrackParams, MaxWarpDistance));
}

void FAgTrackParams::Validate(const FJsonObject& Json, EAgTaskPlacement Placement, const FString& Path, TArray<FString>& Errors)
{
	const TSharedPtr<FJsonValue> Range = Json.TryGetField(TEXT("TargetingRange"));
	const TSharedPtr<FJsonValue> Distance = Json.TryGetField(TEXT("MaxWarpDistance"));
	bHasTargetingRange = Range && Range->Type != EJson::Null;
	bHasMaxWarpDistance = Distance && Distance->Type != EJson::Null;
	if (RotationMethod != EMotionWarpRotationMethod::ConstantRate && RotationMethod != EMotionWarpRotationMethod::Slerp)
	{
		Errors.Add(Path + TEXT(".RotationMethod: expected ConstantRate or Slerp"));
	}
}

void FAgTrackParams::ValidateLinks(const FAgTaskLinks& Links, const FString& Path, TArray<FString>& Errors) const
{
	if (!Links.Montage)
	{
		return;
	}
	if (AgMontageTimeline::OverlapsLoop(*Links.Montage, Links.Start, Links.End))
	{
		Errors.Add(Path + TEXT(": a tracking window can't overlap a looping section"));
	}
	// 몽타주 루트 모션이 없는 구간(루트 모션 소스로 움직임)은 워핑할 이동이 없어 돌기만 합니다.
	if (!AgMontageTimeline::GivesRootMotion(*Links.Montage, Links.Start, Links.End) && !(bHasMaxWarpDistance && MaxWarpDistance <= 0.f))
	{
		Errors.Add(Path + TEXT(": a tracking window without montage root motion only turns, so MaxWarpDistance must be 0"));
	}
}

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
	bFacingOnly = !ActionTarget || (Params.bHasTargetingRange && Character->GetHorizontalDistanceTo(ActionTarget) > Params.TargetingRange);
	Target = bFacingOnly ? Character->GetFallbackFacingTarget() : ActionTarget;
	LastPosition = Instance->GetPosition();
	if (!Target.IsValid())
	{
		// 향할 대상이 없으면 모션 그대로 둡니다.
		EndTask();
		return;
	}

	// 몽타주 루트 모션이 있으면 이 구간의 Modifier를 엔진 Motion Warping에 더합니다. 켜고(앞 재생 위치가 구간 안) 지우는 것(구간을
	// 지남)과 루트 모션 보정은 엔진이 재생 위치로 합니다.
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

	// 따라 돌다가 대상이 TargetingRange 밖으로 나가면 이 구간에서는 멈춥니다.
	const AActor* Goal = Target.Get();
	bStopped = !Goal ||
	           (!bFacingOnly && Params.bHasTargetingRange && Character->GetHorizontalDistanceTo(Goal) > Params.TargetingRange);
	const FVector ToTarget = Goal ? (Goal->GetActorLocation() - Character->GetActorLocation()).GetSafeNormal2D() : FVector::ZeroVector;
	if (bStopped || ToTarget.IsNearlyZero())
	{
		return;
	}

	// ConstantRate는 초당 캐릭터 회전 속도만큼(틱 시간에 주인의 시간 배율이 들어 있음), Slerp는 구간 끝에 다 돌도록 남은 각도를
	// 나눠 돕니다.
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
	// 구간이 끝나거나 재생이 바뀌면 Modifier를 지우도록 표시합니다. 엔진이 다음 갱신에서 지웁니다.
	if (UAgRootMotionModifier_MotionWarping* Warp = Modifier.Get())
	{
		Warp->SetState(ERootMotionModifierState::MarkedForRemoval);
	}
	Modifier.Reset();
	Super::OnDestroy(bInOwnerFinished);
}
