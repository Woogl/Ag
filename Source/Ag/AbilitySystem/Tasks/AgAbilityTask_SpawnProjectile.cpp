// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_SpawnProjectile.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "Abilities/GameplayAbilityTargetTypes.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/AgCharacterBase.h"
#include "Combat/AgProjectile.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/AgEffectData.h"
#include "Ag.h"

UAgAbilityTask_SpawnProjectile* UAgAbilityTask_SpawnProjectile::SpawnProjectile(UGameplayAbility* Owner, int32 MontageInstanceId,
    const FAgProjectileDefinition& Projectile, const FAgAttackParams& Attack, EAgProjectileOrigin Origin, FName SocketName)
{
	UAgAbilityTask_SpawnProjectile* Task = NewAbilityTask<UAgAbilityTask_SpawnProjectile>(Owner);
	Task->SourceInstanceId = MontageInstanceId;
	Task->ProjectileClass = Projectile.ProjectileClass;
	Task->FlightParams = Projectile.Params;
	Task->Hit = Attack.MakeHit();
	Task->SpawnOrigin = Origin;
	Task->SpawnSocket = SocketName;
	return Task;
}

bool UAgAbilityTask_SpawnProjectile::IsSourceActive() const
{
	if (!Ability || !Ability->IsActive() || !AbilitySystemComponent.IsValid() || AbilitySystemComponent->GetAnimatingAbility() != Ability)
	{
		return false;
	}
	const AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor());
	UAnimInstance* Anim = Character ? Character->GetMesh()->GetAnimInstance() : nullptr;
	const FAnimMontageInstance* Instance = Anim ? Anim->GetMontageInstanceForID(SourceInstanceId) : nullptr;
	return Instance && Instance->IsActive() && Anim->GetActiveInstanceForMontage(Instance->Montage) == Instance &&
	       AbilitySystemComponent->GetCurrentMontage() == Instance->Montage;
}

void UAgAbilityTask_SpawnProjectile::Activate()
{
	if (!IsSourceActive())
	{
		EndTask();
		return;
	}
	AAgCharacterBase* Character = CastChecked<AAgCharacterBase>(GetAvatarActor());
	if (!ProjectileClass)
	{
		EndTask();
		return;
	}
	const UAnimMontage* Montage = AbilitySystemComponent->GetCurrentMontage();
	FTransform Transform;
	if (SpawnOrigin == EAgProjectileOrigin::CharacterFeetForward)
	{
		Direction = Character->GetActorForwardVector().GetSafeNormal2D();
		const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
		Transform = FTransform(Direction.Rotation(), Character->GetActorLocation() + Direction * Capsule->GetScaledCapsuleRadius() -
		                                                 FVector(0, 0, Capsule->GetScaledCapsuleHalfHeight()));
	}
	else if (SpawnOrigin == EAgProjectileOrigin::MeshSocket && !SpawnSocket.IsNone() && Character->GetMesh()->DoesSocketExist(SpawnSocket))
	{
		Transform = Character->GetMesh()->GetSocketTransform(SpawnSocket);
		Direction = Transform.GetRotation().GetForwardVector();
	}
	else
	{
		UE_LOG(LogAg, Error, TEXT("%s projectile %s has invalid socket/origin %s"), *GetNameSafe(Montage),
		    *GetNameSafe(ProjectileClass.Get()), *SpawnSocket.ToString());
		EndTask();
		return;
	}
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
	if (!IsValid(PendingProjectile) || !IsSourceActive() || IsFinished())
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

void UAgAbilityTask_SpawnProjectile::HandleSpawned(AActor* Actor)
{
	if (!IsValid(Actor) || Actor != PendingProjectile || IsFinished() || !IsSourceActive())
	{
		return;
	}
	// 바닥 기준 검기만 판정 상자 반높이만큼 올립니다. 소켓 발사는 소켓 변환을 보존합니다.
	if (SpawnOrigin == EAgProjectileOrigin::CharacterFeetForward)
	{
		PendingProjectile->AddActorWorldOffset(FVector(0.f, 0.f, PendingProjectile->GetCollisionHalfHeight()));
	}
	const FGameplayEffectSpecHandle DamageSpec = CastChecked<UAgGameplayAbility>(Ability)->MakeEffectSpec(EAgEffect::Damage);
	if (DamageSpec.IsValid())
	{
		DamageSpec.Data->GetContext().AddOrigin(PendingProjectile->GetActorLocation());
		Hit.AddToSpec(*DamageSpec.Data);
	}
	PendingProjectile->Launch(DamageSpec, Direction, FlightParams);
	PendingProjectile = nullptr;
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
