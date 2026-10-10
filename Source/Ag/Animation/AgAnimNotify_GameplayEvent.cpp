// Copyright Woogle. All Rights Reserved.

#include "Animation/AgAnimNotify_GameplayEvent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"

void UAgAnimNotify_GameplayEvent::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (AActor* Owner = MeshComp->GetOwner(); Owner && EventTag.IsValid())
	{
		FGameplayEventData Payload;
		Payload.EventTag = EventTag;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, EventTag, Payload);
	}
}

FString UAgAnimNotify_GameplayEvent::GetNotifyName_Implementation() const
{
	return EventTag.IsValid() ? EventTag.ToString() : Super::GetNotifyName_Implementation();
}
