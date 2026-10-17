// Copyright Woogle. All Rights Reserved.
#include "Animation/AgAnimNotify_GameplayEvent.h"
#include "Animation/AgAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
void UAgAnimNotify_GameplayEvent::Notify(
    USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& Reference)
{
	if (UAgAnimInstance* Anim = Mesh ? Cast<UAgAnimInstance>(Mesh->GetAnimInstance()) : nullptr)
	{
		Anim->BroadcastMontageNotify(Animation, Reference);
	}
}

FString UAgAnimNotify_GameplayEvent::GetNotifyName_Implementation() const
{
	return EventTag.ToString();
}
