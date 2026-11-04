// Copyright Woogle. All Rights Reserved.

#include "Combat/AgShockwave.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystem/AgGameplayEffectContext.h"
#include "Character/AgCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"

namespace
{
	/** BP 외형 고리의 반경과 높이입니다. */
	constexpr float VisualSize = 100.f;
} // namespace

AAgShockwave::AAgShockwave()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AAgShockwave::OnSpawnedByAbility(const UAgGameplayAbility& Ability)
{
	DamageSpec = Ability.MakeDamageSpec(Damage, MakeShared<FAgHitState>());
}

void AAgShockwave::BeginPlay()
{
	Super::BeginPlay();

	// 생성 위치는 발사자의 캡슐 중심이므로 발이 닿은 지면으로 내립니다.
	if (const AAgCharacterBase* Shooter = GetInstigator<AAgCharacterBase>())
	{
		AddActorWorldOffset(FVector(0.f, 0.f, -Shooter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
	}
	UpdateScale();
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

	// 이번 틱에 가장자리가 지나간 반경 구간에 캡슐이 걸친 대상을 맞힙니다. 높이는 지면 두께의 상자 질의가 가립니다.
	const float InnerRadius = Radius;
	Radius = FMath::Min(Radius + Speed * DeltaSeconds, MaxRadius);
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
		const FVector ToCapsule = (Capsule->GetComponentLocation() - Center) * FVector(1.f, 1.f, 0.f);
		const float Distance = ToCapsule.Size();
		const float CapsuleRadius = Capsule->GetScaledCapsuleRadius();
		if (Distance - CapsuleRadius > Radius || Distance + CapsuleRadius < InnerRadius)
		{
			continue;
		}
		TouchedActors.Add(Target);
		const FVector ToTarget = ((Target->GetActorLocation() - Center) * FVector(1.f, 1.f, 0.f)).GetSafeNormal();
		ApplyHit(*Target, Target->GetActorLocation() - ToTarget * Target->GetCapsuleComponent()->GetScaledCapsuleRadius());
	}

	UpdateScale();
	if (Radius >= MaxRadius)
	{
		Destroy();
	}
}

void AAgShockwave::ApplyHit(AAgCharacterBase& Target, const FVector& HitLocation)
{
	FGameplayEffectSpec Spec(*DamageSpec.Data);
	FGameplayEffectContextHandle Context = Spec.GetContext().Duplicate();
	Context.AddInstigator(GetInstigator(), this);
	Context.AddOrigin(HitLocation);
	// SetContext는 출처 태그와 어트리뷰트를 다시 캡처하므로 적중 시점의 ATK를 씁니다.
	Spec.SetContext(Context);
	if (UAbilitySystemComponent* InstigatorASC = Context.GetInstigatorAbilitySystemComponent())
	{
		InstigatorASC->ApplyGameplayEffectSpecToTarget(Spec, Target.GetAbilitySystemComponent());
	}
}

void AAgShockwave::UpdateScale()
{
	SetActorScale3D(FVector(Radius / VisualSize, Radius / VisualSize, Height / VisualSize));
}
