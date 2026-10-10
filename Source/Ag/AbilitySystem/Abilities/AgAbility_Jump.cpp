// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_Jump.h"

#include "Abilities/Tasks/AbilityTask_WaitMovementModeChange.h"
#include "Animation/AnimInstance.h"
#include "Character/AgPlayerCharacter.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"

UAgAbility_Jump::UAgAbility_Jump()
{
	SetupPlayerAction(AgGameplayTags::Ability_Action_Jump, /*bIgnoresMovement*/ false);

	// A sprint keeps going through the jump (플레이어 사양 '점프'), so the jump cancels every other action but not the sprint.
	CancelAbilitiesWithTag.RemoveTag(AgGameplayTags::Ability_Action);
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Action_BasicAttack);
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Action_Dodge);
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Action_DodgeCounter);
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Action_Guard);
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Action_Skill);
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Action_Ultimate);
	CancelAbilitiesWithTag.AddTag(AgGameplayTags::Ability_Action_Execution);
	ActivationOwnedTags.AddTag(AgGameplayTags::State_InAir);
}

void UAgAbility_Jump::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(GetAgCharacter());
	if (!Player || !Player->CanJump() || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 공중: only movement until landing.
	BeginBusy();
	Player->Jump();

	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance();
	if (Data && Data->JumpMontage && AnimInstance && !IsMoving())
	{
		AnimInstance->Montage_Play(Data->JumpMontage);
	}

	// The jump turns walking into falling; the next change back to walking is the landing.
	UAbilityTask_WaitMovementModeChange* LandTask = UAbilityTask_WaitMovementModeChange::CreateWaitMovementModeChange(this, MOVE_Walking);
	LandTask->OnChange.AddDynamic(this, &ThisClass::HandleLanded);
	LandTask->ReadyForActivation();
}

void UAgAbility_Jump::HandleLanded(EMovementMode NewMovementMode)
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance();
	if (Data && Data->LandMontage && AnimInstance && !IsMoving())
	{
		AnimInstance->Montage_Play(Data->LandMontage);
	}
	else if (Data && Data->JumpMontage && AnimInstance)
	{
		AnimInstance->Montage_Stop(0.1f, Data->JumpMontage);
	}
	EndSelf(false);
}

bool UAgAbility_Jump::IsMoving() const
{
	const AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(GetAgCharacter());
	return Player && !Player->GetMoveInputDirection().IsNearlyZero();
}

void UAgAbility_Jump::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(GetAgCharacter()))
	{
		Player->StopJumping();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
