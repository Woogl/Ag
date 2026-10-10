// Copyright Woogle. All Rights Reserved.

#include "Character/AgCharacterBase.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Game/AgBossStageGameMode.h"

namespace
{
	/** Engine collision profile for simulating skeletal meshes (BaseEngine.ini). */
	const FName RagdollProfileName(TEXT("Ragdoll"));
}

AAgCharacterBase::AAgCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AAgCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	if (AAgBossStageGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgBossStageGameMode>())
	{
		GameMode->RegisterCombatant(this);
	}
}

void AAgCharacterBase::Die(const FVector& HitDirection)
{
	if (bDead)
	{
		return;
	}
	bDead = true;

	// Stop input (player) and movement.
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		DisableInput(PlayerController);
	}
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->StopMovementImmediately();
	Movement->DisableMovement();

	// Don't block the other character any more.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	// Ragdoll, pushed the way the last attack came from.
	USkeletalMeshComponent* MeshComponent = GetMesh();
	MeshComponent->SetCollisionProfileName(RagdollProfileName);
	MeshComponent->SetSimulatePhysics(true);
	if (!HitDirection.IsNearlyZero())
	{
		MeshComponent->AddImpulse(HitDirection, NAME_None, /*bVelChange*/ true);
	}

	OnDied.Broadcast(this);
}

void AAgCharacterBase::FellOutOfWorld(const UDamageType& DamageType)
{
	Die(FVector::ZeroVector);
}
