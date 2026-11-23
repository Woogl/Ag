// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_UseItem.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "Ag.h"
#include "GameFramework/PlayerController.h"
#include "Item/AgInventoryComponent.h"

UAbilityTask* FAgUseItemParams::MakeTask(const FAgTaskContext& Context) const
{
	return UAbilityTask::NewAbilityTask<UAgAbilityTask_UseItem>(&Context.Ability);
}

void UAgAbilityTask_UseItem::Activate()
{
	const FName Item = CastChecked<UAgGameplayAbility>(Ability)->GetDefinition().Item;
	const APlayerController* PlayerController = Ability->GetCurrentActorInfo()->PlayerController.Get();
	UAgInventoryComponent* Inventory = PlayerController ? PlayerController->FindComponentByClass<UAgInventoryComponent>() : nullptr;
	if (!Inventory || !Inventory->RemoveItem(Item, 1))
	{
		UE_LOG(LogAg, Warning, TEXT("UseItem: no %s to use"), *Item.ToString());
	}
	EndTask();
}
