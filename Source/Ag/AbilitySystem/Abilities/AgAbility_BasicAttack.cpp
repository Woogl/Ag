// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_BasicAttack.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Ag.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"

UAgAbility_BasicAttack::UAgAbility_BasicAttack()
{
	SetupPlayerAction(AgGameplayTags::Ability_Action_BasicAttack);
}

void UAgAbility_BasicAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (!IsActive())
	{
		return;
	}

	UAbilityTask_WaitGameplayEvent* InputTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Input_Attack);
	InputTask->EventReceived.AddDynamic(this, &ThisClass::HandleAttackInput);
	InputTask->ReadyForActivation();

	StartHit(0);
}

void UAgAbility_BasicAttack::StartHit(int32 Index)
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	if (!Data || !Data->BasicAttack.IsValidIndex(Index) || !Data->BasicAttackMontages.IsValidIndex(Index))
	{
		EndSelf(true);
		return;
	}

	HitIndex = Index;
	UE_LOG(LogAg, Verbose, TEXT("Basic attack: hit %d starts"), Index + 1);
	StartAttack(Data->BasicAttack[Index], MakeArrayView(&Data->BasicAttackMontages[Index], 1));

	// The boss checks its 백스텝 (B1) every time a basic attack hit starts.
	FGameplayEventData Payload;
	Payload.EventTag = AgGameplayTags::Event_BasicAttackStarted;
	Payload.Instigator = GetAvatarActorFromActorInfo();
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetAvatarActorFromActorInfo(), AgGameplayTags::Event_BasicAttackStarted, Payload);
}

void UAgAbility_BasicAttack::HandleAttackInput(FGameplayEventData Payload)
{
	// 평타 키의 처리 순서: during a hit's recovery the next hit starts; before it, the input is ignored.
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	if (IsInRecovery() && Data && Data->BasicAttack.Num() > 0)
	{
		StartHit((HitIndex + 1) % Data->BasicAttack.Num());
	}
}
