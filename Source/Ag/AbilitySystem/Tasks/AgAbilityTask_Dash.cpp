// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_Dash.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/AgGameplayTags.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"

UAbilityTask* FAgDashParams::MakeTask(const FAgTaskContext& Context) const
{
	return UAgAbilityTask_Dash::Dash(&Context.Ability, *this);
}

UAgAbilityTask_Dash* UAgAbilityTask_Dash::Dash(UGameplayAbility* Owner, const FAgDashParams& InParams)
{
	UAgAbilityTask_Dash* Task = NewAbilityTask<UAgAbilityTask_Dash>(Owner, TEXT("AgDash"));
	Task->Params = InParams;
	return Task;
}

void UAgAbilityTask_Dash::Activate()
{
	const ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
	const UAgGameplayAbility* AgAbility = Cast<UAgGameplayAbility>(Ability);
	const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Movement || !AgAbility || !AbilitySystemComponent.IsValid() || Params.Speed <= 0.f)
	{
		EndTask();
		return;
	}
	bTagAdded = true;
	AbilitySystemComponent->AddLooseGameplayTag(AgGameplayTags::State_Dashing);

	const FVector Forward = Character->GetActorForwardVector().GetSafeNormal2D();
	Target = AgAbility->GetActionTarget();
	float Distance = Target.IsValid()
	                     ? static_cast<float>(FVector::Dist2D(Character->GetActorLocation(), Target->GetActorLocation())) + Params.Overshoot
	                     : 0.f;
	Distance = Params.MaxDistance ? (Target.IsValid() ? FMath::Min(Distance, *Params.MaxDistance) : *Params.MaxDistance) : Distance;
	if (Distance <= 0.f || Forward.IsNearlyZero())
	{
		EndTask();
		return;
	}
	AttackBlockedHandle = AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_AttackBlocked)
	                          .AddUObject(this, &ThisClass::HandleBlocked);
	MoveBlockedHandle = AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_MoveBlocked)
	                        .AddUObject(this, &ThisClass::HandleBlocked);

	// 몽타주 루트 모션이 있으면 캐릭터 무브먼트가 루트 모션 소스를 쓰지 않으므로 돌진하는 동안 끕니다.
	UAnimInstance* Anim = Character->GetMesh()->GetAnimInstance();
	FAnimMontageInstance* Instance = Anim ? Anim->GetActiveInstanceForMontage(Ability->GetCurrentMontage()) : nullptr;
	if (Instance)
	{
		Instance->PushDisableRootMotion();
		MontageInstanceId = Instance->GetInstanceID();
	}

	// 엔진 Task의 생성 함수가 하는 설정입니다.
	ForceName = TEXT("AgDash");
	WorldDirection = Forward;
	Strength = Params.Speed;
	// 속도가 일정하고 막히면 바로 멈추므로, 거리를 속도로 나눈 시간이 지나면 휘어진 경로로도 그 거리를 간 것입니다.
	Duration = Distance / Params.Speed;
	bIsAdditive = false;
	bEnableGravity = false;
	FinishVelocityMode = ERootMotionFinishVelocityMode::SetVelocity;
	FinishSetVelocity = FVector::ZeroVector;
	SharedInitAndApply();
}

void UAgAbilityTask_Dash::TickTask(float DeltaTime)
{
	// 지속 시간이 지나면 엔진 Task가 끝냅니다.
	Super::TickTask(DeltaTime);
	if (IsFinished())
	{
		return;
	}
	AActor* Avatar = GetAvatarActor();
	if (!Avatar || !MovementComponent.IsValid())
	{
		EndTask();
		return;
	}
	if (Params.HomingTurnRate <= 0.f || !Target.IsValid() || DeltaTime <= 0.f)
	{
		return;
	}
	const FVector ToTarget = (Target->GetActorLocation() - Avatar->GetActorLocation()).GetSafeNormal2D();
	const TSharedPtr<FRootMotionSource> Source = MovementComponent->GetRootMotionSourceByID(RootMotionSourceID);
	if (ToTarget.IsNearlyZero() || !Source.IsValid())
	{
		return;
	}
	const float Yaw = FMath::FixedTurn(WorldDirection.Rotation().Yaw, ToTarget.Rotation().Yaw, Params.HomingTurnRate * DeltaTime);
	WorldDirection = FRotator(0.f, Yaw, 0.f).Vector();
	StaticCastSharedPtr<FRootMotionSource_ConstantForce>(Source)->Force = WorldDirection * Strength;
	FRotator Rotation = Avatar->GetActorRotation();
	Rotation.Yaw = Yaw;
	Avatar->SetActorRotation(Rotation);
}

void UAgAbilityTask_Dash::OnDestroy(bool bInOwnerFinished)
{
	if (AbilitySystemComponent.IsValid())
	{
		if (AttackBlockedHandle.IsValid())
		{
			AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_AttackBlocked).Remove(AttackBlockedHandle);
		}
		if (MoveBlockedHandle.IsValid())
		{
			AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_MoveBlocked).Remove(MoveBlockedHandle);
		}
		if (bTagAdded)
		{
			AbilitySystemComponent->RemoveLooseGameplayTag(AgGameplayTags::State_Dashing);
		}
	}
	const ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
	UAnimInstance* Anim = Character && MontageInstanceId != INDEX_NONE ? Character->GetMesh()->GetAnimInstance() : nullptr;
	FAnimMontageInstance* Instance = Anim ? Anim->GetMontageInstanceForID(MontageInstanceId) : nullptr;
	if (Instance)
	{
		Instance->PopDisableRootMotion();
	}
	AttackBlockedHandle.Reset();
	MoveBlockedHandle.Reset();
	bTagAdded = false;
	MontageInstanceId = INDEX_NONE;
	Super::OnDestroy(bInOwnerFinished);
}

void UAgAbilityTask_Dash::HandleBlocked(const FGameplayEventData* Payload)
{
	EndTask();
}
