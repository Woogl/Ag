// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_Attack.h"
#include "AbilitySystem/AgEffect_Damage.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Character/AgCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/AgCollision.h"
#include "Engine/World.h"

namespace
{
	/** 빠르게 휘둘러도 몸을 건너뛰지 않도록 무기 날을 이 길이(cm) 이하로 나눠 스윕합니다(최대 MaxSweepSteps 단계). */
	constexpr float SweepStepLength = 10.f;
	constexpr int32 MaxSweepSteps = 16;

	/** 캐릭터 축에서 이보다 가까우면 축 둘레의 각도가 불안정하므로, 나눈 위치를 직선으로 잡습니다. */
	constexpr float MinArcRadius = 20.f;

	/**
	 * 나눈 단계의 A와 B 사이 위치. 휘두르기는 몸을 돌아 지나가고 한 프레임에 4분의 1바퀴까지 돌기도 하므로, 직선 대신
	 * Center를 지나는 캐릭터 수직축 둘레의 호를 따라갑니다.
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

UAgAbilityTask_Attack::UAgAbilityTask_Attack()
{
	AttackTick.Target = this;
	AttackTick.bCanEverTick = true;
	AttackTick.TickGroup = TG_PostPhysics;
	AttackTick.EndTickGroup = TG_PostPhysics;
}

void UAgAbilityTask_Attack::FAgAttackTickFunction::ExecuteTick(
    float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
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

UAgAbilityTask_Attack* UAgAbilityTask_Attack::Sweep(UGameplayAbility* Owner, const FAgAttackParams& InParams)
{
	UAgAbilityTask_Attack* Task = NewAbilityTask<UAgAbilityTask_Attack>(Owner);
	Task->Params = InParams;
	return Task;
}

UAgAbilityTask_Attack* UAgAbilityTask_Attack::Strike(UGameplayAbility* Owner, const FAgAttackParams& InParams,
    const TArray<AAgCharacterBase*>& InTargets, bool bInFinisher, TOptional<FVector> InHitLocation)
{
	UAgAbilityTask_Attack* Task = Sweep(Owner, InParams);
	Task->HitLocation = InHitLocation;
	Task->bSingleStrike = true;
	Task->bFinisher = bInFinisher;
	for (AAgCharacterBase* Target : InTargets)
	{
		if (Target)
		{
			Task->Targets.AddUnique(Target);
		}
	}
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
	if (bSingleStrike)
	{
		for (const TWeakObjectPtr<AAgCharacterBase>& Target : Targets)
		{
			if (IsFinished() || !Ability->IsActive())
			{
				break;
			}
			if (!Target.IsValid() || Target->IsDead())
			{
				continue;
			}
			const FGameplayEffectSpecHandle Spec = Ability->MakeOutgoingGameplayEffectSpec(UAgEffect_Damage::StaticClass());
			UAgEffect_Damage::SetAttack(*Spec.Data, Params.MakeHit(bFinisher), HitLocation.Get(Target->GetActorLocation()));
			AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*Spec.Data, Target->GetAbilitySystemComponent());
		}
		EndTask();
		return;
	}
	WeaponCollision = Character->FindComponentByTag<UCapsuleComponent>(AAgCharacterBase::WeaponCollisionTag);
	if (!GetWeaponCollision(LastEdgeStart, LastEdgeEnd))
	{
		EndTask();
		return;
	}
	// 첫 판정의 피해 콜백에서 취소되면 OnDestroy가 이미 등록된 틱까지 정리합니다.
	AttackTick.AddPrerequisite(Character->GetMesh(), Character->GetMesh()->PrimaryComponentTick);
	AttackTick.RegisterTickFunction(Character->GetLevel());
	SweepWeapon();
}

void UAgAbilityTask_Attack::TickTask(float DeltaTime)
{
	if (!IsActive())
	{
		return;
	}
	const AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor());
	if (!Character || Character->IsDead() || !Ability || !Ability->IsActive() || !WeaponCollision.IsValid())
	{
		EndTask();
		return;
	}
	SweepWeapon();
}

bool UAgAbilityTask_Attack::GetWeaponCollision(FVector& OutStart, FVector& OutEnd) const
{
	const UCapsuleComponent* Capsule = WeaponCollision.Get();
	if (!Capsule)
	{
		return false;
	}
	// 캡슐의 축은 로컬 Z입니다. 크기와 부착 위치는 캐릭터 BP의 무기 제작 정보입니다.
	const FVector Center = Capsule->GetComponentLocation();
	const FVector HalfSegment = Capsule->GetUpVector() * Capsule->GetScaledCapsuleHalfHeight_WithoutHemisphere();
	OutStart = Center + HalfSegment;
	OutEnd = Center - HalfSegment;
	return true;
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
	const FAgPlayerAttackHit Hit = Params.MakeHit();

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
			if (!Target || Character->GetTeamAttitudeTowards(*Target) != ETeamAttitude::Hostile || TouchedActors.Contains(Target))
			{
				continue;
			}
			// 무적 등으로 피해가 없어도 한 판정에서는 대상마다 한 번만 처리합니다.
			TouchedActors.Add(Target);
			const FVector Location = Touch.bStartPenetrating ? Touch.Location : Touch.ImpactPoint;
			const FGameplayEffectSpecHandle Spec = Ability->MakeOutgoingGameplayEffectSpec(UAgEffect_Damage::StaticClass());
			UAgEffect_Damage::SetAttack(*Spec.Data, Hit, Location);
			AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*Spec.Data, Target->GetAbilitySystemComponent());
			if (!IsActive() || !Ability->IsActive())
			{
				EndTask();
				return;
			}
		}
	}
	LastEdgeStart = EdgeStart;
	LastEdgeEnd = EdgeEnd;
}

void UAgAbilityTask_Attack::OnDestroy(bool bInOwnerFinished)
{
	AttackTick.SetTickFunctionEnable(false);
	AttackTick.UnRegisterTickFunction();
	WeaponCollision.Reset();
	TouchedActors.Reset();
	Super::OnDestroy(bInOwnerFinished);
}
