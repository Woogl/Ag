// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_SpawnProjectile.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Character/AgCharacterBase.h"
#include "Combat/AgProjectile.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/AgAssetManager.h"
#include "Data/AgEffectData.h"
#include "Ag.h"

UAbilityTask* FAgSpawnProjectileParams::MakeTask(const FAgTaskContext& Context) const
{
	const FAgProjectileDefinition* Projectile = UAgAssetManager::Get().GetProjectiles().Find(ProjectileId);
	if (!Projectile)
	{
		UE_LOG(LogAg, Error, TEXT("Abilities.%s: projectile %s missing in Projectiles"), *Context.Ability.GetAbilityId().ToString(),
			*ProjectileId.ToString());
		return nullptr;
	}
	return UAgAbilityTask_SpawnProjectile::SpawnProjectile(&Context.Ability, *Projectile, SocketName);
}

UAgAbilityTask_SpawnProjectile* UAgAbilityTask_SpawnProjectile::SpawnProjectile(UGameplayAbility* Owner, const FAgProjectileDefinition& Projectile, FName SocketName)
{
	UAgAbilityTask_SpawnProjectile* Task = NewAbilityTask<UAgAbilityTask_SpawnProjectile>(Owner);
	Task->ProjectileClass = Projectile.ProjectileClass;
	Task->FlightParams = Projectile.Params;
	Task->Damage = Projectile.Damage;
	Task->SpawnSocket = SocketName;
	return Task;
}

void UAgAbilityTask_SpawnProjectile::Activate()
{
	AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor());
	if (!Character || !ProjectileClass || !Ability || !Ability->IsActive())
	{
		EndTask();
		return;
	}
	const USkeletalMeshComponent* Mesh = Character->GetMesh();
	if (!SpawnSocket.IsNone() && !Mesh->DoesSocketExist(SpawnSocket))
	{
		UE_LOG(LogAg, Error, TEXT("SpawnProjectile: %s has no socket %s"), *GetNameSafe(Mesh->GetSkeletalMeshAsset()), *SpawnSocket.ToString());
		EndTask();
		return;
	}
	Direction = Character->GetActorForwardVector().GetSafeNormal2D();
	const FVector Location = SpawnSocket.IsNone() ? Character->GetActorLocation() : Mesh->GetSocketLocation(SpawnSocket);
	const FTransform Transform(Direction.Rotation(), Location);
	FGameplayAbilityTargetData_LocationInfo* Target = new FGameplayAbilityTargetData_LocationInfo();
	Target->TargetLocation.LocationType = EGameplayAbilityTargetingLocationType::LiteralTransform;
	Target->TargetLocation.LiteralTransform = Transform;
	CachedTargetDataHandle.Add(Target);
	Success.AddDynamic(this, &ThisClass::HandleSpawned);
	AActor* Spawned = nullptr;
	if (!BeginSpawningActor(Ability, CachedTargetDataHandle, ProjectileClass, Spawned))
	{
		EndTask();
		return;
	}
	PendingProjectile = Cast<AAgProjectile>(Spawned);
	if (!IsValid(PendingProjectile) || !Ability->IsActive() || IsFinished())
	{
		if (IsValid(Spawned))
		{
			Spawned->Destroy();
		}
		PendingProjectile = nullptr;
		EndTask();
		return;
	}
	PendingProjectile->SetOwner(Character);
	PendingProjectile->SetInstigator(Character);
	FinishSpawningActor(Ability, CachedTargetDataHandle, PendingProjectile);
}

void UAgAbilityTask_SpawnProjectile::OnDestroy(bool bInOwnerFinished)
{
	if (IsValid(PendingProjectile))
	{
		PendingProjectile->Destroy();
	}
	PendingProjectile = nullptr;
	Super::OnDestroy(bInOwnerFinished);
}

void UAgAbilityTask_SpawnProjectile::HandleSpawned(AActor* Actor)
{
	if (!IsValid(Actor) || Actor != PendingProjectile || IsFinished() || !Ability->IsActive())
	{
		return;
	}
	const FGameplayEffectSpecHandle DamageSpec = CastChecked<UAgGameplayAbility>(Ability)->MakeEffectSpec(EAgEffect::Damage);
	if (DamageSpec.IsValid())
	{
		DamageSpec.Data->GetContext().AddOrigin(PendingProjectile->GetActorLocation());
		Damage.AddToSpec(*DamageSpec.Data);
	}
	PendingProjectile->Launch(DamageSpec, Direction, FlightParams);
	PendingProjectile = nullptr;
}
