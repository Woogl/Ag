// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_WeaponHit.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Character/AgCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/AgCollision.h"
#include "Data/AgEffectData.h"
#include "Engine/World.h"
#include "Ag.h"

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

UAbilityTask* FAgWeaponHitParams::MakeTask(const FAgTaskContext& Context) const
{
	return UAgAbilityTask_WeaponHit::WeaponHit(&Context.Ability, *this);
}

UAgAbilityTask_WeaponHit::UAgAbilityTask_WeaponHit()
{
	SweepTick.Target = this;
	SweepTick.bCanEverTick = true;
	SweepTick.TickGroup = TG_PostPhysics;
	SweepTick.EndTickGroup = TG_PostPhysics;
}

UAgAbilityTask_WeaponHit* UAgAbilityTask_WeaponHit::WeaponHit(UGameplayAbility* Owner, const FAgWeaponHitParams& InParams)
{
	UAgAbilityTask_WeaponHit* Task = NewAbilityTask<UAgAbilityTask_WeaponHit>(Owner);
	Task->Params = InParams;
	return Task;
}

void UAgAbilityTask_WeaponHit::Activate()
{
	Super::Activate();
	AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor());
	if (!Character || Character->IsDead() || !Ability || !Ability->IsActive())
	{
		EndTask();
		return;
	}
	TArray<UCapsuleComponent*> Capsules;
	Character->GetComponents(Capsules);
	for (UCapsuleComponent* Capsule : Capsules)
	{
		FAgWeaponEdge Edge;
		if (Capsule->ComponentHasTag(Params.ComponentTag) && GetEdge(Capsule, Edge.LastStart, Edge.LastEnd))
		{
			Edge.Capsule = Capsule;
			Edges.Add(Edge);
		}
	}
	if (Edges.IsEmpty())
	{
		UE_LOG(LogAg, Error, TEXT("WeaponHit: %s has no capsule tagged %s"), *Character->GetName(), *Params.ComponentTag.ToString());
		EndTask();
		return;
	}
	// 첫 판정의 피해 콜백에서 취소되면 OnDestroy가 이미 등록된 틱까지 정리합니다.
	SweepTick.AddPrerequisite(Character->GetMesh(), Character->GetMesh()->PrimaryComponentTick);
	SweepTick.RegisterTickFunction(Character->GetLevel());
	SweepEdges();
}

void UAgAbilityTask_WeaponHit::TickTask(float DeltaTime)
{
	if (!IsActive())
	{
		return;
	}
	const AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor());
	if (!Character || Character->IsDead() || !Ability || !Ability->IsActive())
	{
		EndTask();
		return;
	}
	SweepEdges();
}

void UAgAbilityTask_WeaponHit::OnDestroy(bool bInOwnerFinished)
{
	SweepTick.SetTickFunctionEnable(false);
	SweepTick.UnRegisterTickFunction();
	Super::OnDestroy(bInOwnerFinished);
}

void UAgAbilityTask_WeaponHit::FAgSweepTickFunction::ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	if (IsValid(Target) && Target->IsActive() && TickType != LEVELTICK_ViewportsOnly)
	{
		Target->TickTask(DeltaTime);
	}
}

FString UAgAbilityTask_WeaponHit::FAgSweepTickFunction::DiagnosticMessage()
{
	return Target->GetFullName() + TEXT("[SweepTick]");
}

bool UAgAbilityTask_WeaponHit::GetEdge(const UCapsuleComponent* Capsule, FVector& OutStart, FVector& OutEnd)
{
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

void UAgAbilityTask_WeaponHit::SweepEdges()
{
	for (FAgWeaponEdge& Edge : Edges)
	{
		// 피해가 행동을 끝냈으면 이 Task도 끝났으므로 남은 날은 보지 않습니다.
		if (!Sweep(Edge))
		{
			return;
		}
	}
}

bool UAgAbilityTask_WeaponHit::Sweep(FAgWeaponEdge& Edge)
{
	const UCapsuleComponent* Capsule = Edge.Capsule.Get();
	FVector EdgeStart, EdgeEnd;
	if (!GetEdge(Capsule, EdgeStart, EdgeEnd))
	{
		return true;
	}
	AAgCharacterBase* Character = CastChecked<AAgCharacterBase>(GetAvatarActor());
	const float Travel = FMath::Max(FVector::Dist(EdgeStart, Edge.LastStart), FVector::Dist(EdgeEnd, Edge.LastEnd));
	const int32 Steps = FMath::Clamp(FMath::CeilToInt(Travel / SweepStepLength), 1, MaxSweepSteps);
	const FCollisionShape Shape = FCollisionShape::MakeSphere(Capsule->GetScaledCapsuleRadius());
	const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AgWeaponSweep), false, Character);
	const FVector Center = Character->GetActorLocation();

	TArray<FHitResult> Touches;
	for (int32 Step = 1; Step <= Steps; ++Step)
	{
		const float Alpha = static_cast<float>(Step) / Steps;
		const FVector From = ArcLerp(Center, Edge.LastStart, EdgeStart, Alpha);
		const FVector To = ArcLerp(Center, Edge.LastEnd, EdgeEnd, Alpha);
		Touches.Reset();
		GetWorld()->SweepMultiByChannel(Touches, From, To, FQuat::Identity, ECC_AgWeapon, Shape, QueryParams);
		for (const FHitResult& Touch : Touches)
		{
			AAgCharacterBase* Target = Cast<AAgCharacterBase>(Touch.GetActor());
			if (Target && !Hit(*Target, Touch.bStartPenetrating ? Touch.Location : Touch.ImpactPoint))
			{
				return false;
			}
		}
	}
	Edge.LastStart = EdgeStart;
	Edge.LastEnd = EdgeEnd;
	return true;
}

bool UAgAbilityTask_WeaponHit::Hit(AAgCharacterBase& Target, const FVector& Location)
{
	const AAgCharacterBase* Character = CastChecked<AAgCharacterBase>(GetAvatarActor());
	if (Target.IsDead() || Character->GetTeamAttitudeTowards(Target) != ETeamAttitude::Hostile || TouchedActors.Contains(&Target))
	{
		return true;
	}
	TouchedActors.Add(&Target);
	const FGameplayEffectSpecHandle Spec = CastChecked<UAgGameplayAbility>(Ability)->MakeEffectSpec(EAgEffect::Damage);
	if (Spec.IsValid())
	{
		Spec.Data->GetContext().AddOrigin(Location);
		Params.Damage.AddToSpec(*Spec.Data);
		AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*Spec.Data, Target.GetAbilitySystemComponent());
	}
	if (!IsActive() || !Ability->IsActive())
	{
		EndTask();
		return false;
	}
	return true;
}
