// Copyright Woogle. All Rights Reserved.

#include "Combat/AgShockwave.h"

#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystem/AgGameplayEffectContext.h"
#include "Character/AgCharacterBase.h"
#include "Components/BoxComponent.h"
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

	/**
	 * 닿은 컴포넌트가 고리 구간(InnerRadius ~ Radius)에 걸쳤는지 봅니다. 캐릭터 메시도 Pawn 오브젝트 종류라 몸 캡슐과 극한 회피
	 * 판정 상자만 보며, 상자는 반 변을 반경으로 한 원기둥으로 봅니다.
	 */
	bool IsOnRing(const UPrimitiveComponent& Component, const FVector& Center, float InnerRadius, float Radius)
	{
		float BodyRadius = 0.f;
		if (const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(&Component))
		{
			BodyRadius = Capsule->GetScaledCapsuleRadius();
		}
		else if (const UBoxComponent* Box = Cast<UBoxComponent>(&Component))
		{
			BodyRadius = Box->GetScaledBoxExtent().X;
		}
		else
		{
			return false;
		}
		const float Distance = FVector::Dist2D(Component.GetComponentLocation(), Center);
		return Distance - BodyRadius <= Radius && Distance + BodyRadius >= InnerRadius;
	}
} // namespace AgShockwavePrivate

AAgShockwave::AAgShockwave()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

const UScriptStruct& AAgShockwave::GetSpawnDataType() const
{
	return *FAgShockwaveData::StaticStruct();
}

void AAgShockwave::OnSpawnedByAbility(const UAgGameplayAbility& Ability, const FAgSpawnActorData& Data)
{
	SpawnData = static_cast<const FAgShockwaveData&>(Data);
	const TSharedPtr<FAgHitState> HitState = MakeShared<FAgHitState>();
	for (const FAgDamage& Hit : SpawnData.Hits)
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

	const float WaveInterval = (SpawnData.Speed > 0.f ? SpawnData.MaxRadius / SpawnData.Speed : 0.f) + SpawnData.Interval;
	TInlineComponentArray<UNiagaraComponent*> NiagaraComponents(this);
	for (UNiagaraComponent* Niagara : NiagaraComponents)
	{
		Niagara->SetVariableFloat(AgShockwavePrivate::RadiusParameter, SpawnData.MaxRadius);
		Niagara->SetVariableFloat(AgShockwavePrivate::DurationParameter, FMath::Max(SpawnData.Hits.Num() - 1, 0) * WaveInterval);
		Niagara->SetVariableFloat(AgShockwavePrivate::IntervalParameter, WaveInterval);
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
	Radius = FMath::Min(Radius + SpawnData.Speed * DeltaSeconds, SpawnData.MaxRadius);
	const float InnerRadius = FMath::Min(PreviousRadius, Radius - SpawnData.Width);
	const FVector Center = GetActorLocation();
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AgShockwave), false, this);
	QueryParams.AddIgnoredActor(Shooter);
	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, Center + FVector(0.f, 0.f, SpawnData.Height * 0.5f), FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeBox(FVector(Radius, Radius, SpawnData.Height * 0.5f)), QueryParams);
	const FGameplayEffectSpecHandle& Spec = HitSpecs[Wave];
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AAgCharacterBase* Target = Cast<AAgCharacterBase>(Overlap.GetActor());
		if (!IsValid(Target) || TouchedActors.Contains(Target) || !Target->CanBeHitBy(*Shooter) ||
			!AgShockwavePrivate::IsOnRing(*Overlap.GetComponent(), Center, InnerRadius, Radius))
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
	if (Radius < SpawnData.MaxRadius)
	{
		return;
	}
	if (++Wave < HitSpecs.Num())
	{
		Radius = 0.f;
		TouchedActors.Reset();
		WaitTime = SpawnData.Interval;
		return;
	}
	SetActorTickEnabled(false);
	DestroyWhenVFXFinish();
}

void AAgShockwave::UpdateLook()
{
	if (UStaticMeshComponent* Mesh = FindComponentByClass<UStaticMeshComponent>())
	{
		SetActorScale3D(FVector(Radius / AgShockwavePrivate::VisualSize, Radius / AgShockwavePrivate::VisualSize, SpawnData.Height / AgShockwavePrivate::VisualSize));
		Mesh->SetCustomPrimitiveDataFloat(0, Radius > 0.f ? (Radius - SpawnData.Width) / Radius : 0.f);
	}
}

void AAgShockwave::DestroyWhenVFXFinish()
{
	// 마지막 파동과 함께 지우면 그때 터지는 나이아가라도 함께 사라지므로, 켜져 있는 나이아가라가 모두 끝난 뒤 지웁니다.
	bool bPlaying = false;
	TInlineComponentArray<UNiagaraComponent*> NiagaraComponents(this);
	for (UNiagaraComponent* Niagara : NiagaraComponents)
	{
		if (Niagara->IsActive())
		{
			Niagara->OnSystemFinished.AddUniqueDynamic(this, &ThisClass::HandleVFXFinished);
			bPlaying = true;
		}
	}
	if (!bPlaying)
	{
		Destroy();
	}
}

void AAgShockwave::HandleVFXFinished(UNiagaraComponent* Niagara)
{
	DestroyWhenVFXFinish();
}
