// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_JumpAttack.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitMovementModeChange.h"
#include "AbilitySystem/Tasks/AgAbilityTask_Attack.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Animation/AnimMontage.h"
#include "Character/AgCharacterBase.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	const FName LandSection(TEXT("Land"));
}

UAgAbility_JumpAttack::UAgAbility_JumpAttack()
{
	SetupPlayerAction(AgGameplayTags::Ability_Action_JumpAttack);

	// 공중에서는 점프가 행동 중이라, 행동 중 대신 점프 중인지로 시작할 수 있는지 정합니다(플레이어 입력 '상태별 허용 입력'의 공중).
	ActivationBlockedTags.RemoveTag(AgGameplayTags::State_Busy);
	ActivationRequiredTags.AddTag(AgGameplayTags::State_Jumping);
}

bool UAgAbility_JumpAttack::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	// 점프한 프레임에는 아직 땅에 있습니다. 그때 점프를 취소하면 점프 입력도 지워져 뛰지 않으므로, 떨어지는 중일 때만 시작합니다.
	const AAgCharacterBase* Character = ActorInfo ? Cast<AAgCharacterBase>(ActorInfo->AvatarActor.Get()) : nullptr;
	return Character && Character->GetCharacterMovement()->IsFalling() && Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UAgAbility_JumpAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	const FAgJumpAttackParams* Params = GetActionParams<FAgJumpAttackParams>();
	if (!Params || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	BeginBusy();
	UAgAbilityTask_Attack* AttackTask = UAgAbilityTask_Attack::PlayAttack(this, *Params, Data->JumpAttackMontage);
	AttackTask->OnRecoveryStarted.AddDynamic(this, &ThisClass::HandleRecoveryStarted);
	AttackTask->OnCompleted.AddDynamic(this, &ThisClass::HandleAttackCompleted);
	AttackTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleAttackInterrupted);
	AttackTask->ReadyForActivation();

	// 몽타주를 재생하지 못하면 OnInterrupted가 그 자리에서 어빌리티를 끝냅니다.
	if (!IsActive())
	{
		return;
	}

	// 평타의 각 타처럼, 점프 공격을 시작할 때도 그 대상인 보스가 Backstep을 검사합니다.
	FGameplayEventData Payload;
	Payload.EventTag = AgGameplayTags::Event_BasicAttackStarted;
	Payload.Instigator = GetAvatarActorFromActorInfo();
	Payload.Target = AttackTask->GetHitTarget();
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetAvatarActorFromActorInfo(), AgGameplayTags::Event_BasicAttackStarted, Payload);

	UAbilityTask_WaitGameplayEvent* PlungeTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Event_JumpAttackPlunge, nullptr, /*OnlyTriggerOnce*/ true);
	PlungeTask->EventReceived.AddDynamic(this, &ThisClass::HandlePlunge);
	PlungeTask->ReadyForActivation();

	UAbilityTask_WaitMovementModeChange* LandTask = UAbilityTask_WaitMovementModeChange::CreateWaitMovementModeChange(this, MOVE_Walking);
	LandTask->OnChange.AddDynamic(this, &ThisClass::HandleLanded);
	LandTask->ReadyForActivation();
}

void UAgAbility_JumpAttack::HandlePlunge(FGameplayEventData Payload)
{
	// 아래로는 정한 속도로 떨어지기 시작하고(그 뒤로는 엔진 중력이 더해짐), 수평 이동은 점프할 때의 이동을 그대로 잇습니다.
	AAgCharacterBase* Character = GetAgCharacter();
	const FAgJumpAttackParams* Params = GetActionParams<FAgJumpAttackParams>();
	if (Params && Character->GetCharacterMovement()->IsFalling())
	{
		Character->LaunchCharacter(FVector(0.f, 0.f, -Params->PlungeSpeed), /*bXYOverride*/ false, /*bZOverride*/ true);
	}
}

void UAgAbility_JumpAttack::HandleLanded(EMovementMode NewMovementMode)
{
	// 후딜은 착지 모션(Land 섹션)의 노티파이에서 시작합니다.
	MontageJumpToSection(LandSection);
}

void UAgAbility_JumpAttack::HandleRecoveryStarted()
{
	EndBusy();
}

void UAgAbility_JumpAttack::HandleAttackCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UAgAbility_JumpAttack::HandleAttackInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
