// Copyright Woogle. All Rights Reserved.

#include "Animation/AgAnimNotify_SpawnProjectile.h"
#include "Animation/AgAnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#if WITH_EDITOR
bool UAgAnimNotify_SpawnProjectile::CanBePlaced(UAnimSequenceBase* Animation) const
{
	return Animation && Animation->IsA<UAnimMontage>();
}

#endif
void UAgAnimNotify_SpawnProjectile::Notify(
    USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& Reference)
{
	Super::Notify(Mesh, Animation, Reference);
	if (UAgAnimInstance* Anim = Mesh ? Cast<UAgAnimInstance>(Mesh->GetAnimInstance()) : nullptr)
	{
		Anim->BroadcastMontageNotify(Animation, Reference);
	}
}
