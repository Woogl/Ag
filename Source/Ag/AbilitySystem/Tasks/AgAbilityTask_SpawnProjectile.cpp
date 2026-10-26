// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_SpawnProjectile.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystem/AgGameplayEffectContext.h"
#include "Character/AgCharacterBase.h"
#include "Combat/AgProjectile.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/AgAssetManager.h"
#include "Data/AgProjectileData.h"
#include "Ag.h"

UAbilityTask* FAgSpawnProjectileParams::MakeTask(const FAgTaskContext& Context) const
{
	UAgAbilityTask_SpawnProjectile* Task = UAbilityTask::NewAbilityTask<UAgAbilityTask_SpawnProjectile>(&Context.Ability);
	Task->Params = *this;
	return Task;
}

void UAgAbilityTask_SpawnProjectile::Activate()
{
	AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor());
	if (!Character)
	{
		EndTask();
		return;
	}
	UAgGameplayAbility* AgAbility = CastChecked<UAgGameplayAbility>(Ability);
	const FAgProjectileDefinition* Definition = UAgAssetManager::Get().GetProjectiles().Find(Params.Projectile);
	if (!Definition || !Definition->ProjectileClass)
	{
		UE_LOG(LogAg, Error, TEXT("SpawnProjectile (ability %s): projectile %s missing in Projectiles"), *AgAbility->GetAbilityId().ToString(),
			*Params.Projectile.ToString());
		EndTask();
		return;
	}
	const USkeletalMeshComponent* Mesh = Character->GetMesh();
	if (!Params.SocketName.IsNone() && !Mesh->DoesSocketExist(Params.SocketName))
	{
		UE_LOG(LogAg, Error, TEXT("SpawnProjectile: %s has no socket %s"), *GetNameSafe(Mesh->GetSkeletalMeshAsset()), *Params.SocketName.ToString());
		EndTask();
		return;
	}
	const FVector Direction = Character->GetActorForwardVector().GetSafeNormal2D();
	const FVector Location = Params.SocketName.IsNone() ? Character->GetActorLocation() : Mesh->GetSocketLocation(Params.SocketName);
	const FTransform Transform(Direction.Rotation(), Location);
	if (AAgProjectile* Projectile = GetWorld()->SpawnActorDeferred<AAgProjectile>(Definition->ProjectileClass, Transform, Character, Character, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
	{
		Projectile->Launch(Definition->Damage, AgAbility->MakeDamageSpec(Definition->Damage, FAgHitRewardState::Make(Params.HitReward)), Direction,
			Definition->Params);
		Projectile->FinishSpawning(Transform);
	}
	EndTask();
}
