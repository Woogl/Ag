// Copyright Woogle. All Rights Reserved.

#include "Combat/AgProjectile.h"

#include "AbilitySystemComponent.h"
#include "Character/AgCharacterBase.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

AAgProjectile::AAgProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	// 상자는 매 틱 코드에서 직접 검사하므로 자체 충돌이 필요 없습니다.
	HitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("HitBox"));
	HitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitBox->SetGenerateOverlapEvents(false);
	RootComponent = HitBox;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(HitBox);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetGenerateOverlapEvents(false);
}

void AAgProjectile::Launch(const FGameplayEffectSpecHandle& InDamageSpec, const FVector& Direction, const FAgProjectileParams& InParams)
{
	DamageSpec = InDamageSpec;
	Params = InParams;
	FlightDirection = Direction.GetSafeNormal();
	Traveled = 0.f;
}

float AAgProjectile::GetCollisionHalfHeight() const
{
	return HitBox->GetScaledBoxExtent().Z;
}

void AAgProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AAgCharacterBase* ShooterCharacter = GetInstigator<AAgCharacterBase>();
	if (!ShooterCharacter || ShooterCharacter->IsDead() || !DamageSpec.IsValid())
	{
		Destroy();
		return;
	}

	// 상자 바닥은 지면에 닿아 있으므로, 벽은 상자 가운데 높이의 선으로 찾아 바닥을 벽으로 치지 않습니다.
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AgProjectile), false, this);
	QueryParams.AddIgnoredActor(ShooterCharacter);
	const FVector Start = GetActorLocation();
	const float Step = FMath::Min(Params.Speed * DeltaSeconds, Params.Range - Traveled);
	FVector End = Start + FlightDirection * Step;
	FHitResult WallHit;
	const bool bHitWall =
		GetWorld()->LineTraceSingleByObjectType(WallHit, Start, End, FCollisionObjectQueryParams(ECC_WorldStatic), QueryParams);
	if (bHitWall)
	{
		End = WallHit.Location;
	}

	TArray<FHitResult> Reached;
	const FCollisionShape Box = FCollisionShape::MakeBox(HitBox->GetScaledBoxExtent());
	GetWorld()->SweepMultiByObjectType(Reached, Start, End, GetActorQuat(), FCollisionObjectQueryParams(ECC_Pawn), Box, QueryParams);
	for (const FHitResult& Contact : Reached)
	{
		AAgCharacterBase* Target = Cast<AAgCharacterBase>(Contact.GetActor());
		if (!Target || FGenericTeamId::GetAttitude(ShooterCharacter, Target) != ETeamAttitude::Hostile || TouchedActors.Contains(Target))
		{
			continue;
		}
		TouchedActors.Add(Target);
		const FVector HitLocation = Target->GetActorLocation() - FlightDirection * Target->GetCapsuleComponent()->GetScaledCapsuleRadius();
		ApplyHit(*Target, HitLocation);
	}

	if (bHitWall)
	{
		Destroy();
		return;
	}
	SetActorLocation(End);
	Traveled += Step;
	if (Traveled >= Params.Range)
	{
		Destroy();
	}
}

void AAgProjectile::ApplyHit(AAgCharacterBase& Target, const FVector& HitLocation)
{
	FGameplayEffectSpec Spec(*DamageSpec.Data);
	FGameplayEffectContextHandle Context = Spec.GetContext().Duplicate();
	Context.AddInstigator(GetInstigator(), this);
	Context.AddOrigin(HitLocation);
	// SetContext는 출처 태그와 어트리뷰트를 다시 캡처하므로 적중 시점의 ATK를 씁니다.
	Spec.SetContext(Context);
	Context.GetInstigatorAbilitySystemComponent()->ApplyGameplayEffectSpecToTarget(Spec, Target.GetAbilitySystemComponent());
}
