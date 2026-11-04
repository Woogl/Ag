// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_SpawnActor.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "Character/AgCharacterBase.h"
#include "Combat/AgAbilitySpawnedActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/AgAssetManager.h"
#include "Engine/World.h"
#include "Ag.h"

UAbilityTask* FAgSpawnActorParams::MakeTask(const FAgTaskContext& Context) const
{
	UAgAbilityTask_SpawnActor* Task = UAbilityTask::NewAbilityTask<UAgAbilityTask_SpawnActor>(&Context.Ability);
	Task->Params = *this;
	return Task;
}

void UAgAbilityTask_SpawnActor::Activate()
{
	AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor());
	if (!Character)
	{
		EndTask();
		return;
	}
	UAgGameplayAbility* AgAbility = CastChecked<UAgGameplayAbility>(Ability);
	AActor* Template = UAgAssetManager::Get().FindSpawnActor(Params.Actor);
	if (!Template)
	{
		UE_LOG(LogAg, Error, TEXT("SpawnActor (ability %s): actor %s missing in SpawnActors"), *AgAbility->GetAbilityId().ToString(),
			*Params.Actor.ToString());
		EndTask();
		return;
	}
	const USkeletalMeshComponent* Mesh = Character->GetMesh();
	if (!Params.SocketName.IsNone() && !Mesh->DoesSocketExist(Params.SocketName))
	{
		UE_LOG(LogAg, Error, TEXT("SpawnActor: %s has no socket %s"), *GetNameSafe(Mesh->GetSkeletalMeshAsset()), *Params.SocketName.ToString());
		EndTask();
		return;
	}
	const FVector Location = Params.SocketName.IsNone() ? Character->GetActorLocation() : Mesh->GetSocketLocation(Params.SocketName);
	const FTransform Transform(Character->GetActorForwardVector().GetSafeNormal2D().Rotation(), Location);

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Template = Template;
	SpawnParameters.Owner = Character;
	SpawnParameters.Instigator = Character;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParameters.bDeferConstruction = true;
	if (AActor* Spawned = GetWorld()->SpawnActor(Template->GetClass(), &Transform, SpawnParameters))
	{
		if (IAgAbilitySpawnedActor* AbilityActor = Cast<IAgAbilitySpawnedActor>(Spawned))
		{
			AbilityActor->OnSpawnedByAbility(*AgAbility);
		}
		Spawned->FinishSpawning(Transform);
	}
	EndTask();
}
