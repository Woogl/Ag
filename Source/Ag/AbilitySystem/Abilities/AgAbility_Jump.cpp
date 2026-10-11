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

	// 달리기는 점프 중에도 이어지므로(플레이어 사양 '점프') 점프는 달리기를 취소하지 않습니다.
	CancelAbilitiesWithTag.RemoveTag(AgGameplayTags::Ability_Sprint);
}

void UAgAbility_Jump::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AAgPlayerCharacter* Player = CastChecked<AAgPlayerCharacter>(GetAgCharacter());
	if (!Player->CanJump() || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 공중: 착지할 때까지 이동만 합니다.
	BeginBusy();
	Player->Jump();

	// 시작 모션은 루트 모션이 없어 이동 입력을 막지 않으므로, 움직일 때도 재생합니다.
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance();
	if (Data->JumpMontage && AnimInstance)
	{
		AnimInstance->Montage_Play(Data->JumpMontage);
	}

	// 점프하면 걷기가 낙하로 바뀌므로, 다음에 걷기로 바뀌는 때가 착지입니다.
	UAbilityTask_WaitMovementModeChange* LandTask = UAbilityTask_WaitMovementModeChange::CreateWaitMovementModeChange(this, MOVE_Walking);
	LandTask->OnChange.AddDynamic(this, &ThisClass::HandleLanded);
	LandTask->ReadyForActivation();
}

void UAgAbility_Jump::HandleLanded(EMovementMode NewMovementMode)
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance();
	if (Data->LandMontage && AnimInstance && !IsMoving())
	{
		AnimInstance->Montage_Play(Data->LandMontage);
	}
	else if (Data->JumpMontage && AnimInstance)
	{
		AnimInstance->Montage_StopWithBlendOut(Data->JumpMontage->GetBlendOutArgs(), Data->JumpMontage);
	}
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

bool UAgAbility_Jump::IsMoving() const
{
	const AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(GetAgCharacter());
	return Player && !Player->GetMoveInputDirection().IsNearlyZero();
}

void UAgAbility_Jump::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	CastChecked<AAgPlayerCharacter>(GetAgCharacter())->StopJumping();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
