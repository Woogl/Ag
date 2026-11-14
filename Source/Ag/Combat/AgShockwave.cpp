// Copyright Woogle. All Rights Reserved.

#include "Combat/AgShockwave.h"

#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystem/AgGameplayEffectContext.h"
#include "Character/AgCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"

namespace AgShockwavePrivate
{
	/** BP 외형 고리의 반경과 높이입니다. */
	constexpr float VisualSize = 100.f;
} // namespace AgShockwavePrivate

AAgShockwave::AAgShockwave()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AAgShockwave::OnSpawnedByAbility(const UAgGameplayAbility& Ability)
{
	DamageSpec = Ability.MakeDamageSpec(Damage, MakeShared<FAgHitState>());
	DamageSpec.Data->GetContext().AddInstigator(GetInstigator(), this);
}

void AAgShockwave::BeginPlay()
{
	Super::BeginPlay();

	// 생성 위치는 발사자의 캡슐 중심이므로 발이 닿은 지면으로 내립니다.
	if (const AAgCharacterBase* Shooter = GetInstigator<AAgCharacterBase>())
	{
		AddActorWorldOffset(FVector(0.f, 0.f, -Shooter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
	}
	UpdateLook();
}

void AAgShockwave::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AAgCharacterBase* Shooter = GetInstigator<AAgCharacterBase>();
	if (!IsValid(Shooter) || !DamageSpec.IsValid())
	{
		Destroy();
		return;
	}

	// 고리 구간에 캡슐이 걸친 대상을 맞힙니다. 한 틱 이동이 너비보다 길어도 건너뛰는 곳이 없게 직전 반경까지 포함합니다. 높이는 지면 두께의 상자 질의가 가립니다.
	const float PreviousRadius = Radius;
	Radius = FMath::Min(Radius + Speed * DeltaSeconds, MaxRadius);
	const float InnerRadius = FMath::Min(PreviousRadius, Radius - Width);
	const FVector Center = GetActorLocation();
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AgShockwave), false, this);
	QueryParams.AddIgnoredActor(Shooter);
	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, Center + FVector(0.f, 0.f, Height * 0.5f), FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeBox(FVector(Radius, Radius, Height * 0.5f)), QueryParams);
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AAgCharacterBase* Target = Cast<AAgCharacterBase>(Overlap.GetActor());
		const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(Overlap.GetComponent());
		if (!IsValid(Target) || !Capsule || TouchedActors.Contains(Target) || !Target->CanBeHitBy(*Shooter))
		{
			continue;
		}
		const float Distance = FVector::Dist2D(Capsule->GetComponentLocation(), Center);
		const float CapsuleRadius = Capsule->GetScaledCapsuleRadius();
		if (Distance - CapsuleRadius > Radius || Distance + CapsuleRadius < InnerRadius)
		{
			continue;
		}
		TouchedActors.Add(Target);
		const FVector ToTarget = (Target->GetActorLocation() - Center).GetSafeNormal2D();
		const FVector HitLocation = Target->GetActorLocation() - ToTarget * Target->GetCapsuleComponent()->GetScaledCapsuleRadius();
		DamageSpec.Data->GetContext().AddOrigin(HitLocation);
		Shooter->GetAbilitySystemComponent()->ApplyGameplayEffectSpecToTarget(*DamageSpec.Data, Target->GetAbilitySystemComponent());
	}

	UpdateLook();
	if (Radius >= MaxRadius)
	{
		Destroy();
	}
}

void AAgShockwave::UpdateLook()
{
	SetActorScale3D(FVector(Radius / AgShockwavePrivate::VisualSize, Radius / AgShockwavePrivate::VisualSize, Height / AgShockwavePrivate::VisualSize));
	if (UStaticMeshComponent* Mesh = FindComponentByClass<UStaticMeshComponent>())
	{
		Mesh->SetCustomPrimitiveDataFloat(0, Radius > 0.f ? (Radius - Width) / Radius : 0.f);
	}
}
