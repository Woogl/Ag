// Copyright Woogle. All Rights Reserved.

#include "Character/AgCharacterBase.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Ag.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Combat/AgCombatLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/AgCollision.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Game/AgBossStageGameMode.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "MotionWarpingComponent.h"

namespace
{
	/** Engine collision profile for simulating skeletal meshes (BaseEngine.ini). */
	const FName RagdollProfileName(TEXT("Ragdoll"));

	/** The weapon edge is swept in steps no longer than this, so a fast swing can't skip over a body. */
	constexpr float SweepStepLength = 10.f;
	constexpr int32 MaxSweepSteps = 16;
}

const FName AAgCharacterBase::AttackWarpTarget(TEXT("AttackTarget"));

AAgCharacterBase::AAgCharacterBase()
{
	// Ticks after physics so weapon sweeps see this frame's pose.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	AttributeSet = CreateDefaultSubobject<UAgAttributeSet>(TEXT("AttributeSet"));
	MotionWarping = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarping"));

	StaticWeapon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticWeapon"));
	StaticWeapon->SetupAttachment(GetMesh());
	StaticWeapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SkeletalWeapon = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalWeapon"));
	SkeletalWeapon->SetupAttachment(GetMesh());
	SkeletalWeapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Characters never block the camera (카메라 '카메라 충돌').
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	// Weapon sweeps hit the mesh's physics bodies.
	GetMesh()->SetCollisionResponseToChannel(ECC_AgWeapon, ECR_Overlap);

	// Attacks hit the mesh's physics bodies and the weapon follows the hand, so the pose must update even off screen.
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
}

UAbilitySystemComponent* AAgCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AAgCharacterBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyWeapon();
}

void AAgCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UAgAttributeSet::GetMOVAttribute()).AddUObject(this, &ThisClass::HandleMOVChanged);
	InitializeStats();
	GrantAbilities();
	if (CharacterData && CharacterData->bAlwaysSuperArmor)
	{
		AbilitySystemComponent->AddLooseGameplayTag(AgGameplayTags::State_SuperArmor);
	}

	GetCharacterMovement()->ProcessRootMotionPostConvertToWorld.BindUObject(this, &ThisClass::ClampRootMotion);

	if (AAgBossStageGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgBossStageGameMode>())
	{
		GameMode->RegisterCombatant(this);
	}
}

void AAgCharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (OpenWindowHit != INDEX_NONE)
	{
		SweepWeapon();
	}
	if (ApproachTarget.IsValid())
	{
		UpdateApproach();
	}
}

void AAgCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// The player is possessed after BeginPlay; refresh so the ability system knows its controller.
	AbilitySystemComponent->RefreshAbilityActorInfo();
}

void AAgCharacterBase::InitializeStats()
{
	if (!CharacterData)
	{
		UE_LOG(LogAg, Error, TEXT("%s has no character data; its stats stay at 0."), *GetName());
		return;
	}

	const FAgStats& Stats = CharacterData->Stats;
	UAbilitySystemComponent& ASC = *AbilitySystemComponent;

	// Maximums first: the current values are clamped to them.
	ASC.SetNumericAttributeBase(UAgAttributeSet::GetMaxHPAttribute(), Stats.MaxHP);
	ASC.SetNumericAttributeBase(UAgAttributeSet::GetMaxSPAttribute(), Stats.MaxSP);
	ASC.SetNumericAttributeBase(UAgAttributeSet::GetMaxMPAttribute(), Stats.MaxMP);
	ASC.SetNumericAttributeBase(UAgAttributeSet::GetMaxUPAttribute(), Stats.MaxUP);
	ASC.SetNumericAttributeBase(UAgAttributeSet::GetMaxPPAttribute(), Stats.bInfinitePP ? 0.f : Stats.MaxPP);

	ASC.SetNumericAttributeBase(UAgAttributeSet::GetHPAttribute(), Stats.HP);
	ASC.SetNumericAttributeBase(UAgAttributeSet::GetSPAttribute(), Stats.SP);
	ASC.SetNumericAttributeBase(UAgAttributeSet::GetMPAttribute(), Stats.MP);
	ASC.SetNumericAttributeBase(UAgAttributeSet::GetUPAttribute(), Stats.UP);
	ASC.SetNumericAttributeBase(UAgAttributeSet::GetPPAttribute(), Stats.bInfinitePP ? 0.f : Stats.PP);
	ASC.SetNumericAttributeBase(UAgAttributeSet::GetATKAttribute(), Stats.ATK);
	ASC.SetNumericAttributeBase(UAgAttributeSet::GetDEFAttribute(), Stats.DEF);
	ASC.SetNumericAttributeBase(UAgAttributeSet::GetMOVAttribute(), Stats.MOV);

	// The change delegate only fires when the value changes, so apply MOV once here.
	GetCharacterMovement()->MaxWalkSpeed = AttributeSet->GetMOV();
}

void AAgCharacterBase::GrantAbilities()
{
	if (!CharacterData)
	{
		return;
	}
	for (const TSubclassOf<UGameplayAbility>& Ability : CharacterData->Abilities)
	{
		if (Ability)
		{
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(Ability));
		}
	}
}

void AAgCharacterBase::HandleMOVChanged(const FOnAttributeChangeData& Data)
{
	GetCharacterMovement()->MaxWalkSpeed = Data.NewValue;
}

void AAgCharacterBase::ApplyWeapon()
{
	UStreamableRenderAsset* WeaponMesh = CharacterData ? CharacterData->WeaponMesh.Get() : nullptr;
	const FName Socket = CharacterData ? CharacterData->WeaponSocket : NAME_None;
	const FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepRelative, false);

	StaticWeapon->SetStaticMesh(Cast<UStaticMesh>(WeaponMesh));
	StaticWeapon->AttachToComponent(GetMesh(), AttachRules, Socket);

	SkeletalWeapon->SetSkeletalMeshAsset(Cast<USkeletalMesh>(WeaponMesh));
	SkeletalWeapon->AttachToComponent(GetMesh(), AttachRules, Socket);
}

USceneComponent* AAgCharacterBase::GetWeaponComponent() const
{
	if (StaticWeapon->GetStaticMesh())
	{
		return StaticWeapon;
	}
	return SkeletalWeapon->GetSkeletalMeshAsset() ? SkeletalWeapon.Get() : nullptr;
}

bool AAgCharacterBase::GetWeaponEdge(FVector& OutStart, FVector& OutEnd) const
{
	const USceneComponent* Weapon = GetWeaponComponent();
	if (!Weapon || !CharacterData)
	{
		return false;
	}
	const FTransform& WeaponTransform = Weapon->GetComponentTransform();
	OutStart = WeaponTransform.TransformPosition(CharacterData->WeaponEdgeStart);
	OutEnd = WeaponTransform.TransformPosition(CharacterData->WeaponEdgeEnd);
	return true;
}

void AAgCharacterBase::SetActiveAttack(TConstArrayView<FAgAttackHit> Hits)
{
	ClearActiveAttack();
	ActiveAttackHits = Hits;
}

void AAgCharacterBase::ClearActiveAttack()
{
	OpenWindowHit = INDEX_NONE;
	WindowTouchedActors.Reset();
	ActiveAttackHits.Reset();
}

void AAgCharacterBase::BeginAttackWindow(int32 HitIndex)
{
	if (bDead || !ActiveAttackHits.IsValidIndex(HitIndex))
	{
		return;
	}

	// The facing and approach target are fixed once the attack window starts (플레이어 사양 '공격 중 이동').
	StopApproach();

	OpenWindowHit = HitIndex;
	WindowTouchedActors.Reset();
	UE_LOG(LogAg, Verbose, TEXT("%s: attack window %d opens"), *GetName(), HitIndex + 1);
	if (GetWeaponEdge(LastEdgeStart, LastEdgeEnd))
	{
		SweepWeapon();
	}
}

void AAgCharacterBase::EndAttackWindow(int32 HitIndex)
{
	if (OpenWindowHit == HitIndex)
	{
		OpenWindowHit = INDEX_NONE;
		WindowTouchedActors.Reset();
	}
}

void AAgCharacterBase::SweepWeapon()
{
	FVector EdgeStart, EdgeEnd;
	if (!GetWeaponEdge(EdgeStart, EdgeEnd))
	{
		return;
	}

	const float Travel = FMath::Max(FVector::Dist(EdgeStart, LastEdgeStart), FVector::Dist(EdgeEnd, LastEdgeEnd));
	const int32 Steps = FMath::Clamp(FMath::CeilToInt(Travel / SweepStepLength), 1, MaxSweepSteps);
	const FCollisionShape Edge = FCollisionShape::MakeSphere(CharacterData->WeaponEdgeRadius);
	const FCollisionQueryParams Params(SCENE_QUERY_STAT(AgWeaponSweep), /*bTraceComplex*/ false, this);

	TArray<FHitResult> Touches;
	for (int32 Step = 1; Step <= Steps && OpenWindowHit != INDEX_NONE; ++Step)
	{
		const float Alpha = static_cast<float>(Step) / Steps;
		const FVector From = FMath::Lerp(LastEdgeStart, EdgeStart, Alpha);
		const FVector To = FMath::Lerp(LastEdgeEnd, EdgeEnd, Alpha);

		Touches.Reset();
		GetWorld()->SweepMultiByChannel(Touches, From, To, FQuat::Identity, ECC_AgWeapon, Edge, Params);
		for (const FHitResult& Touch : Touches)
		{
			AAgCharacterBase* Target = Cast<AAgCharacterBase>(Touch.GetActor());
			if (!Target || !UAgCombatLibrary::AreHostile(this, Target) || WindowTouchedActors.Contains(Target))
			{
				continue;
			}

			// One touch per target per window, even when it does nothing (an invincible target stays missed).
			WindowTouchedActors.Add(Target);
			const FVector HitLocation = Touch.bStartPenetrating ? Touch.Location : Touch.ImpactPoint;
			UAgCombatLibrary::ProcessHit(this, Target, ActiveAttackHits[OpenWindowHit], HitLocation);
			if (OpenWindowHit == INDEX_NONE)
			{
				break;
			}
		}
	}

	LastEdgeStart = EdgeStart;
	LastEdgeEnd = EdgeEnd;
}

void AAgCharacterBase::SetStopTarget(AActor* Target, float InStopDistance)
{
	StopTarget = Target;
	StopDistance = InStopDistance;
}

void AAgCharacterBase::ClearStopTarget()
{
	StopTarget.Reset();
}

FTransform AAgCharacterBase::ClampRootMotion(const FTransform& WorldRootMotion, UCharacterMovementComponent* Movement, float DeltaSeconds)
{
	const AActor* Target = StopTarget.Get();
	if (!Target)
	{
		return WorldRootMotion;
	}

	// Remove the part of this frame's root motion that would bring the character inside the stop distance.
	FVector Delta = WorldRootMotion.GetTranslation();
	const FVector ToTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	const float Toward = FVector::DotProduct(FVector(Delta.X, Delta.Y, 0.f), ToTarget);
	const float Allowed = FMath::Max(0.f, UAgCombatLibrary::GetHorizontalDistance(this, Target) - StopDistance);
	if (Toward <= Allowed)
	{
		return WorldRootMotion;
	}

	Delta -= ToTarget * (Toward - Allowed);
	FTransform Clamped = WorldRootMotion;
	Clamped.SetTranslation(Delta);
	return Clamped;
}

void AAgCharacterBase::StartApproach(AActor* Target, float InStopDistance, float MaxRange, float MaxTravel, bool bFaceTarget)
{
	ApproachTarget = Target;
	ApproachOrigin = GetActorLocation();
	ApproachStopDistance = InStopDistance;
	ApproachMaxRange = MaxRange;
	ApproachMaxTravel = MaxTravel;
	bApproachFacesTarget = bFaceTarget;
	UpdateApproach();
}

void AAgCharacterBase::StopApproach()
{
	ApproachTarget.Reset();
}

void AAgCharacterBase::UpdateApproach()
{
	const AActor* Target = ApproachTarget.Get();
	const float Gap = UAgCombatLibrary::GetHorizontalDistance(this, Target);
	if (!Target || (ApproachMaxRange > 0.f && Gap > ApproachMaxRange))
	{
		// Out of range: stop following and let the motion carry on in its last direction.
		StopApproach();
		MotionWarping->RemoveWarpTarget(AttackWarpTarget);
		return;
	}

	const FVector Location = GetActorLocation();
	const FVector Direction = (Target->GetActorLocation() - Location).GetSafeNormal2D();
	FVector Goal = Location;
	if (Gap > ApproachStopDistance)
	{
		Goal = Target->GetActorLocation() - Direction * ApproachStopDistance;
		Goal.Z = Location.Z;
	}
	if (ApproachMaxTravel > 0.f)
	{
		const FVector Offset = (Goal - ApproachOrigin).GetClampedToMaxSize2D(ApproachMaxTravel);
		Goal = FVector(ApproachOrigin.X + Offset.X, ApproachOrigin.Y + Offset.Y, Location.Z);
	}

	const FRotator Facing = bApproachFacesTarget && !Direction.IsNearlyZero() ? Direction.Rotation() : GetActorRotation();
	MotionWarping->AddOrUpdateWarpTargetFromLocationAndRotation(AttackWarpTarget, Goal, Facing);
}

void AAgCharacterBase::PlayFlinch()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && CharacterData && CharacterData->FlinchMontage)
	{
		AnimInstance->Montage_Play(CharacterData->FlinchMontage);
	}
}

void AAgCharacterBase::Die(const FVector& PushVelocity, UGameplayAbility* DeathAbility)
{
	if (bDead)
	{
		return;
	}
	bDead = true;

	// 1-2. Stop every action; later hits do nothing (IsDead).
	AbilitySystemComponent->AddLooseGameplayTag(AgGameplayTags::State_Dead);
	AbilitySystemComponent->CancelAllAbilities(DeathAbility);
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->StopAllMontages(0.f);
	}

	// 3. Stop input (player) or AI (boss), and movement.
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		DisableInput(PlayerController);
	}
	else if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->StopMovement();
	}
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->StopMovementImmediately();
	Movement->DisableMovement();

	// 4. Remove this character's attack windows.
	ClearActiveAttack();
	ClearStopTarget();
	StopApproach();

	// 5. Don't block the other character any more.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	// 6. Ragdoll, pushed the way the last attack came from.
	USkeletalMeshComponent* MeshComponent = GetMesh();
	MeshComponent->SetCollisionProfileName(RagdollProfileName);
	MeshComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	MeshComponent->SetSimulatePhysics(true);
	if (!PushVelocity.IsNearlyZero())
	{
		MeshComponent->AddImpulse(PushVelocity, NAME_None, /*bVelChange*/ true);
	}

	OnDied.Broadcast(this);
}

void AAgCharacterBase::Kill()
{
	if (bDead)
	{
		return;
	}

	AbilitySystemComponent->SetNumericAttributeBase(UAgAttributeSet::GetHPAttribute(), 0.f);

	FGameplayEventData Payload;
	Payload.EventTag = AgGameplayTags::Event_Death;
	Payload.Target = this;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, AgGameplayTags::Event_Death, Payload);

	// Without a death ability (no character data), die right away.
	if (!bDead)
	{
		Die(FVector::ZeroVector);
	}
}

void AAgCharacterBase::FellOutOfWorld(const UDamageType& DamageType)
{
	Kill();
}
