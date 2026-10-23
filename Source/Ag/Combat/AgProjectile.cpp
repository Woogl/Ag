// Copyright Woogle. All Rights Reserved.

#include "Combat/AgProjectile.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "Character/AgCharacterBase.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgEffectData.h"

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
	Reflector.Reset();
	TouchedActors.Reset();
}

void AAgProjectile::Reflect(AAgCharacterBase& Defender)
{
	const AAgCharacterBase* Shooter = GetInstigator<AAgCharacterBase>();
	if (!IsValid(Shooter) || !DamageSpec.IsValid() || IsReflected())
	{
		return;
	}
	UAgAbilitySystemComponent* ASC = CastChecked<UAgAbilitySystemComponent>(Defender.GetAbilitySystemComponent());
	const FGameplayEffectSpecHandle ReflectedSpec = ASC->MakeEffectSpec(EAgEffect::Damage, DamageSpec.Data->GetLevel(), ASC->MakeEffectContext());
	if (!ReflectedSpec.IsValid())
	{
		return;
	}
	FAgDamage Damage;
	Damage.HPMultiplier = DamageSpec.Data->GetSetByCallerMagnitude(AgGameplayTags::SetByCaller_HPMultiplier);
	const FGameplayTagContainer& AttackTags = DamageSpec.Data->GetDynamicAssetTags();
	Damage.bGuardable = !AttackTags.HasTagExact(AgGameplayTags::Effect_Damage_Unblockable);
	Damage.bFinisher = AttackTags.HasTagExact(AgGameplayTags::Effect_Damage_Finisher);
	if (AttackTags.HasTagExact(AgGameplayTags::Effect_Damage_Reaction_KnockUp))
	{
		Damage.HitReaction = EAgHitReaction::KnockUp;
	}
	else if (AttackTags.HasTagExact(AgGameplayTags::Effect_Damage_Reaction_KnockDown))
	{
		Damage.HitReaction = EAgHitReaction::KnockDown;
	}
	else if (AttackTags.HasTagExact(AgGameplayTags::Effect_Damage_Reaction_Flinch))
	{
		Damage.HitReaction = EAgHitReaction::Flinch;
	}
	Damage.AddToSpec(*ReflectedSpec.Data);
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
	return !Reflector.IsExplicitlyNull();
}

void AAgProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AAgCharacterBase* ShooterCharacter = GetInstigator<AAgCharacterBase>();
	AAgCharacterBase* DamageSource = IsReflected() ? Reflector.Get() : ShooterCharacter;
	if (!IsValid(ShooterCharacter) || !IsValid(DamageSource) || !DamageSpec.IsValid())
	{
		Destroy();
		return;
	}

	// 벽은 상자 가운데 높이의 선으로 찾아, 상자가 스치는 바닥이나 낮은 턱을 벽으로 치지 않습니다.
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AgProjectile), false, this);
	QueryParams.AddIgnoredActor(DamageSource);
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
		if (!IsValid(Target) || Target->IsDead() || FGenericTeamId::GetAttitude(DamageSource, Target) != ETeamAttitude::Hostile || TouchedActors.Contains(Target))
		{
			continue;
		}
		TouchedActors.Add(Target);
		const bool bWasReflected = IsReflected();
		SetActorLocation(Contact.Location);
		const FVector HitLocation = Target->GetActorLocation() - FlightDirection * Target->GetCapsuleComponent()->GetScaledCapsuleRadius();
		ApplyHit(*Target, HitLocation);
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
	Context.AddInstigator(IsReflected() ? Reflector.Get() : GetInstigator(), this);
	Context.AddOrigin(HitLocation);
	// SetContext는 출처 태그와 어트리뷰트를 다시 캡처하므로 적중 시점의 ATK를 씁니다.
	Spec.SetContext(Context);
	if (UAbilitySystemComponent* InstigatorASC = Context.GetInstigatorAbilitySystemComponent())
	{
		InstigatorASC->ApplyGameplayEffectSpecToTarget(Spec, Target.GetAbilitySystemComponent());
	}
}
