// Copyright Woogle. All Rights Reserved.

#include "Combat/AgSwordWave.h"

#include "Character/AgCharacterBase.h"
#include "Combat/AgCombatLibrary.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
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

void AAgSwordWave::Launch(AAgCharacterBase* InShooter, const FAgAttackHit& InHit, const FVector& Direction, float InSpeed, float InRange, const FVector& Size)
{
	Shooter = InShooter;
	Hit = InHit;
	FlightDirection = Direction.GetSafeNormal2D();
	Speed = InSpeed;
	Range = InRange;
	Traveled = 0.f;

	// The box stands on the ground, facing its flight.
	const FVector Extent = Size * 0.5f;
	HitBox->SetBoxExtent(Extent);
	SetActorLocationAndRotation(GetActorLocation() + FVector(0.f, 0.f, Extent.Z), FlightDirection.Rotation());

	// The mesh is stretched over the box, whatever its own size and pivot.
	const UStaticMesh* StaticMesh = Mesh->GetStaticMesh();
	const FBox Bounds = StaticMesh ? StaticMesh->GetBoundingBox() : FBox(ForceInit);
	const FVector MeshSize = Bounds.GetSize();
	if (MeshSize.X > 0.f && MeshSize.Y > 0.f && MeshSize.Z > 0.f)
	{
		const FVector Scale = Size / MeshSize;
		Mesh->SetRelativeScale3D(Scale);
		Mesh->SetRelativeLocation(-Bounds.GetCenter() * Scale);
	}
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
	const FVector End = Start + FlightDirection * Step;
	FHitResult WallHit;
	if (GetWorld()->LineTraceSingleByObjectType(WallHit, Start, End, FCollisionObjectQueryParams(ECC_WorldStatic), QueryParams))
	{
		Destroy();
		return;
	}
	SetActorLocation(End);
	Traveled += Step;

	// A character inside the box takes the hit. Invincibility lets the wave pass through and it keeps flying.
	TArray<FOverlapResult> Overlaps;
	const FCollisionShape Box = FCollisionShape::MakeBox(HitBox->GetScaledBoxExtent());
	GetWorld()->OverlapMultiByObjectType(Overlaps, End, GetActorQuat(), FCollisionObjectQueryParams(ECC_Pawn), Box, QueryParams);
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AAgCharacterBase* Target = Cast<AAgCharacterBase>(Overlap.GetActor());
		if (!Target || !UAgCombatLibrary::AreHostile(ShooterCharacter, Target))
		{
			continue;
		}
		const FVector HitLocation = Target->GetActorLocation() - FlightDirection * Target->GetCapsuleComponent()->GetScaledCapsuleRadius();
		const EAgHitResult Result = UAgCombatLibrary::ProcessHit(ShooterCharacter, Target, Hit, HitLocation, this);
		if (Result == EAgHitResult::Hit || Result == EAgHitResult::Guard || Result == EAgHitResult::GuardBreak || Result == EAgHitResult::Parry)
		{
			Destroy();
			return;
		}
	}

	if (Traveled >= Range)
	{
		Destroy();
	}
}
