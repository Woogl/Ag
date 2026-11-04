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
#include "Core/AgGameplayTags.h"
#include "Data/AgEffectData.h"
#include "Ag.h"

AAgProjectile::AAgProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AAgProjectile::OnSpawnedByAbility(const UAgGameplayAbility& Ability)
{
	DamageSpec = Ability.MakeDamageSpec(Damage, MakeShared<FAgHitState>());
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
	// 반사탄 적중의 PP 감소는 Parried 어빌리티가 맡으므로 반사한 피해에는 PP 배율을 싣지 않습니다.
	Damage.PPMultiplier = 0.f;
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
	// 반사한 캐릭터가 사라져도 반사됨으로 남아야, 원래 발사자의 피해로 돌아가지 않고 Tick에서 사라집니다.
	return !Reflector.IsExplicitlyNull();
}

void AAgProjectile::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	HitShape = FindComponentByClass<UShapeComponent>();
	if (!HitShape)
	{
		UE_LOG(LogAg, Error, TEXT("%s: no shape component for hits"), *GetClass()->GetName());
		return;
	}
	// 판정 모양은 매 틱 코드에서 직접 검사하므로 자체 충돌이 필요 없습니다.
	HitShape->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitShape->SetGenerateOverlapEvents(false);
}

void AAgProjectile::BeginPlay()
{
	Super::BeginPlay();

	FlightDirection = GetActorForwardVector();
}

void AAgProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AAgCharacterBase* ShooterCharacter = GetInstigator<AAgCharacterBase>();
	AAgCharacterBase* DamageSource = IsReflected() ? Reflector.Get() : ShooterCharacter;
	if (!HitShape || !IsValid(ShooterCharacter) || !IsValid(DamageSource) || !DamageSpec.IsValid())
	{
		Destroy();
		return;
	}

	// 판정은 모양 컴포넌트의 자리와 방향으로 쓸고 액터는 판정이 간 만큼 옮깁니다. 벽은 판정 모양 가운데의 선으로 찾아,
	// 판정이 스치는 바닥이나 낮은 턱을 벽으로 치지 않습니다.
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AgProjectile), false, this);
	QueryParams.AddIgnoredActor(DamageSource);
	const FVector ActorStart = GetActorLocation();
	const FVector Start = HitShape->GetComponentLocation();
	const float Step = FMath::Min(Speed * DeltaSeconds, Range - Traveled);
	FVector End = Start + FlightDirection * Step;
	FHitResult WallHit;
	const bool bHitWall =
		GetWorld()->LineTraceSingleByObjectType(WallHit, Start, End, FCollisionObjectQueryParams(ECC_WorldStatic), QueryParams);
	if (bHitWall)
	{
		End = WallHit.Location;
	}

	TArray<FHitResult> Reached;
	GetWorld()->SweepMultiByObjectType(Reached, Start, End, HitShape->GetComponentQuat(), FCollisionObjectQueryParams(ECC_Pawn),
		HitShape->GetCollisionShape(), QueryParams);
	for (const FHitResult& Contact : Reached)
	{
		AAgCharacterBase* Target = Cast<AAgCharacterBase>(Contact.GetActor());
		if (!IsValid(Target) || !Target->CanBeHitBy(*DamageSource) || TouchedActors.Contains(Target))
		{
			continue;
		}
		TouchedActors.Add(Target);
		const bool bWasReflected = IsReflected();
		SetActorLocation(ActorStart + (Contact.Location - Start));
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
	SetActorLocation(ActorStart + (End - Start));
	Traveled += Step;
	if (Traveled >= Range)
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
