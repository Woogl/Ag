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
#include "NiagaraComponent.h"

namespace AgShockwavePrivate
{
	/** BP 외형 고리의 반경과 높이입니다. */
	constexpr float VisualSize = 100.f;

	constexpr const TCHAR* RadiusParameter = TEXT("User.Radius");
	constexpr const TCHAR* DurationParameter = TEXT("User.Duration");
	constexpr const TCHAR* IntervalParameter = TEXT("User.Interval");
} // namespace AgShockwavePrivate

AAgShockwave::AAgShockwave()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AAgShockwave::OnSpawnedByAbility(const UAgGameplayAbility& Ability)
{
	const TSharedPtr<FAgHitState> HitState = MakeShared<FAgHitState>();
	for (const FAgDamage& Hit : Hits)
	{
		const FGameplayEffectSpecHandle Spec = Ability.MakeDamageSpec(Hit, HitState);
		Spec.Data->GetContext().AddInstigator(GetInstigator(), this);
		HitSpecs.Add(Spec);
	}
}

void AAgShockwave::BeginPlay()
{
	Super::BeginPlay();

	// 생성 위치가 캡슐 중심이든 소켓이든, 고리는 발사자의 발이 닿은 지면에서 퍼집니다.
	if (const AAgCharacterBase* Shooter = GetInstigator<AAgCharacterBase>())
	{
		FVector Location = GetActorLocation();
		Location.Z = Shooter->GetActorLocation().Z - Shooter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		SetActorLocation(Location);
	}
	UpdateLook();

	const float WaveInterval = (Speed > 0.f ? MaxRadius / Speed : 0.f) + Interval;
	TInlineComponentArray<UNiagaraComponent*> Effects(this);
	for (UNiagaraComponent* Effect : Effects)
	{
		Effect->SetVariableFloat(AgShockwavePrivate::RadiusParameter, MaxRadius);
		Effect->SetVariableFloat(AgShockwavePrivate::DurationParameter, FMath::Max(Hits.Num() - 1, 0) * WaveInterval);
		Effect->SetVariableFloat(AgShockwavePrivate::IntervalParameter, WaveInterval);
	}
}

void AAgShockwave::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AAgCharacterBase* Shooter = GetInstigator<AAgCharacterBase>();
	if (!IsValid(Shooter) || !HitSpecs.IsValidIndex(Wave))
	{
		Destroy();
		return;
	}
	if (WaitTime > 0.f)
	{
		WaitTime -= DeltaSeconds;
		if (WaitTime > 0.f)
		{
			return;
		}
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
	const FGameplayEffectSpecHandle& Spec = HitSpecs[Wave];
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
		Spec.Data->GetContext().AddOrigin(HitLocation);
		Shooter->GetAbilitySystemComponent()->ApplyGameplayEffectSpecToTarget(*Spec.Data, Target->GetAbilitySystemComponent());
	}

	UpdateLook();
	if (Radius < MaxRadius)
	{
		return;
	}
	if (++Wave < HitSpecs.Num())
	{
		Radius = 0.f;
		TouchedActors.Reset();
		WaitTime = Interval;
		return;
	}
	SetActorTickEnabled(false);
	DestroyWhenEffectsFinish();
}

void AAgShockwave::UpdateLook()
{
	if (UStaticMeshComponent* Mesh = FindComponentByClass<UStaticMeshComponent>())
	{
		SetActorScale3D(FVector(Radius / AgShockwavePrivate::VisualSize, Radius / AgShockwavePrivate::VisualSize, Height / AgShockwavePrivate::VisualSize));
		Mesh->SetCustomPrimitiveDataFloat(0, Radius > 0.f ? (Radius - Width) / Radius : 0.f);
	}
}

void AAgShockwave::DestroyWhenEffectsFinish()
{
	// 마지막 파동과 함께 지우면 그때 터지는 나이아가라도 함께 사라지므로, 켜져 있는 나이아가라가 모두 끝난 뒤 지웁니다.
	bool bPlaying = false;
	TInlineComponentArray<UNiagaraComponent*> Effects(this);
	for (UNiagaraComponent* Effect : Effects)
	{
		if (Effect->IsActive())
		{
			Effect->OnSystemFinished.AddUniqueDynamic(this, &ThisClass::HandleEffectFinished);
			bPlaying = true;
		}
	}
	if (!bPlaying)
	{
		Destroy();
	}
}

void AAgShockwave::HandleEffectFinished(UNiagaraComponent* Effect)
{
	DestroyWhenEffectsFinish();
}
