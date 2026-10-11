// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_PlayReaction.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/Tasks/AgAbilityTask_HoldTag.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Core/AgGameplayTags.h"

UAgAbilityTask_PlayReaction* UAgAbilityTask_PlayReaction::PlayReaction(UGameplayAbility* OwningAbility, UAnimMontage* InMontage, UAnimMontage* InFollowUpMontage)
{
	UAgAbilityTask_PlayReaction* Task = NewAbilityTask<UAgAbilityTask_PlayReaction>(OwningAbility);
	Task->Montage = InMontage;
	Task->FollowUpMontage = InFollowUpMontage;
	return Task;
}

void UAgAbilityTask_PlayReaction::Activate()
{
	Super::Activate();

	// 넉백, 넉업: 바닥에 쓰러진 순간부터 기상이 끝날 때까지 무적
	if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
	{
		GroundedHandle = ASC->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_DownGrounded).AddUObject(this, &ThisClass::HandleGrounded);
	}

	bPlayingFollowUp = false;
	PlayMontage(Montage);
}

void UAgAbilityTask_PlayReaction::PlayMontage(UAnimMontage* InMontage, FName StartSection)
{
	// 바꾼 몽타주 태스크는 그 몽타주가 멈춰도 반응을 끝내지 않습니다.
	if (MontageTask)
	{
		MontageTask->EndTask();
	}
	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(Ability, NAME_None, InMontage, 1.f, StartSection);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::HandleMontageBlendingOut);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UAgAbilityTask_PlayReaction::HandleMontageBlendingOut()
{
	if (!bPlayingFollowUp && FollowUpMontage)
	{
		bPlayingFollowUp = true;
		PlayMontage(FollowUpMontage);
	}
}

void UAgAbilityTask_PlayReaction::HandleMontageCompleted()
{
	MontageTask = nullptr;
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnCompleted.Broadcast();
	}
	EndTask();
}

void UAgAbilityTask_PlayReaction::HandleMontageCancelled()
{
	MontageTask = nullptr;
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnInterrupted.Broadcast();
	}
	EndTask();
}

void UAgAbilityTask_PlayReaction::HandleGrounded(const FGameplayEventData* Payload)
{
	if (!InvincibleTask)
	{
		InvincibleTask = UAgAbilityTask_HoldTag::HoldTag(Ability, AgGameplayTags::State_Invincible);
		InvincibleTask->ReadyForActivation();
	}
}

void UAgAbilityTask_PlayReaction::OnDestroy(bool bInOwnerFinished)
{
	if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get(); ASC && GroundedHandle.IsValid())
	{
		ASC->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_DownGrounded).Remove(GroundedHandle);
	}

	// 어빌리티가 끝날 때는 어빌리티가 하위 태스크도 끝냅니다(몽타주를 멈추고 무적을 뗌).
	if (!bInOwnerFinished)
	{
		if (MontageTask)
		{
			MontageTask->EndTask();
		}
		if (InvincibleTask)
		{
			InvincibleTask->EndTask();
		}
	}
	MontageTask = nullptr;
	InvincibleTask = nullptr;

	Super::OnDestroy(bInOwnerFinished);
}
