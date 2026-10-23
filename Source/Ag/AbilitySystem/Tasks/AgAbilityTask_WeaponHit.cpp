// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_WeaponHit.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystem/AgGameplayEffectContext.h"
#include "AbilitySystemComponent.h"
#include "Character/AgCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/AgCollision.h"
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

	bool GetEdge(const UCapsuleComponent* Capsule, FVector& OutStart, FVector& OutEnd)
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

	FName FindAttachBone(const USceneComponent& Component, const USkeletalMeshComponent& Mesh)
	{
		for (const USceneComponent* Child = &Component; Child; Child = Child->GetAttachParent())
		{
			if (Child->GetAttachParent() == &Mesh)
			{
				return Mesh.GetSocketBoneName(Child->GetAttachSocketName());
			}
		}
		return NAME_None;
	}
} // namespace

UAbilityTask* FAgWeaponHitParams::MakeTask(const FAgTaskContext& Context) const
{
	UAgAbilityTask_WeaponHit* Task = UAbilityTask::NewAbilityTask<UAgAbilityTask_WeaponHit>(&Context.Ability);
	Task->Params = *this;
	return Task;
}

UAgAbilityTask_WeaponHit::UAgAbilityTask_WeaponHit()
{
	SweepTick.Target = this;
	SweepTick.bCanEverTick = true;
	SweepTick.TickGroup = TG_PostPhysics;
	SweepTick.EndTickGroup = TG_PostPhysics;
}

void UAgAbilityTask_WeaponHit::Activate()
{
	Super::Activate();
	AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor());
	if (!Character)
	{
		EndTask();
		return;
	}
	HitReward = FAgHitRewardState::Make(Params.HitReward);
	TArray<UCapsuleComponent*> Capsules;
	Character->GetComponents(Capsules);
	for (UCapsuleComponent* Capsule : Capsules)
	{
		FAgWeaponEdge Edge;
		if (Capsule->ComponentHasTag(Params.ComponentTag) && GetEdge(Capsule, Edge.LastStart, Edge.LastEnd))
		{
			Edge.Capsule = Capsule;
			Edge.HandBone = FindAttachBone(*Capsule, *Character->GetMesh());
			Edges.Add(Edge);
		}
	}
	if (Edges.IsEmpty())
	{
		UE_LOG(LogAg, Error, TEXT("WeaponHit: %s has no capsule tagged %s"), *Character->GetName(), *Params.ComponentTag.ToString());
		EndTask();
		return;
	}
	// 같은 때에 시작한 Task(막힘을 기다리는 Dash)가 모두 시작한 뒤에 피해 결과가 나도록, 첫 스윕도 이 프레임의 물리
	// 갱신 뒤 틱에서 합니다.
	SweepTick.AddPrerequisite(Character->GetMesh(), Character->GetMesh()->PrimaryComponentTick);
	SweepTick.RegisterTickFunction(Character->GetLevel());
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
		Target->SweepEdges();
	}
}

FString UAgAbilityTask_WeaponHit::FAgSweepTickFunction::DiagnosticMessage()
{
	return Target->GetFullName() + TEXT("[SweepTick]");
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
	FVector LastFrom = Edge.LastStart;
	FVector LastTo = Edge.LastEnd;
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
			if (!Target)
			{
				continue;
			}
			const FVector Location = Touch.bStartPenetrating ? Touch.Location : Touch.ImpactPoint;
			// 피격 흔들림이 날이 움직인 방향으로 밀도록, 날의 맞은 지점이 앞 단계부터 지나온 선을
			// TraceStart·End에 담습니다.
			const FVector Blade = To - From;
			const float BladeAlpha = Blade.IsNearlyZero() ? 0.f : FMath::Clamp(((Location - From) | Blade) / Blade.SizeSquared(), 0.f, 1.f);
			FHitResult Contact = Touch;
			Contact.TraceStart = FMath::Lerp(LastFrom, LastTo, BladeAlpha);
			Contact.TraceEnd = FMath::Lerp(From, To, BladeAlpha);
			// 판정 구간 첫 프레임처럼 날이 아직 움직이지 않았으면, 엔진이 애니메이션으로 움직인 키네마틱 손 바디에
			// 계산해 둔 속도로 그 지점의 방향을 구합니다. 공격자도 맞아 흔들리는 중이면 손 바디가 물리로 돌아 보이는
			// 무기와 다르므로 쓰지 않습니다.
			if (Contact.TraceStart.Equals(Contact.TraceEnd))
			{
				const FBodyInstance* Hand = Edge.HandBone.IsNone() ? nullptr : Character->GetMesh()->GetBodyInstance(Edge.HandBone);
				if (Hand && !Hand->IsInstanceSimulatingPhysics())
				{
					Contact.TraceStart = Contact.TraceEnd - Hand->GetUnrealWorldVelocityAtPoint(Contact.TraceEnd) * GetWorld()->GetDeltaSeconds();
				}
			}
			if (!Hit(*Target, Location, Contact))
			{
				return false;
			}
		}
		LastFrom = From;
		LastTo = To;
	}
	Edge.LastStart = EdgeStart;
	Edge.LastEnd = EdgeEnd;
	return true;
}

bool UAgAbilityTask_WeaponHit::Hit(AAgCharacterBase& Target, const FVector& Location, const FHitResult& Contact)
{
	const AAgCharacterBase* Character = CastChecked<AAgCharacterBase>(GetAvatarActor());
	if (Target.IsDead() || Character->GetTeamAttitudeTowards(Target) == ETeamAttitude::Friendly || TouchedActors.Contains(&Target))
	{
		return true;
	}
	TouchedActors.Add(&Target);
	const FGameplayEffectSpecHandle Spec = CastChecked<UAgGameplayAbility>(Ability)->MakeDamageSpec(Params.Damage, HitReward);
	if (Spec.IsValid())
	{
		// 엔진은 Origin이 없으면 HitResult의 TraceStart를 Origin으로 넣으므로, 맞은 위치를 먼저 넣습니다.
		Spec.Data->GetContext().AddOrigin(Location);
		Spec.Data->GetContext().AddHitResult(Contact);
		AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*Spec.Data, Target.GetAbilitySystemComponent());
	}
	// 피해가 행동이나 이 구간을 끝냈으면 엔진이나 재생 Task가 이 Task도 끝냈습니다.
	return IsActive();
}
