// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_SetCollisionResponseToChannel.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Character.h"

UAbilityTask* FAgSetCollisionResponseToChannelParams::MakeTask(const FAgTaskContext& Context) const
{
	const ACharacter* Character = Cast<ACharacter>(Context.Runner.GetAvatarActorFromActorInfo());
	return UAgAbilityTask_SetCollisionResponseToChannel::SetCollisionResponseToChannel(
	    &Context.Runner, Character ? Character->GetCapsuleComponent() : nullptr, Channel, Response);
}

UAgAbilityTask_SetCollisionResponseToChannel* UAgAbilityTask_SetCollisionResponseToChannel::SetCollisionResponseToChannel(
    UGameplayAbility* OwningAbility, UPrimitiveComponent* InComponent, ECollisionChannel InChannel, ECollisionResponse InResponse)
{
	UAgAbilityTask_SetCollisionResponseToChannel* Task = NewAbilityTask<UAgAbilityTask_SetCollisionResponseToChannel>(OwningAbility);
	Task->Component = InComponent;
	Task->Channel = InChannel;
	Task->Response = InResponse;
	return Task;
}

void UAgAbilityTask_SetCollisionResponseToChannel::Activate()
{
	Super::Activate();

	UPrimitiveComponent* TargetComponent = Component.Get();
	if (!TargetComponent)
	{
		EndTask();
		return;
	}
	SavedResponse = TargetComponent->GetCollisionResponseToChannel(Channel);
	bResponseApplied = true;
	const bool bOwnerWasActive = Ability && Ability->IsActive();
	TargetComponent->SetCollisionResponseToChannel(Channel, Response);
	// 엔진의 활성 Task 목록 등록 전에도 충돌 콜백이 Ability를 취소할 수 있습니다.
	if (bOwnerWasActive && !IsFinished() && (!Ability || !Ability->IsActive()))
	{
		EndTask();
	}
}

void UAgAbilityTask_SetCollisionResponseToChannel::OnDestroy(bool bInOwnerFinished)
{
	const bool bRestoreResponse = bResponseApplied;
	bResponseApplied = false;
	if (UPrimitiveComponent* TargetComponent = Component.Get(); TargetComponent && bRestoreResponse)
	{
		TargetComponent->SetCollisionResponseToChannel(Channel, SavedResponse);
	}
	Super::OnDestroy(bInOwnerFinished);
}
