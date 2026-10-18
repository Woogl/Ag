// Copyright Woogle. All Rights Reserved.
#include "Animation/AgRootMotionModifier_MotionWarping.h"
#include "Character/AgCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MotionWarpingComponent.h"

UAgRootMotionModifier_MotionWarping::UAgRootMotionModifier_MotionWarping(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 엔진 기본값 외에 추적 구간이 모두 함께 쓰는 설정입니다.
	WarpTargetName = TEXT("AttackTarget");
}

void UAgRootMotionModifier_MotionWarping::SetTrack(const FAgTrackParams& InParams, AActor* InTarget, float InGap, bool bInFacingOnly)
{
	Params = InParams;
	Target = InTarget;
	Gap = InGap;
	bFacingOnly = bInFacingOnly;
	RotationMethod = Params.RotationMethod;
	bWarpTranslation = !bFacingOnly && !(Params.bHasMaxWarpDistance && Params.MaxWarpDistance <= 0.f);
	// 실행마다 자기 엔진 목표 이름을 사용하여 다른 구간의 목표를 덮거나 제거하지 않습니다.
	WarpTargetName = FName(*FString::Printf(TEXT("%s_%u"), *WarpTargetName.ToString(), GetUniqueID()));
}

void UAgRootMotionModifier_MotionWarping::Update(const FMotionWarpingUpdateContext& Context)
{
	// 추적 Task가 끝나며 지운 Modifier는 엔진이 이번 갱신 뒤에 지우도록 다른 상태로 바꾸지 않습니다.
	if (GetState() == ERootMotionModifierState::MarkedForRemoval)
	{
		return;
	}
	AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetActorOwner());
	if (!Character || Context.Animation != Animation || Context.PreviousPosition < StartTime || Context.PreviousPosition >= EndTime)
	{
		Super::Update(Context);
		return;
	}
	if (!bInitialized)
	{
		bInitialized = true;
		Origin = Character->GetActorLocation();
	}
	if (!Target.IsValid() ||
		(!bFacingOnly && Params.bHasTargetingRange && Character->GetHorizontalDistanceTo(Target.Get()) > Params.TargetingRange))
	{
		SetState(ERootMotionModifierState::Disabled);
		return;
	}
	// 엔진 ConstantRate의 프레임당 회전 한도는 이동 틱의 시간 × 재생 배율(Rate Scale × 재생 속도) × 회전 한도라, 재생 배율로 나눠
	// 재생 배율과 관계없이 캐릭터 회전 속도로 돕니다.
	WarpMaxRotationRate = static_cast<float>(Character->GetCharacterMovement()->RotationRate.Yaw);
	if (FMath::Abs(Context.PlayRate) > UE_SMALL_NUMBER)
	{
		WarpMaxRotationRate /= FMath::Abs(Context.PlayRate);
	}
	const FVector Location = Character->GetActorLocation();
	const FVector Direction = (Target->GetActorLocation() - Location).GetSafeNormal2D();
	FVector Goal = Location;
	if (!bFacingOnly && Character->GetHorizontalDistanceTo(Target.Get()) > Gap)
	{
		Goal = Target->GetActorLocation() - Direction * Gap;
	}
	Goal.Z = Location.Z;
	if (!bFacingOnly && Params.bHasMaxWarpDistance)
	{
		Goal = Origin + (Goal - Origin).GetClampedToMaxSize2D(Params.MaxWarpDistance);
		Goal.Z = Location.Z;
	}
	GetOwnerComponent()->AddOrUpdateWarpTargetFromLocationAndRotation(
		WarpTargetName, Goal, Direction.IsNearlyZero() ? Character->GetActorRotation() : Direction.Rotation());
	bOwnsTarget = true;
	Super::Update(Context);
}

void UAgRootMotionModifier_MotionWarping::OnStateChanged(ERootMotionModifierState LastState)
{
	Super::OnStateChanged(LastState);
	if (bOwnsTarget && (GetState() == ERootMotionModifierState::MarkedForRemoval || GetState() == ERootMotionModifierState::Disabled))
	{
		if (GetOwnerComponent())
		{
			GetOwnerComponent()->RemoveWarpTarget(WarpTargetName);
		}
		bOwnsTarget = false;
	}
}
