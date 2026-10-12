// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_SpawnProjectile.h"

#include "AbilitySystemComponent.h"
#include "Combat/AgProjectile.h"
#include "Components/CapsuleComponent.h"
#include "Core/AgGameplayTags.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

UAgAbilityTask_SpawnProjectile* UAgAbilityTask_SpawnProjectile::SpawnProjectiles(UGameplayAbility* OwningAbility, TSubclassOf<AAgProjectile> InClass, TConstArrayView<FAgPlayerAttackHit> InHits, const FAgProjectileParams& InParams)
{
	UAgAbilityTask_SpawnProjectile* Task = NewAbilityTask<UAgAbilityTask_SpawnProjectile>(OwningAbility);
	Task->ProjectileClass = InClass;
	Task->Hits = InHits;
	Task->Params = InParams;
	return Task;
}

UAgAbilityTask_SpawnProjectile* UAgAbilityTask_SpawnProjectile::SpawnProjectiles(UGameplayAbility* OwningAbility, TSubclassOf<AAgProjectile> InClass, TConstArrayView<FAgAttackHit> InHits, const FAgProjectileParams& InParams)
{
	UAgAbilityTask_SpawnProjectile* Task = NewAbilityTask<UAgAbilityTask_SpawnProjectile>(OwningAbility);
	Task->ProjectileClass = InClass;
	Task->Hits.Reserve(InHits.Num());
	for (const FAgAttackHit& Hit : InHits)
	{
		Task->Hits.Emplace(Hit);
	}
	Task->Params = InParams;
	return Task;
}

void UAgAbilityTask_SpawnProjectile::Activate()
{
	Super::Activate();

	if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
	{
		ReleaseHandle = ASC->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_ProjectileRelease).AddUObject(this, &ThisClass::HandleRelease);
	}
}

void UAgAbilityTask_SpawnProjectile::HandleRelease(const FGameplayEventData* Payload)
{
	const int32 Index = NumReleased++;
	ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
	if (Character && ProjectileClass && Hits.IsValidIndex(Index))
	{
		// 캐릭터 앞의 바닥에서 정면으로 쏩니다.
		const FVector Direction = Character->GetActorForwardVector().GetSafeNormal2D();
		const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
		const FVector Location = Character->GetActorLocation() + Direction * Capsule->GetScaledCapsuleRadius() - FVector(0.f, 0.f, Capsule->GetScaledCapsuleHalfHeight());

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = Character;
		SpawnParameters.Instigator = Character;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AAgProjectile* Projectile = GetWorld()->SpawnActor<AAgProjectile>(ProjectileClass, Location, Direction.Rotation(), SpawnParameters))
		{
			Projectile->Launch(Hits[Index], Direction, Params);
		}
	}

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnFired.Broadcast(Index);
	}
}

void UAgAbilityTask_SpawnProjectile::OnDestroy(bool bInOwnerFinished)
{
	if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get(); ASC && ReleaseHandle.IsValid())
	{
		ASC->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_ProjectileRelease).Remove(ReleaseHandle);
	}

	Super::OnDestroy(bInOwnerFinished);
}
