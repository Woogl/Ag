// Copyright Woogle. All Rights Reserved.

#include "Character/AgCharacterBase.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Ag.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/AgCharacterData.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Game/AgBossStageGameMode.h"

namespace
{
	/** Engine collision profile for simulating skeletal meshes (BaseEngine.ini). */
	const FName RagdollProfileName(TEXT("Ragdoll"));
}

AAgCharacterBase::AAgCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	AttributeSet = CreateDefaultSubobject<UAgAttributeSet>(TEXT("AttributeSet"));

	StaticWeapon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticWeapon"));
	StaticWeapon->SetupAttachment(GetMesh());
	StaticWeapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SkeletalWeapon = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalWeapon"));
	SkeletalWeapon->SetupAttachment(GetMesh());
	SkeletalWeapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Characters never block the camera (카메라 '카메라 충돌').
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

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

	if (AAgBossStageGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgBossStageGameMode>())
	{
		GameMode->RegisterCombatant(this);
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

void AAgCharacterBase::Die(const FVector& HitDirection)
{
	if (bDead)
	{
		return;
	}
	bDead = true;

	// Stop input (player) and movement.
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		DisableInput(PlayerController);
	}
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->StopMovementImmediately();
	Movement->DisableMovement();

	// Don't block the other character any more.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	// Ragdoll, pushed the way the last attack came from.
	USkeletalMeshComponent* MeshComponent = GetMesh();
	MeshComponent->SetCollisionProfileName(RagdollProfileName);
	MeshComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	MeshComponent->SetSimulatePhysics(true);
	if (!HitDirection.IsNearlyZero())
	{
		MeshComponent->AddImpulse(HitDirection, NAME_None, /*bVelChange*/ true);
	}

	OnDied.Broadcast(this);
}

void AAgCharacterBase::FellOutOfWorld(const UDamageType& DamageType)
{
	Die(FVector::ZeroVector);
}
