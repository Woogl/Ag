// Copyright Woogle. All Rights Reserved.

#include "Character/AgCharacterBase.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/AgEffect_Damage.h"
#include "AbilitySystem/AgEffect_Kill.h"
#include "AbilitySystem/AgEffect_Regen.h"
#include "AbilitySystem/AgEffect_TimedTag.h"
#include "AbilitySystemComponent.h"
#include "Ag.h"
#include "Animation/AgAnimNotifyState_AttackWindow.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Combat/AgCombatLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/AgAssetManager.h"
#include "Core/AgCollision.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "Data/AgCombatRules.h"
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

	/** Closer than this to the character's axis, the angle around it is unreliable, so a sub-step goes straight. */
	constexpr float MinArcRadius = 20.f;

	/** 히트스톱 중의 시간 배율: 모션, 몽타주, 이동이 거의 멈춥니다. */
	constexpr float HitstopTimeDilation = 1.e-4f;

	/**
	 * A point between A and B for a sub-step. Swings travel around the body, so this follows the arc around the
	 * character's vertical axis through Center rather than the straight chord: a sweep can turn a quarter turn in a frame.
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

const FName AAgCharacterBase::AttackWarpTarget(TEXT("AttackTarget"));
const FName AAgCharacterBase::WeaponTag(TEXT("Weapon"));
const FName AAgCharacterBase::WeaponCollisionTag(TEXT("WeaponCollision"));

AAgCharacterBase::AAgCharacterBase()
{
	// 물리 다음에 틱해서 무기 스윕이 이번 프레임의 포즈를 봅니다.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	AttributeSet = CreateDefaultSubobject<UAgAttributeSet>(TEXT("AttributeSet"));
	MotionWarping = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarping"));

	// 캐릭터는 카메라를 가리지 않습니다(카메라 '카메라 충돌').
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	// 무기 스윕은 메시의 피직스 바디에 맞습니다.
	GetMesh()->SetCollisionResponseToChannel(ECC_AgWeapon, ECR_Overlap);

	// 공격은 메시의 피직스 바디에 맞고 무기는 손을 따라가므로, 화면 밖에서도 포즈를 갱신합니다.
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
}

UAbilitySystemComponent* AAgCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

bool AAgCharacterBase::IsDead() const
{
	return AbilitySystemComponent->HasMatchingGameplayTag(AgGameplayTags::State_Dead);
}

bool AAgCharacterBase::CanLosePP() const
{
	return !CharacterData->Stats.bInfinitePP && !AbilitySystemComponent->HasMatchingGameplayTag(AgGameplayTags::State_Groggy);
}

void AAgCharacterBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	WeaponComponent = FindComponentByTag<USceneComponent>(WeaponTag);
	WeaponCollisionComponent = FindComponentByTag<UCapsuleComponent>(WeaponCollisionTag);
}

void AAgCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UAgAttributeSet::GetMOVAttribute()).AddUObject(this, &ThisClass::HandleMOVChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UAgAttributeSet::GetPPAttribute()).AddUObject(this, &ThisClass::HandlePPChanged);
	AbilitySystemComponent->RegisterGameplayTagEvent(AgGameplayTags::State_Hitstop, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::HandleHitstopChanged);
	InitializeStats();
	GrantAbilities();
	StartRegen();

	GetCharacterMovement()->ProcessRootMotionPostConvertToWorld.BindUObject(this, &ThisClass::ClampRootMotion);

	if (AAgBossStageGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgBossStageGameMode>())
	{
		GameMode->RegisterCombatant(this);
	}
}

void AAgCharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// ASC는 자기가 등록 해제될 때에야 어빌리티를 취소합니다. 그 전에 메시가 등록 해제되며 몽타주를 끊으면, 살아 있는
	// 어빌리티가 그 콜백에서 메시를 바꿉니다(처형 사망의 래그돌이 이미 지운 물리 상태를 다시 만듦).
	AbilitySystemComponent->CancelAllAbilities();

	Super::EndPlay(EndPlayReason);
}

void AAgCharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (OpenWindowHit != INDEX_NONE)
	{
		SweepWeapon();
	}
	UpdateFacing(DeltaSeconds);
}

void AAgCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// The player is possessed after BeginPlay; refresh so the ability system knows its controller.
	AbilitySystemComponent->RefreshAbilityActorInfo();
}

void AAgCharacterBase::InitializeStats()
{
	// 캐릭터 데이터는 캐릭터 블루프린트가 정하는 필수 데이터입니다. 여기서 한 번 확인하고, 다른 곳은 있다고 보고 씁니다.
	checkf(CharacterData, TEXT("%s has no character data. Set it on the character blueprint."), *GetName());

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
	UpdateMoveSpeed();
}

void AAgCharacterBase::GrantAbilities()
{
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
	UpdateMoveSpeed();
}

void AAgCharacterBase::UpdateMoveSpeed()
{
	GetCharacterMovement()->MaxWalkSpeed = AttributeSet->GetMOV() * GetMoveSpeedMultiplier();
}

void AAgCharacterBase::StartRegen()
{
	for (const TSubclassOf<UAgEffect_Regen>& Regen : CharacterData->RegenEffects)
	{
		if (Regen)
		{
			AbilitySystemComponent->ApplyGameplayEffectToSelf(Regen.GetDefaultObject(), 1.f, AbilitySystemComponent->MakeEffectContext());
		}
	}
}

void AAgCharacterBase::HandlePPChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue < Data.OldValue)
	{
		RestartRegenDelay(PPRegenDelay, AgGameplayTags::State_Regen_PPDelay, UAgAssetManager::Get().GetCombatRules().PPRegenDelay);
	}
}

void AAgCharacterBase::HandleHitstopChanged(const FGameplayTag Tag, int32 NewCount)
{
	// 멈춘 동안 다시 맞으면 태그가 하나 더 붙고, 마지막 것이 끝날 때 풀립니다.
	CustomTimeDilation = NewCount > 0 ? HitstopTimeDilation : 1.f;
}

void AAgCharacterBase::RestartRegenDelay(FActiveGameplayEffectHandle& Handle, const FGameplayTag& DelayTag, float Duration)
{
	if (Handle.IsValid())
	{
		AbilitySystemComponent->RemoveActiveGameplayEffect(Handle);
	}
	Handle = UAgEffect_TimedTag::Apply(AbilitySystemComponent, DelayTag, Duration);
}

bool AAgCharacterBase::GetWeaponCollision(FVector& OutStart, FVector& OutEnd) const
{
	if (!WeaponCollisionComponent)
	{
		return false;
	}
	// 캡슐의 축은 로컬 Z입니다.
	const FVector Center = WeaponCollisionComponent->GetComponentLocation();
	const FVector HalfSegment = WeaponCollisionComponent->GetUpVector() * WeaponCollisionComponent->GetScaledCapsuleHalfHeight_WithoutHemisphere();
	OutStart = Center + HalfSegment;
	OutEnd = Center - HalfSegment;
	return true;
}

void AAgCharacterBase::SetActiveAttack(TConstArrayView<FAgPlayerAttackHit> Hits)
{
	ClearActiveAttack();
	ActiveAttackHits = Hits;
}

void AAgCharacterBase::SetActiveAttack(TConstArrayView<FAgAttackHit> Hits)
{
	ClearActiveAttack();
	for (const FAgAttackHit& Hit : Hits)
	{
		ActiveAttackHits.Emplace(Hit);
	}
}

void AAgCharacterBase::ClearActiveAttack()
{
	OpenWindowHit = INDEX_NONE;
	WindowTouchedActors.Reset();
	ActiveAttackHits.Reset();
}

void AAgCharacterBase::BeginAttackWindow(int32 HitIndex)
{
	if (IsDead() || !ActiveAttackHits.IsValidIndex(HitIndex))
	{
		return;
	}

	OpenWindowHit = HitIndex;
	WindowTouchedActors.Reset();
	UE_LOG(LogAg, Verbose, TEXT("%s: attack window %d opens"), *GetName(), HitIndex + 1);
	if (GetWeaponCollision(LastEdgeStart, LastEdgeEnd))
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
	if (!GetWeaponCollision(EdgeStart, EdgeEnd))
	{
		return;
	}

	const float Travel = FMath::Max(FVector::Dist(EdgeStart, LastEdgeStart), FVector::Dist(EdgeEnd, LastEdgeEnd));
	const int32 Steps = FMath::Clamp(FMath::CeilToInt(Travel / SweepStepLength), 1, MaxSweepSteps);
	const FCollisionShape Edge = FCollisionShape::MakeSphere(WeaponCollisionComponent->GetScaledCapsuleRadius());
	const FCollisionQueryParams Params(SCENE_QUERY_STAT(AgWeaponSweep), /*bTraceComplex*/ false, this);
	const FVector Center = GetActorLocation();

	TArray<FHitResult> Touches;
	for (int32 Step = 1; Step <= Steps && OpenWindowHit != INDEX_NONE; ++Step)
	{
		const float Alpha = static_cast<float>(Step) / Steps;
		const FVector From = ArcLerp(Center, LastEdgeStart, EdgeStart, Alpha);
		const FVector To = ArcLerp(Center, LastEdgeEnd, EdgeEnd, Alpha);

		Touches.Reset();
		GetWorld()->SweepMultiByChannel(Touches, From, To, FQuat::Identity, ECC_AgWeapon, Edge, Params);
		for (const FHitResult& Touch : Touches)
		{
			AAgCharacterBase* Target = Cast<AAgCharacterBase>(Touch.GetActor());
			if (!Target || !UAgCombatLibrary::AreHostile(this, Target) || WindowTouchedActors.Contains(Target))
			{
				continue;
			}

			// 판정 구간 하나에서 대상마다 한 번만 닿습니다. 아무 일도 없었어도(무적) 다시 맞지 않습니다.
			WindowTouchedActors.Add(Target);
			const FVector HitLocation = Touch.bStartPenetrating ? Touch.Location : Touch.ImpactPoint;
			UAgEffect_Damage::ApplyAttack(this, this, Target, ActiveAttackHits[OpenWindowHit], HitLocation);
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

void AAgCharacterBase::SetFacingTarget(AActor* Target)
{
	FacingTarget = Target;
}

bool AAgCharacterBase::IsFacingFixed() const
{
	return OpenWindowHit != INDEX_NONE || IsInRotationStopWindow();
}

bool AAgCharacterBase::IsInRotationStopWindow() const
{
	const UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance)
	{
		return false;
	}

	// 판정 구간보다 회전 정지 시간만큼 앞서 멈추고, 판정 구간이 끝나면 다시 돕니다.
	const float LeadInMontage = CharacterData->GetRotationStopLeadInMontage();
	for (const FAnimMontageInstance* Instance : AnimInstance->MontageInstances)
	{
		if (!Instance || !Instance->IsActive())
		{
			continue;
		}
		const float Position = Instance->GetPosition();
		for (const FAnimNotifyEvent& Notify : Instance->Montage->Notifies)
		{
			if (Cast<UAgAnimNotifyState_AttackWindow>(Notify.NotifyStateClass)
				&& Position >= Notify.GetTriggerTime() - LeadInMontage && Position <= Notify.GetEndTriggerTime())
			{
				return true;
			}
		}
	}
	return false;
}

void AAgCharacterBase::UpdateFacing(float DeltaSeconds)
{
	const AActor* Target = GetFacingTarget();
	if (IsDead() || !Target || IsFacingFixed())
	{
		return;
	}

	// 회전 속도는 캐릭터 무브먼트의 회전 속도(엔진 기본값)입니다.
	const FVector ToTarget = UAgCombatLibrary::GetHorizontalDirection(this, Target);
	if (!ToTarget.IsNearlyZero())
	{
		const FRotator Goal(0.f, ToTarget.Rotation().Yaw, 0.f);
		SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(), Goal, DeltaSeconds, GetCharacterMovement()->RotationRate.Yaw));
	}
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

void AAgCharacterBase::PlayFlinch()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && CharacterData->FlinchMontage)
	{
		AnimInstance->Montage_Play(CharacterData->FlinchMontage);
	}
}

void AAgCharacterBase::PlayGuardFlinch()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && CharacterData->GuardFlinchMontage)
	{
		AnimInstance->Montage_Play(CharacterData->GuardFlinchMontage);
	}
}

void AAgCharacterBase::Die(const FVector& PushVelocity, UGameplayAbility* DeathAbility, bool bRagdoll)
{
	if (IsDead())
	{
		return;
	}

	// 1-2. 모든 행동을 멈추고, 이후의 피격은 무시합니다(IsDead). 행동의 공격 중 이동 태스크도 함께 끝납니다.
	AbilitySystemComponent->AddLooseGameplayTag(AgGameplayTags::State_Dead);
	AbilitySystemComponent->CancelAllAbilities(DeathAbility);
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && bRagdoll)
	{
		AnimInstance->StopAllMontages(0.f);
	}

	// 3. 입력(플레이어)과 이동을 멈춥니다. 보스의 AI는 사망 상태로 넘어가 이동 입력을 더 주지 않습니다. 사망 모션은 루트 모션으로
	// 계속 움직입니다.
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		DisableInput(PlayerController);
	}
	GetCharacterMovement()->StopMovementImmediately();

	// 4. 이 캐릭터의 판정 구간을 없앱니다.
	ClearActiveAttack();

	// 5. 더는 상대 캐릭터를 막지 않습니다.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	// 6. 마지막 공격이 온 쪽으로 밀리며 래그돌이 됩니다.
	if (bRagdoll)
	{
		StartRagdoll(PushVelocity);
	}

	OnDied.Broadcast(this);
}

void AAgCharacterBase::StartRagdoll(const FVector& PushVelocity)
{
	GetCharacterMovement()->DisableMovement();

	USkeletalMeshComponent* MeshComponent = GetMesh();
	MeshComponent->SetCollisionProfileName(RagdollProfileName);
	MeshComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	MeshComponent->SetSimulatePhysics(true);
	if (!PushVelocity.IsNearlyZero())
	{
		MeshComponent->AddImpulse(PushVelocity, NAME_None, /*bVelChange*/ true);
	}
}

void AAgCharacterBase::Kill()
{
	if (IsDead())
	{
		return;
	}

	AbilitySystemComponent->ApplyGameplayEffectToSelf(GetDefault<UAgEffect_Kill>(), 1.f, AbilitySystemComponent->MakeEffectContext());
}

void AAgCharacterBase::FellOutOfWorld(const UDamageType& DamageType)
{
	Kill();
}
