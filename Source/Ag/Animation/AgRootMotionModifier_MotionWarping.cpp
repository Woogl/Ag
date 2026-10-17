// Copyright Woogle. All Rights Reserved.
#include "Animation/AgRootMotionModifier_MotionWarping.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Character/AgCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MotionWarpingComponent.h"
#include "Ag.h"

void UAgRootMotionModifier_MotionWarping::Update(const FMotionWarpingUpdateContext& Context)
{
	AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetActorOwner());
	if (!Character || Context.Animation != Animation || Context.PreviousPosition < StartTime || Context.PreviousPosition >= EndTime)
	{
		Super::Update(Context);
		return;
	}
	if (!bInitialized)
	{
		bInitialized = true;
		UAnimInstance* Anim = Character->GetMesh()->GetAnimInstance();
		const FAnimMontageInstance* RootInstance = Anim ? Anim->GetRootMotionMontageInstance() : nullptr;
		const FAnimMontageInstance* Instance = RootInstance && RootInstance->Montage == Animation.Get() ? RootInstance : nullptr;
		UAgGameplayAbility* Owner = Cast<UAgGameplayAbility>(Character->GetAbilitySystemComponent()->GetAnimatingAbility());
		AActor* Selected = nullptr;
		if (!Owner || !Instance || !Owner->PrepareWarp(Instance->GetInstanceID(), StartTime, ParamsId, Params, Selected, Gap, bFacingOnly))
		{
			UE_LOG(LogAg, Error, TEXT("MotionWarping.%s: missing playback owner or Params (animation %s, window %.3f..%.3f)"),
			    *ParamsId.ToString(), *GetNameSafe(Animation.Get()), StartTime, EndTime);
			SetState(ERootMotionModifierState::Disabled);
			return;
		}
		MontageInstanceId = Instance->GetInstanceID();
		OwnerAbility = Owner;
		Target = Selected;
		Origin = Character->GetActorLocation();
		Owner->RegisterWarp(this);
		// 실행마다 자기 엔진 목표 이름을 사용하여 다른 구간의 목표를 덮거나 제거하지 않습니다.
		WarpTargetName = FName(*FString::Printf(TEXT("%s_%d_%u"), *WarpTargetName.ToString(), MontageInstanceId, GetUniqueID()));
		if (bFacingOnly)
		{
			bWarpTranslation = false;
		}
	}
	if (!OwnerAbility.IsValid() || !OwnerAbility->OwnsMontage(MontageInstanceId) || !Target.IsValid() ||
	    (!bFacingOnly && Params.bHasTargetingRange && Character->GetHorizontalDistanceTo(Target.Get()) > Params.TargetingRange))
	{
		SetState(ERootMotionModifierState::Disabled);
		return;
	}
	if (RotationMethod == EMotionWarpRotationMethod::ConstantRate && FMath::Abs(Context.PlayRate) > UE_SMALL_NUMBER)
	{
		WarpMaxRotationRate = Character->GetCharacterMovement()->RotationRate.Yaw / FMath::Abs(Context.PlayRate);
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
