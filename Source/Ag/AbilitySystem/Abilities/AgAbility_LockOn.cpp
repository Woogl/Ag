// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_LockOn.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/Tasks/AgAbilityTask_LockOn.h"
#include "Character/AgLockOnComponent.h"
#include "Character/AgPlayerCharacter.h"
#include "Core/AgGameplayTags.h"

UAgAbility_LockOn::UAgAbility_LockOn()
{
	SetAssetTags(FGameplayTagContainer(AgGameplayTags::Ability_LockOn));
}

void UAgAbility_LockOn::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(GetAvatarActorFromActorInfo());
	UAgLockOnComponent* LockOn = Player ? Player->GetLockOn() : nullptr;
	AAgCharacterBase* Target = LockOn ? LockOn->FindTarget() : nullptr;
	if (!Target || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	LockOn->SetTarget(Target);

	UAbilityTask_WaitGameplayEvent* InputTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Input_LockOn);
	InputTask->EventReceived.AddDynamic(this, &ThisClass::HandleLockOnInput);
	InputTask->ReadyForActivation();

	UAgAbilityTask_LockOn* FollowTask = UAgAbilityTask_LockOn::FollowTarget(this, LockOn);
	FollowTask->OnTargetLost.AddDynamic(this, &ThisClass::HandleTargetLost);
	FollowTask->ReadyForActivation();
}

void UAgAbility_LockOn::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	const AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(GetAvatarActorFromActorInfo());
	if (UAgLockOnComponent* LockOn = Player ? Player->GetLockOn() : nullptr)
	{
		LockOn->SetTarget(nullptr);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UAgAbility_LockOn::HandleLockOnInput(FGameplayEventData Payload)
{
	// 락온 해제 조건: 락온 키 재입력
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UAgAbility_LockOn::HandleTargetLost()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
