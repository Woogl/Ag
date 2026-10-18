// Copyright Woogle. All Rights Reserved.
#include "Animation/AgRootMotionModifier_MotionWarping.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Character/AgCharacterBase.h"
#include "Data/AgJsonRead.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MotionWarpingComponent.h"
#include "Ag.h"

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

UAgRootMotionModifier_MotionWarping::UAgRootMotionModifier_MotionWarping(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 엔진 기본값 외에 추적 구간이 모두 함께 쓰는 설정입니다.
	WarpTargetName = TEXT("AttackTarget");
}

void UAgRootMotionModifier_MotionWarping::SetTrackSettings(const FAgTrackParams& InParams)
{
	Params = InParams;
	RotationMethod = Params.RotationMethod;
	bWarpTranslation = !(Params.bHasMaxWarpDistance && Params.MaxWarpDistance <= 0.f);
}

void UAgRootMotionModifier_MotionWarping::Update(const FMotionWarpingUpdateContext& Context)
{
	// 재생을 맡은 Runner가 지운 구간(ClearWarps)은 엔진이 이번 갱신 뒤에 지우도록 그대로 둡니다. 다른 상태로 바꾸면 남아서 같은 몽타주의
	// 새 재생이 같은 구간에 Modifier를 더하지 못합니다.
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
		UAnimInstance* Anim = Character->GetMesh()->GetAnimInstance();
		const FAnimMontageInstance* RootInstance = Anim ? Anim->GetRootMotionMontageInstance() : nullptr;
		const FAnimMontageInstance* Instance = RootInstance && RootInstance->Montage == Animation.Get() ? RootInstance : nullptr;
		UAgGameplayAbility* Owner = Cast<UAgGameplayAbility>(Character->GetAbilitySystemComponent()->GetAnimatingAbility());
		AActor* Selected = nullptr;
		if (!Owner || !Instance || !Owner->PrepareTrackTarget(Instance->GetInstanceID(), Params, Selected, Gap, bFacingOnly))
		{
			UE_LOG(LogAg, Error, TEXT("Track: missing playback owner (animation %s, window %.3f..%.3f)"), *GetNameSafe(Animation.Get()),
				StartTime, EndTime);
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
	// 엔진 ConstantRate의 프레임당 회전 한도는 이동 틱의 시간 × 회전 속도라, 재생 배율과 관계없이 캐릭터 회전 속도로 돕니다.
	WarpMaxRotationRate = static_cast<float>(Character->GetCharacterMovement()->RotationRate.Yaw);
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
