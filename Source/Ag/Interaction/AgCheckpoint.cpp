// Copyright Woogle. All Rights Reserved.

#include "Interaction/AgCheckpoint.h"

#include "Character/AgPlayerCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/AgCollision.h"
#include "Engine/World.h"
#include "Game/AgGameMode.h"
#include "Game/AgSaveSubsystem.h"

AAgCheckpoint::AAgCheckpoint()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);

	StartPoint = CreateDefaultSubobject<USceneComponent>(TEXT("StartPoint"));
	StartPoint->SetupAttachment(RootComponent);
	StartPoint->SetRelativeLocationAndRotation(FVector(150., 0., 0.), FRotator(0., 180., 0.));

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
	UAgSaveSubsystem::Get(this).SetRestedCheckpoint(*this);
}

FTransform AAgCheckpoint::GetStartTransform() const
{
	return FTransform(StartPoint->GetComponentRotation(), StartPoint->GetComponentLocation());
}
