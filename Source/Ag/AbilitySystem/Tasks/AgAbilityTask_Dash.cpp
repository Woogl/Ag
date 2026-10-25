// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_Dash.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Character/AgCharacterBase.h"
#include "Core/AgGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"

UAbilityTask* FAgDashParams::MakeTask(const FAgTaskContext& Context) const
{
	UAgAbilityTask_Dash* Task = UAbilityTask::NewAbilityTask<UAgAbilityTask_Dash>(&Context.Ability, TEXT("AgDash"));
	Task->Params = *this;
	return Task;
}

void UAgAbilityTask_Dash::Activate()
{
	AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor());
	const UAgGameplayAbility* AgAbility = CastChecked<UAgGameplayAbility>(Ability);
	const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Movement || !AbilitySystemComponent.IsValid() || Params.Speed <= 0.f)
	{
		EndTask();
		return;
	}

	const FVector Forward = Character->GetActorForwardVector().GetSafeNormal2D();
	Target = AgAbility->GetActionTarget();
	float Distance = Params.MaxDistance.Get(0.f);
	if (Target.IsValid())
	{
		const float ToTarget = static_cast<float>(FVector::Dist2D(Character->GetActorLocation(), Target->GetActorLocation())) + Params.Overshoot;
		Distance = Params.MaxDistance ? FMath::Min(ToTarget, *Params.MaxDistance) : ToTarget;
	}
	if (Distance <= 0.f || Forward.IsNearlyZero())
	{
		EndTask();
		return;
	}
	bDashing = true;
	AbilitySystemComponent->AddLooseGameplayTag(AgGameplayTags::State_Dashing);
	Character->PushDisableMontageRootMotion();
	AttackBlockedHandle = AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_AttackBlocked)
	                          .AddUObject(this, &ThisClass::HandleAttackBlocked);
	MoveBlockedHandle = AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_MoveBlocked)
	                        .AddUObject(this, &ThisClass::HandleMoveBlocked);

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
		if (bDashing)
		{
			AbilitySystemComponent->RemoveLooseGameplayTag(AgGameplayTags::State_Dashing);
		}
	}
	AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor());
	if (bDashing && Character)
	{
		Character->PopDisableMontageRootMotion();
	}
	Super::OnDestroy(bInOwnerFinished);
}

void UAgAbilityTask_Dash::HandleAttackBlocked(const FGameplayEventData* Payload)
{
	// 막힘 이벤트는 주인의 모든 공격(앞 행동이 쏜 투사체 포함)에 오므로, 이 행동의 공격이 막혔을 때만 멈춥니다.
	if (Payload && Payload->ContextHandle.GetAbilityInstance_NotReplicated() == Ability)
	{
		EndTask();
	}
}

void UAgAbilityTask_Dash::HandleMoveBlocked(const FGameplayEventData* Payload)
{
	EndTask();
}
