// Copyright Woogle. All Rights Reserved.

#include "Character/AgHitPhysicsComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Character/AgCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/AgGameplayTags.h"
#include "PhysicsEngine/BodyInstance.h"

namespace
{
	const FName TorsoBone(TEXT("spine_02"));
	const FName GuardRangeBone(TEXT("spine_03"));
	const FName ArmBones[] = {FName(TEXT("clavicle_l")), FName(TEXT("clavicle_r"))};
	const FName HandBones[] = {FName(TEXT("hand_l")), FName(TEXT("hand_r"))};

	constexpr int32 RangeDepth = 1;
	constexpr float BlendInTime = 0.1f;
	constexpr float BlendOutTime = 0.3f;

	FName FindBodyAbove(const USkeletalMeshComponent& Mesh, FName Bone)
	{
		for (FName Parent = Mesh.GetParentBone(Bone); !Parent.IsNone(); Parent = Mesh.GetParentBone(Parent))
		{
			if (Mesh.GetBodyInstance(Parent))
			{
				return Parent;
			}
		}
		return NAME_None;
	}

	bool IsUnder(const USkeletalMeshComponent& Mesh, FName Bone, FName Root)
	{
		return Bone == Root || Mesh.BoneIsChildOf(Bone, Root);
	}

	FName FindHitBone(const USkeletalMeshComponent& Mesh, const FAgPhysicsHit& Hit)
	{
		FName Bone = Hit.Bone;
		if (Bone.IsNone() || !Mesh.GetBodyInstance(Bone))
		{
			FClosestPointOnPhysicsAsset Closest;
			Bone = Mesh.GetClosestPointOnPhysicsAsset(Hit.Location, Closest, true) ? Closest.BoneName : NAME_None;
		}
		for (const FName& Arm : ArmBones)
		{
			if (IsUnder(Mesh, Bone, Arm))
			{
				return FindBodyAbove(Mesh, Arm);
			}
		}
		return IsUnder(Mesh, Bone, TorsoBone) ? Bone : TorsoBone;
	}

	FName FindRangeBone(const USkeletalMeshComponent& Mesh, FName HitBone)
	{
		FName Bone = HitBone;
		for (int32 Step = 0; Step < RangeDepth && Bone != TorsoBone; ++Step)
		{
			Bone = FindBodyAbove(Mesh, Bone);
		}
		return Bone;
	}

	void PushAtHit(FBodyInstance& Body, const FAgPhysicsHit& Hit, float Speed)
	{
		// 팔에 맞아 몸통을 밀 때처럼 맞은 곳이 바디 밖이면, 지나치게 돌지 않게 바디 표면의 가장 가까운 점을 밉니다.
		FVector Point = Hit.Location;
		Body.GetDistanceToBody(Hit.Location, Point);
		// 위치를 주는 엔진 밀기는 속도 변화를 받지 않아, 질량을 곱해 다른 바디와 같은 속도 변화로 맞춥니다.
		Body.AddImpulseAtPosition(Hit.Direction * Speed * Body.GetBodyMass(), Point);
	}
} // namespace

UAgHitPhysicsComponent::UAgHitPhysicsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	// 그 프레임의 물리 스텝과 물리 자세 섞기 전에 바디와 비율을 바꿉니다.
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UAgHitPhysicsComponent::Play(const FAgPhysicsHit& Hit)
{
	Request(Hit, false);
}

void UAgHitPhysicsComponent::PlayGuard(const FAgPhysicsHit& Hit)
{
	Request(Hit, true);
}

void UAgHitPhysicsComponent::Stop()
{
	// 히트스톱 중에 래그돌로 넘어가면 멈춰 둔 바디가 중력을 받지 않으므로 되돌립니다.
	ResumeBodies();
	PendingHits.Reset();
	RangeBone = NAME_None;
	bArms = false;
	Weight = 0.f;
	bRising = false;
	SetComponentTickEnabled(false);
}

void UAgHitPhysicsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bHitstop)
	{
		return;
	}
	for (const FAgPendingHit& Pending : PendingHits)
	{
		Start(Pending.Hit, Pending.bGuard);
	}
	PendingHits.Reset();
	if (RangeBone.IsNone())
	{
		SetComponentTickEnabled(false);
		return;
	}

	Weight = FMath::FInterpConstantTo(Weight, bRising ? MaxBlendWeight : 0.f, DeltaTime, MaxBlendWeight / (bRising ? BlendInTime : BlendOutTime));
	bRising = bRising && Weight < MaxBlendWeight;
	USkeletalMeshComponent* Mesh = GetMesh();
	Mesh->SetAllBodiesBelowPhysicsBlendWeight(RangeBone, Weight);
	if (!bArms)
	{
		for (const FName& Arm : ArmBones)
		{
			Mesh->SetAllBodiesBelowPhysicsBlendWeight(Arm, 0.f);
		}
	}
	if (!bRising && Weight <= 0.f)
	{
		Mesh->SetAllBodiesBelowSimulatePhysics(RangeBone, false);
		RangeBone = NAME_None;
		bArms = false;
		SetComponentTickEnabled(false);
	}
}

void UAgHitPhysicsComponent::BeginPlay()
{
	Super::BeginPlay();

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_Hitstop, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleHitstopChanged);
}

void UAgHitPhysicsComponent::Request(const FAgPhysicsHit& Hit, bool bGuard)
{
	// 피격 처리는 반응 뒤에 히트스톱을 걸므로, 다음 틱에 히트스톱이 끝났는지 보고 시작합니다.
	PendingHits.Add({Hit, bGuard});
	SetComponentTickEnabled(true);
}

void UAgHitPhysicsComponent::Start(const FAgPhysicsHit& Hit, bool bGuard)
{
	USkeletalMeshComponent* Mesh = GetMesh();
	const FName HitBone = bGuard ? NAME_None : FindHitBone(*Mesh, Hit);
	const FName NewRangeBone = bGuard ? GuardRangeBone : FindRangeBone(*Mesh, HitBone);
	// 물리 자세가 남은 바디를 키네마틱으로 되돌리면 애니메이션 자세로 튀므로, 흔드는 동안 범위와 팔은 더하기만 합니다.
	if (RangeBone.IsNone() || Mesh->BoneIsChildOf(RangeBone, NewRangeBone))
	{
		RangeBone = NewRangeBone;
	}
	bArms = bArms || bGuard;
	// 키네마틱 바디는 애니메이션 자세에 고정돼 관절로 이어진 가슴을 붙잡으므로, 팔도 물리로 돌리고 가드가 아니면
	// 섞는 비율만 0으로 두어 보이는 팔과 무기는 몸통을 따라가게 합니다.
	Mesh->SetAllBodiesBelowSimulatePhysics(RangeBone, true);
	// 맞은 바디 하나만 밀면 관절로 이어진 상체 전체가 힘을 나눠 받고 애니메이션에 고정된 바디가 힘을 받아 거의 움직이지
	// 않으므로, 범위 안 바디를 모두 같은 속도로 밀고 맞은 바디만 맞은 위치에서 밀어 그 자리를 중심으로 돌게 합니다.
	// 가드는 양손으로 칼을 쥐어 두 손을 함께 맞은 위치에서 밀어야 손이 손잡이에서 벌어지지 않습니다.
	const FBodyInstance* HitBody = bGuard ? nullptr : Mesh->GetBodyInstance(HitBone);
	const FBodyInstance* LeftHand = Mesh->GetBodyInstance(HandBones[0]);
	const FBodyInstance* RightHand = Mesh->GetBodyInstance(HandBones[1]);
	Mesh->ForEachBodyBelow(RangeBone, true, false, [this, &Hit, bGuard, HitBody, LeftHand, RightHand](FBodyInstance* Body)
	{
		if (bGuard ? (Body == LeftHand || Body == RightHand) : Body == HitBody)
		{
			PushAtHit(*Body, Hit, PushSpeed);
		}
		else
		{
			Body->AddImpulse(Hit.Direction * PushSpeed, true);
		}
	});
	bRising = true;
}

void UAgHitPhysicsComponent::HandleHitstopChanged(const FGameplayTag Tag, int32 NewCount)
{
	bHitstop = NewCount > 0;
	if (!bHitstop)
	{
		ResumeBodies();
		return;
	}
	if (RangeBone.IsNone())
	{
		return;
	}
	// 물리는 캐릭터별 시간 배율(히트스톱)을 따르지 않으므로, 움직이던 바디의 속도와 중력을 빼 모션과 함께 멈추고 끝나면
	// 되돌립니다. 재우면 엔진이 아주 느리게 움직이는 키네마틱 바디에 이어진 바디를 바로 깨워 중력으로 처집니다.
	GetMesh()->ForEachBodyBelow(RangeBone, true, false, [this](FBodyInstance* Body)
	{
		if (Body->IsInstanceSimulatingPhysics())
		{
			FrozenBodies.Add({Body->InstanceBodyIndex, Body->GetUnrealWorldVelocity(), Body->GetUnrealWorldAngularVelocityInRadians(), static_cast<bool>(Body->bEnableGravity)});
			Body->SetLinearVelocity(FVector::ZeroVector, false);
			Body->SetAngularVelocityInRadians(FVector::ZeroVector, false);
			Body->SetEnableGravity(false);
		}
	});
}

void UAgHitPhysicsComponent::ResumeBodies()
{
	USkeletalMeshComponent* Mesh = GetMesh();
	for (const FAgFrozenBody& Frozen : FrozenBodies)
	{
		if (FBodyInstance* Body = Mesh->Bodies.IsValidIndex(Frozen.BodyIndex) ? Mesh->Bodies[Frozen.BodyIndex] : nullptr)
		{
			Body->SetEnableGravity(Frozen.bGravity);
			Body->SetLinearVelocity(Frozen.LinearVelocity, false);
			Body->SetAngularVelocityInRadians(Frozen.AngularVelocity, false);
		}
	}
	FrozenBodies.Reset();
}

USkeletalMeshComponent* UAgHitPhysicsComponent::GetMesh() const
{
	return CastChecked<AAgCharacterBase>(GetOwner())->GetMesh();
}
