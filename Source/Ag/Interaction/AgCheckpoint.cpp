// Copyright Woogle. All Rights Reserved.

#include "Interaction/AgCheckpoint.h"

#include "Character/AgPlayerCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/AgCollision.h"
#include "Engine/World.h"
#include "Game/AgGameMode.h"

AAgCheckpoint::AAgCheckpoint()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);

	PromptPoint = CreateDefaultSubobject<USceneComponent>(TEXT("PromptPoint"));
	PromptPoint->SetupAttachment(RootComponent);

	InteractionRange = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionRange"));
	InteractionRange->SetupAttachment(PromptPoint);
	InteractionRange->SetCollisionProfileName(AgCollision::InteractionRangeProfile);
}

FText AAgCheckpoint::GetInteractionText() const
{
	return InteractionText;
}

FVector AAgCheckpoint::GetInteractionLocation() const
{
	return PromptPoint->GetComponentLocation();
}

void AAgCheckpoint::Interact(AAgPlayerCharacter& Player)
{
	Player.Rest();
	if (AAgGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgGameMode>())
	{
		GameMode->RespawnEnemies();
	}
}
