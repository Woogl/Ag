// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_Attack.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Character/AgCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/AgCollision.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgEffectData.h"
#include "Data/AgJsonRead.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace
{
	/** 빠르게 휘둘러도 몸을 건너뛰지 않도록 무기 날의 이동을 나눠 스윕합니다. */
	constexpr float SweepStepLength = 10.f;
	constexpr int32 MaxSweepSteps = 16;

	/** 캐릭터 축에서 이보다 가까우면 축 둘레의 각도가 불안정하므로, 나눈 위치를 직선으로 잡습니다. */
	constexpr float MinArcRadius = 20.f;

	/**
	 * 휘두르기는 몸을 돌아 지나가고 한 프레임에 4분의 1바퀴까지 돌기도 하므로, 직선 대신 Center를 지나는 캐릭터 수직축
	 * 둘레의 호를 따라갑니다.
	 */
	FVector ArcLerp(const FVector& Center, const FVector& A, const FVector& B, float Alpha)
	{
		const FVector2D RelA(A.X - Center.X, A.Y - Center.Y);
		const FVector2D RelB(B.X - Center.X, B.Y - Center.Y);
		const float RadiusA = RelA.Size();
		const float RadiusB = RelB.Size();
		if (RadiusA < MinArcRadius || RadiusB < MinArcRadius)
		{
			return FMath::Lerp(A, B, Alpha);
		}

		const float AngleA = FMath::Atan2(RelA.Y, RelA.X);
		const float Angle = AngleA + FMath::FindDeltaAngleRadians(AngleA, FMath::Atan2(RelB.Y, RelB.X)) * Alpha;
		const float Radius = FMath::Lerp(RadiusA, RadiusB, Alpha);
		return FVector(Center.X + Radius * FMath::Cos(Angle), Center.Y + Radius * FMath::Sin(Angle), FMath::Lerp(A.Z, B.Z, Alpha));
	}

}

FAgPlayerAttackHit FAgAttackParams::MakeHit() const
{
	FAgPlayerAttackHit Hit;
	Hit.DamageMultiplier = DamageMultiplier;
	Hit.PoiseMultiplier = PoiseMultiplier;
	Hit.HitReaction = HitReaction;
	Hit.bGuardable = bGuardable;
	Hit.MPCharge = MPCharge;
	Hit.UPCharge = UPCharge;
	Hit.bFinisher = bFinisher;
	return Hit;
}

void FAgAttackParams::GetFieldRules(FAgJsonFieldRules& Rules) const
{
	Rules.Optional.Add(GET_MEMBER_NAME_CHECKED(FAgAttackParams, bFinisher));
	Rules.Optional.Add(GET_MEMBER_NAME_CHECKED(FAgAttackParams, Radius));
	Rules.Optional.Add(GET_MEMBER_NAME_CHECKED(FAgAttackParams, bGroundOnly));
}

void FAgAttackParams::Validate(const FJsonObject& Json, EAgTaskPlacement Placement, const FString& Path, TArray<FString>& Errors)
{
	bHasRadius = Json.HasField(TEXT("Radius"));
	if (bHasRadius && Radius <= 0.f)
	{
		Errors.Add(Path + TEXT(".Radius: must be positive"));
	}
}

UAbilityTask* FAgAttackParams::MakeTask(const FAgTaskContext& Context) const
{
	return UAgAbilityTask_Attack::Attack(&Context.Runner, *this, Context.Placement == EAgTaskPlacement::Point);
}

UAgAbilityTask_Attack::UAgAbilityTask_Attack()
{
	AttackTick.Target = this;
	AttackTick.bCanEverTick = true;
	AttackTick.TickGroup = TG_PostPhysics;
	AttackTick.EndTickGroup = TG_PostPhysics;
}

UAgAbilityTask_Attack* UAgAbilityTask_Attack::Attack(UGameplayAbility* Owner, const FAgAttackParams& InParams, bool bInOnce)
{
	UAgAbilityTask_Attack* Task = NewAbilityTask<UAgAbilityTask_Attack>(Owner);
	Task->Params = InParams;
	Task->bOnce = bInOnce;
	return Task;
}

void UAgAbilityTask_Attack::Activate()
{
	Super::Activate();
	AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor());
	if (!Character || Character->IsDead() || !Ability || !Ability->IsActive())
	{
		EndTask();
		return;
	}
	if (!Params.bHasRadius)
	{
		WeaponCollision = Character->FindComponentByTag<UCapsuleComponent>(AAgCharacterBase::WeaponCollisionTag);
		if (!GetWeaponCollision(LastEdgeStart, LastEdgeEnd))
		{
			EndTask();
			return;
		}
	}
	if (bOnce)
	{
		Detect();
		if (!IsFinished())
		{
			EndTask();
		}
		return;
	}
	// 첫 판정의 피해 콜백에서 취소되면 OnDestroy가 이미 등록된 틱까지 정리합니다.
	AttackTick.AddPrerequisite(Character->GetMesh(), Character->GetMesh()->PrimaryComponentTick);
	AttackTick.RegisterTickFunction(Character->GetLevel());
	Detect();
}

void UAgAbilityTask_Attack::TickTask(float DeltaTime)
{
	if (!IsActive())
	{
		return;
	}
	const AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor());
	if (!Character || Character->IsDead() || !Ability || !Ability->IsActive() || (!Params.bHasRadius && !WeaponCollision.IsValid()))
	{
		EndTask();
		return;
	}
	Detect();
}

void UAgAbilityTask_Attack::OnDestroy(bool bInOwnerFinished)
{
	AttackTick.SetTickFunctionEnable(false);
	AttackTick.UnRegisterTickFunction();
	WeaponCollision.Reset();
	TouchedActors.Reset();
	Super::OnDestroy(bInOwnerFinished);
}

void UAgAbilityTask_Attack::FAgAttackTickFunction::ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	if (IsValid(Target) && Target->IsActive() && TickType != LEVELTICK_ViewportsOnly)
	{
		Target->TickTask(DeltaTime);
	}
}

FString UAgAbilityTask_Attack::FAgAttackTickFunction::DiagnosticMessage()
{
	return Target->GetFullName() + TEXT("[AttackTick]");
}

bool UAgAbilityTask_Attack::GetWeaponCollision(FVector& OutStart, FVector& OutEnd) const
{
	const UCapsuleComponent* Capsule = WeaponCollision.Get();
	if (!Capsule)
	{
		return false;
	}
	const FVector Center = Capsule->GetComponentLocation();
	const FVector HalfSegment = Capsule->GetUpVector() * Capsule->GetScaledCapsuleHalfHeight_WithoutHemisphere();
	OutStart = Center + HalfSegment;
	OutEnd = Center - HalfSegment;
	return true;
}

void UAgAbilityTask_Attack::Detect()
{
	if (Params.bHasRadius)
	{
		HitArea();
	}
	else
	{
		SweepWeapon();
	}
}

void UAgAbilityTask_Attack::SweepWeapon()
{
	AAgCharacterBase* Character = CastChecked<AAgCharacterBase>(GetAvatarActor());
	FVector EdgeStart, EdgeEnd;
	if (!GetWeaponCollision(EdgeStart, EdgeEnd))
	{
		return;
	}
	const float Travel = FMath::Max(FVector::Dist(EdgeStart, LastEdgeStart), FVector::Dist(EdgeEnd, LastEdgeEnd));
	const int32 Steps = FMath::Clamp(FMath::CeilToInt(Travel / SweepStepLength), 1, MaxSweepSteps);
	const FCollisionShape Edge = FCollisionShape::MakeSphere(WeaponCollision->GetScaledCapsuleRadius());
	const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AgWeaponSweep), false, Character);
	const FVector Center = Character->GetActorLocation();

	TArray<FHitResult> Touches;
	for (int32 Step = 1; Step <= Steps; ++Step)
	{
		const float Alpha = static_cast<float>(Step) / Steps;
		const FVector From = ArcLerp(Center, LastEdgeStart, EdgeStart, Alpha);
		const FVector To = ArcLerp(Center, LastEdgeEnd, EdgeEnd, Alpha);
		Touches.Reset();
		GetWorld()->SweepMultiByChannel(Touches, From, To, FQuat::Identity, ECC_AgWeapon, Edge, QueryParams);
		for (const FHitResult& Touch : Touches)
		{
			AAgCharacterBase* Target = Cast<AAgCharacterBase>(Touch.GetActor());
			if (Target && !Hit(*Target, Touch.bStartPenetrating ? Touch.Location : Touch.ImpactPoint))
			{
				return;
			}
		}
	}
	LastEdgeStart = EdgeStart;
	LastEdgeEnd = EdgeEnd;
}

void UAgAbilityTask_Attack::HitArea()
{
	const AAgCharacterBase* Character = CastChecked<AAgCharacterBase>(GetAvatarActor());
	TArray<AAgCharacterBase*> InRange;
	for (TActorIterator<AAgCharacterBase> It(GetWorld()); It; ++It)
	{
		if (*It != Character && Character->GetHorizontalDistanceTo(*It) <= Params.Radius)
		{
			InRange.Add(*It);
		}
	}
	for (AAgCharacterBase* Target : InRange)
	{
		if (!Hit(*Target, Target->GetActorLocation()))
		{
			return;
		}
	}
}

bool UAgAbilityTask_Attack::Hit(AAgCharacterBase& Target, const FVector& Location)
{
	const AAgCharacterBase* Character = CastChecked<AAgCharacterBase>(GetAvatarActor());
	const bool bAirborne = Target.GetAbilitySystemComponent()->HasMatchingGameplayTag(AgGameplayTags::State_Airborne);
	if (Target.IsDead() || Character->GetTeamAttitudeTowards(Target) != ETeamAttitude::Hostile || TouchedActors.Contains(&Target) ||
		(Params.bGroundOnly && bAirborne))
	{
		return true;
	}
	TouchedActors.Add(&Target);
	const FGameplayEffectSpecHandle Spec = CastChecked<UAgGameplayAbility>(Ability)->MakeEffectSpec(EAgEffect::Damage);
	if (Spec.IsValid())
	{
		Spec.Data->GetContext().AddOrigin(Location);
		Params.MakeHit().AddToSpec(*Spec.Data);
		AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*Spec.Data, Target.GetAbilitySystemComponent());
	}
	if (!IsActive() || !Ability->IsActive())
	{
		if (!IsFinished())
		{
			EndTask();
		}
		return false;
	}
	return true;
}
