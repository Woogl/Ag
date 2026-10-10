// Copyright Woogle. All Rights Reserved.

#include "Combat/AgSwordWave.h"

#include "Character/AgCharacterBase.h"
#include "Combat/AgCombatLibrary.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"

AAgSwordWave::AAgSwordWave()
{
	PrimaryActorTick.bCanEverTick = true;

	// The box is queried by hand every tick, so it needs no collision of its own.
	HitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("HitBox"));
	HitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitBox->SetGenerateOverlapEvents(false);
	RootComponent = HitBox;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(HitBox);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCastShadow(false);
}

void AAgSwordWave::Launch(AAgCharacterBase* InShooter, const FAgAttackHit& InHit, const FVector& Direction, float InSpeed, float InRange)
{
	Shooter = InShooter;
	Hit = InHit;
	FlightDirection = Direction.GetSafeNormal2D();
	Speed = InSpeed;
	Range = InRange;
	Traveled = 0.f;

	// The box stands on the ground, facing its flight.
	SetActorLocationAndRotation(GetActorLocation() + FVector(0.f, 0.f, HitBox->GetScaledBoxExtent().Z), FlightDirection.Rotation());
}

void AAgSwordWave::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 사망 처리: the shooter's projectiles vanish with it.
	AAgCharacterBase* ShooterCharacter = Shooter.Get();
	if (!ShooterCharacter || ShooterCharacter->IsDead())
	{
		Destroy();
		return;
	}

	// A wall stops it, invisible walls too. The line runs through the box middle, so the floor never counts.
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AgSwordWave), false, this);
	QueryParams.AddIgnoredActor(ShooterCharacter);
	const FVector Start = GetActorLocation();
	const float Step = Speed * DeltaSeconds;
	FVector End = Start + FlightDirection * Step;
	FHitResult WallHit;
	const bool bHitWall = GetWorld()->LineTraceSingleByObjectType(WallHit, Start, End, FCollisionObjectQueryParams(ECC_WorldStatic), QueryParams);
	if (bHitWall)
	{
		End = WallHit.Location;
	}

	// A character the box reaches is judged once, when the wave first reaches it, and the wave flies on through whatever
	// the result (hit, guard, parry, or invincible). The box sweeps the whole way it moves this tick, so a long frame
	// can't skip a character.
	TArray<FHitResult> Reached;
	const FCollisionShape Box = FCollisionShape::MakeBox(HitBox->GetScaledBoxExtent());
	GetWorld()->SweepMultiByObjectType(Reached, Start, End, GetActorQuat(), FCollisionObjectQueryParams(ECC_Pawn), Box, QueryParams);
	for (const FHitResult& Contact : Reached)
	{
		AAgCharacterBase* Target = Cast<AAgCharacterBase>(Contact.GetActor());
		if (!Target || !UAgCombatLibrary::AreHostile(ShooterCharacter, Target) || TouchedActors.Contains(Target))
		{
			continue;
		}
		TouchedActors.Add(Target);
		const FVector HitLocation = Target->GetActorLocation() - FlightDirection * Target->GetCapsuleComponent()->GetScaledCapsuleRadius();
		UAgCombatLibrary::ProcessHit(ShooterCharacter, Target, Hit, HitLocation, this);
	}

	if (bHitWall)
	{
		Destroy();
		return;
	}
	SetActorLocation(End);
	Traveled += Step;
	if (Traveled >= Range)
	{
		Destroy();
	}
}
