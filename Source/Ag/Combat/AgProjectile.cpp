// Copyright Woogle. All Rights Reserved.

#include "Combat/AgProjectile.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystem/AgGameplayEffectContext.h"
#include "Character/AgCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/ShapeComponent.h"
#include "Engine/World.h"
#include "Core/AgAssetManager.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCombatRules.h"
#include "Data/AgEffectData.h"
#include "Ag.h"

namespace
{
	/** From에서 Speed로 곧게 날아간 투사체가 수평 속도 Velocity로 움직이는 대상에 닿는 위치입니다. */
	FVector PredictLocation(const FVector& From, const FVector& TargetLocation, const FVector& Velocity, float Speed)
	{
		// |Offset + Velocity × T| = Speed × T의 양의 근입니다. 대상이 투사체보다 빠르면 근이 없을 수 있어 지금 거리로 셉니다.
		const FVector Offset = TargetLocation - From;
		const double A = Velocity.SizeSquared() - FMath::Square(Speed);
		const double B = 2.0 * FVector::DotProduct(Offset, Velocity);
		const double C = Offset.SizeSquared();
		const double Time = A < 0.0 ? (-B - FMath::Sqrt(B * B - 4.0 * A * C)) / (2.0 * A) : Offset.Size() / Speed;
		return TargetLocation + Velocity * Time;
	}

} // namespace

AAgProjectile::AAgProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AAgProjectile::OnSpawnedByAbility(const UAgGameplayAbility& Ability)
{
	DamageSpec = Ability.MakeDamageSpec(Damage, MakeShared<FAgHitState>());
	if (DamageSpec.IsValid())
	{
		DamageSpec.Data->GetContext().AddInstigator(GetInstigator(), this);
	}
	AimTarget = Ability.GetActionTarget();
}

void AAgProjectile::Reflect(AAgCharacterBase& Defender)
{
	const AAgCharacterBase* Shooter = GetInstigator<AAgCharacterBase>();
	if (!IsValid(Shooter) || !DamageSpec.IsValid() || IsReflected())
	{
		return;
	}
	UAgAbilitySystemComponent* ASC = Defender.GetAbilitySystemComponent();
	const FGameplayEffectSpecHandle ReflectedSpec = ASC->MakeEffectSpec(EAgEffect::Damage, DamageSpec.Data->GetLevel(), ASC->MakeEffectContext());
	if (!ReflectedSpec.IsValid())
	{
		return;
	}
	const UAgCombatRules& Rules = UAgAssetManager::Get().GetCombatRules();
	Damage.HPMultiplier = Rules.ParryHPMultiplier;
	Damage.PPMultiplier = Rules.ParryPPMultiplier;
	Damage.AddToSpec(*ReflectedSpec.Data);
	ReflectedSpec.Data->GetContext().AddInstigator(&Defender, this);
	DamageSpec = ReflectedSpec;
	Reflector = &Defender;
	const FVector ReturnDirection = (Shooter->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	FlightDirection = ReturnDirection.IsNearlyZero() ? -FlightDirection : ReturnDirection;
	SetActorRotation(FlightDirection.Rotation());
	Traveled = 0.f;
	TouchedActors.Reset();
}

bool AAgProjectile::IsReflected() const
{
	// 반사한 캐릭터가 사라져도 반사됨으로 남아야, 원래 발사자의 피해로 돌아가지 않고 Tick에서 사라집니다.
	return !Reflector.IsExplicitlyNull();
}

void AAgProjectile::OnDamageApplied()
{
	if (bDestroyOnHit)
	{
		Destroy();
	}
}

void AAgProjectile::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	GetComponents(HitShapes);
	if (HitShapes.IsEmpty())
	{
		UE_LOG(LogAg, Error, TEXT("%s: no shape component for hits"), *GetClass()->GetName());
		return;
	}
	// 판정 모양은 매 틱 코드에서 직접 검사하므로 자체 충돌이 필요 없습니다.
	for (UShapeComponent* Shape : HitShapes)
	{
		Shape->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Shape->SetGenerateOverlapEvents(false);
	}
}

void AAgProjectile::BeginPlay()
{
	Super::BeginPlay();

	FlightDirection = GetActorForwardVector();
	// 판정 모양과 루트가 BP 컴포넌트라, 생성을 마친 이때부터 발사 위치를 읽을 수 있습니다.
	const AActor* Target = AimTarget.Get();
	if (!Target || Speed <= 0.f)
	{
		return;
	}
	const FVector From = GetActorLocation();
	const FVector AimLocation = PredictLocation(From, Target->GetActorLocation(),Target->GetVelocity() * FVector(1.0, 1.0, 0.0), Speed);
	const FVector AimDirection = (AimLocation - From).GetSafeNormal();
	if (!AimDirection.IsNearlyZero())
	{
		FlightDirection = AimDirection;
		SetActorRotation(FlightDirection.Rotation());
	}
}

void AAgProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AAgCharacterBase* ShooterCharacter = GetInstigator<AAgCharacterBase>();
	AAgCharacterBase* DamageSource = IsReflected() ? Reflector.Get() : ShooterCharacter;
	if (HitShapes.IsEmpty() || !IsValid(ShooterCharacter) || !IsValid(DamageSource) || !DamageSpec.IsValid())
	{
		Destroy();
		return;
	}

	// 판정은 모양 컴포넌트마다 제 자리와 방향으로 쓸고 액터는 판정이 간 만큼 옮깁니다. 벽은 각 판정 모양 가운데의 선으로 찾아,
	// 판정이 스치는 바닥이나 낮은 턱을 벽으로 치지 않고, 가장 먼저 벽에 닿는 모양까지만 갑니다.
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AgProjectile), false, this);
	QueryParams.AddIgnoredActor(DamageSource);
	const FVector ActorStart = GetActorLocation();
	const float Step = FMath::Min(Speed * DeltaSeconds, Range - Traveled);
	float MoveDistance = Step;
	bool bHitWall = false;
	for (const UShapeComponent* Shape : HitShapes)
	{
		const FVector Start = Shape->GetComponentLocation();
		FHitResult WallHit;
		if (GetWorld()->LineTraceSingleByObjectType(WallHit, Start, Start + FlightDirection * MoveDistance,
				FCollisionObjectQueryParams(ECC_WorldStatic), QueryParams))
		{
			MoveDistance = WallHit.Distance;
			bHitWall = true;
		}
	}

	const FVector Move = FlightDirection * MoveDistance;
	TArray<FHitResult> Reached;
	for (const UShapeComponent* Shape : HitShapes)
	{
		const FVector Start = Shape->GetComponentLocation();
		TArray<FHitResult> ShapeReached;
		GetWorld()->SweepMultiByObjectType(ShapeReached, Start, Start + Move, Shape->GetComponentQuat(),
			FCollisionObjectQueryParams(ECC_Pawn), Shape->GetCollisionShape(), QueryParams);
		Reached.Append(ShapeReached);
	}
	// 모든 모양이 같은 거리를 쓸므로 Time이 곧 이동 거리의 비율이고, 여러 모양의 충돌을 닿은 순서로 맞힙니다.
	Reached.StableSort([](const FHitResult& A, const FHitResult& B) { return A.Time < B.Time; });
	for (const FHitResult& Contact : Reached)
	{
		AAgCharacterBase* Target = Cast<AAgCharacterBase>(Contact.GetActor());
		if (!IsValid(Target) || !Target->CanBeHitBy(*DamageSource) || TouchedActors.Contains(Target))
		{
			continue;
		}
		TouchedActors.Add(Target);
		const bool bWasReflected = IsReflected();
		SetActorLocation(ActorStart + Move * Contact.Time);
		const FVector HitLocation = Target->GetActorLocation() - FlightDirection * Target->GetCapsuleComponent()->GetScaledCapsuleRadius();
		// 패리로 반사하면 적용하는 도중에 DamageSpec이 바뀌므로, 적용이 끝날 때까지 쓰던 스펙을 붙잡아 둡니다.
		const FGameplayEffectSpecHandle Spec = DamageSpec;
		Spec.Data->GetContext().AddOrigin(HitLocation);
		DamageSource->GetAbilitySystemComponent()->ApplyGameplayEffectSpecToTarget(*Spec.Data, Target->GetAbilitySystemComponent());
		if (IsActorBeingDestroyed() || !IsValid(ShooterCharacter))
		{
			Destroy();
			return;
		}
		// 패리 전에 구한 남은 충돌과 벽은 반사 경로에 속하지 않습니다.
		if (bWasReflected != IsReflected())
		{
			return;
		}
	}

	if (bHitWall)
	{
		Destroy();
		return;
	}
	SetActorLocation(ActorStart + Move);
	Traveled += Step;
	if (Traveled >= Range)
	{
		Destroy();
	}
}
