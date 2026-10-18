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

	// 사망 처리: 쏜 캐릭터가 죽으면 투사체도 사라집니다.
	AAgCharacterBase* ShooterCharacter = GetInstigator<AAgCharacterBase>();
	if (!ShooterCharacter || ShooterCharacter->IsDead() || !DamageSpec.IsValid())
	{
		Destroy();
		return;
	}

	// 벽(보이지 않는 벽 포함)에 닿으면 벽까지만 날고 사라집니다. 선은 상자 가운데 높이로 지나가 바닥은 벽으로 치지 않습니다.
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

	// 상자가 닿은 캐릭터는 처음 닿을 때 한 번 맞고, 결과(피격, 가드, 패리, 무적)와 관계없이 투사체는 계속 날아갑니다.
	// 상자는 이번 틱에 움직인 거리 전체를 쓸어, 긴 프레임에도 캐릭터를 건너뛰지 않습니다.
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
	// SetContext는 출처 태그와 어트리뷰트를 다시 캡처하므로, 기존처럼 적중 시점의 ATK를 사용합니다.
	Spec.SetContext(Context);
	Context.GetInstigatorAbilitySystemComponent()->ApplyGameplayEffectSpecToTarget(Spec, Target.GetAbilitySystemComponent());
}
