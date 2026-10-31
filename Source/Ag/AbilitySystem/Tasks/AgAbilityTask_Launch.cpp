// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_Launch.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Character/AgCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "Core/AgGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"

UAbilityTask* FAgLaunchParams::MakeTask(const FAgTaskContext& Context) const
{
	UAgAbilityTask_Launch* Task = UAbilityTask::NewAbilityTask<UAgAbilityTask_Launch>(&Context.Ability);
	Task->Params = *this;
	return Task;
}

void UAgAbilityTask_Launch::Activate()
{
	AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor());
	const UAgGameplayAbility* AgAbility = CastChecked<UAgGameplayAbility>(Ability);
	UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Movement)
	{
		EndTask();
		return;
	}

	FVector Direction = FVector::ZeroVector;
	float Distance = 0.f;
	if (Params.Direction == EAgLaunchDirection::Back)
	{
		// 함께 시작하는 몸 돌리기와 순서가 없어도 같은 방향이 되도록, 대상이 있으면 정면 대신 대상의 반대쪽으로 띄웁니다.
		const AActor* ActionTarget = AgAbility->GetActionTarget();
		const FVector Away = ActionTarget ? (Character->GetActorLocation() - ActionTarget->GetActorLocation()).GetSafeNormal2D() : FVector::ZeroVector;
		Direction = Away.IsNearlyZero() ? -Character->GetActorForwardVector().GetSafeNormal2D() : Away;
		Distance = Params.Distance;
	}
	const AActor* Target = Params.Direction == EAgLaunchDirection::Target ? AgAbility->GetActionTarget() : nullptr;
	if (Target)
	{
		const FVector Delta = (Target->GetActorLocation() - Character->GetActorLocation()) * FVector(1.f, 1.f, 0.f);
		Distance = FMath::Max(0.f, static_cast<float>(Delta.Size()) - AgAbility->GetTargetSpacing());
		if (Params.MaxDistance)
		{
			Distance = FMath::Min(Distance, *Params.MaxDistance);
		}
		Direction = Delta.GetSafeNormal();
	}
	const float Gravity = -Movement->GetGravityZ();
	float Height = Params.Height ? *Params.Height : Gravity * FMath::Square(Params.AirTime.Get(0.f)) / 8.f;
	if (Params.Altitude)
	{
		FFindFloorResult Floor;
		Movement->ComputeFloorDist(Movement->UpdatedComponent->GetComponentLocation(), *Params.Altitude, *Params.Altitude, Floor, Character->GetCapsuleComponent()->GetScaledCapsuleRadius());
		Height = FMath::Max(Height, *Params.Altitude - (Floor.bBlockingHit ? Floor.FloorDist : *Params.Altitude));
	}
	// 띄운 높이로 돌아올 때까지 밀림 거리를 가도록 수평 속도를 정합니다.
	const float VerticalSpeed = FMath::Sqrt(2.f * Gravity * Height);
	const float AirTime = 2.f * VerticalSpeed / Gravity;
	const FVector HorizontalVelocity = AirTime > 0.f ? Direction * (Distance / AirTime) : FVector::ZeroVector;
	Character->LaunchCharacter(HorizontalVelocity + FVector(0.f, 0.f, VerticalSpeed), true, true);

	// 엔진은 정점을 알리면 이 값을 끄므로, 공중에서 다시 띄울 때도 켭니다.
	Movement->bNotifyApex = true;

	bMontageRootMotionDisabled = true;
	Character->PushDisableMontageRootMotion();
	LandedHandle = AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_Landed)
	                   .AddUObject(this, &ThisClass::HandleLanded);
}

void UAgAbilityTask_Launch::OnDestroy(bool bInOwnerFinished)
{
	if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
	{
		ASC->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_Landed).Remove(LandedHandle);
	}
	AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor());
	if (bMontageRootMotionDisabled && Character)
	{
		Character->PopDisableMontageRootMotion();
	}
	Super::OnDestroy(bInOwnerFinished);
}

void UAgAbilityTask_Launch::HandleLanded(const FGameplayEventData* Payload)
{
	// 엔진 착지는 수평 속도를 남겨 바닥 마찰로 미끄러지므로, 착지하면 그 자리에 멈춥니다.
	if (const AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor()))
	{
		Character->GetCharacterMovement()->StopMovementImmediately();
	}
	EndTask();
}
