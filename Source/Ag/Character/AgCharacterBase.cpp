// Copyright Woogle. All Rights Reserved.

#include "Character/AgCharacterBase.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/AgEffect_Kill.h"
#include "AbilitySystem/AgEffect_TimedTag.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "Ag.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
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
	/** 래그돌용 엔진 충돌 프로필 (BaseEngine.ini) */
	const FName RagdollProfileName(TEXT("Ragdoll"));

	/** 히트스톱 중의 시간 배율: 모션, 몽타주, 이동이 거의 멈춥니다. */
	constexpr float HitstopTimeDilation = 1.e-4f;

	/** 움찔·가드 움찔 몽타주의 ID (캐릭터 데이터의 Montages) */
	const FName FlinchMontageId(TEXT("Flinch"));
	const FName GuardFlinchMontageId(TEXT("GuardFlinch"));

}

const FName AAgCharacterBase::WeaponTag(TEXT("Weapon"));
const FName AAgCharacterBase::WeaponCollisionTag(TEXT("WeaponCollision"));

AAgCharacterBase::AAgCharacterBase()
{
	// 이동과 물리 갱신 뒤 일반 회전을 처리합니다.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	AbilitySystemComponent = CreateDefaultSubobject<UAgAbilitySystemComponent>(TEXT("AbilitySystem"));
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
	return !AbilitySystemComponent->HasMatchingGameplayTag(AgGameplayTags::State_GroggyImmune) &&
	       !AbilitySystemComponent->HasMatchingGameplayTag(AgGameplayTags::State_Groggy);
}

void AAgCharacterBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	WeaponComponent = FindComponentByTag<USceneComponent>(WeaponTag);
}

void AAgCharacterBase::BeginPlay()
{
	// 처음 읽기에 실패한 데이터로는 전투를 시작하지 않습니다. 다시 읽기 실패는 기존 정상 값을 유지합니다.
	if (!CharacterData || !CharacterData->HasValidJsonData())
	{
		UE_LOG(LogAg, Fatal, TEXT("%s cannot start: character JSON data is not valid."), *GetName());
		return;
	}
	Super::BeginPlay();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UAgAttributeSet::GetMOVAttribute())
	    .AddUObject(this, &ThisClass::HandleMOVChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UAgAttributeSet::GetPPAttribute())
	    .AddUObject(this, &ThisClass::HandlePPChanged);
	AbilitySystemComponent->RegisterGameplayTagEvent(AgGameplayTags::State_Hitstop, EGameplayTagEventType::NewOrRemoved)
	    .AddUObject(this, &ThisClass::HandleHitstopChanged);
	InitializeAttributes();
	GrantAbilities();
	ApplyEffects();

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

	UpdateFacing(DeltaSeconds);
}

void AAgCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// 빙의할 때마다 ASC가 새 컨트롤러를 알도록 액터 정보를 갱신합니다.
	AbilitySystemComponent->RefreshAbilityActorInfo();
}

void AAgCharacterBase::InitializeAttributes()
{
	// 캐릭터 데이터는 캐릭터 블루프린트가 정하는 필수 데이터입니다. 여기서 한 번 확인하고, 다른 곳은 있다고 보고 씁니다.
	checkf(CharacterData, TEXT("%s has no character data. Set it on the character blueprint."), *GetName());

	const FAgAttributes& Attributes = CharacterData->Attributes;
	UAbilitySystemComponent& ASC = *AbilitySystemComponent;

	// 최대치를 먼저 넣습니다. 현재 값이 최대치에 맞춰 잘리기 때문입니다.
	ASC.SetNumericAttributeBase(UAgAttributeSet::GetMaxHPAttribute(), Attributes.MaxHP);
	ASC.SetNumericAttributeBase(UAgAttributeSet::GetMaxSPAttribute(), Attributes.MaxSP);
	ASC.SetNumericAttributeBase(UAgAttributeSet::GetMaxMPAttribute(), Attributes.MaxMP);
	ASC.SetNumericAttributeBase(UAgAttributeSet::GetMaxUPAttribute(), Attributes.MaxUP);
	ASC.SetNumericAttributeBase(UAgAttributeSet::GetMaxPPAttribute(), Attributes.MaxPP);

	ASC.SetNumericAttributeBase(UAgAttributeSet::GetHPAttribute(), Attributes.HP);
	ASC.SetNumericAttributeBase(UAgAttributeSet::GetSPAttribute(), Attributes.SP);
	ASC.SetNumericAttributeBase(UAgAttributeSet::GetMPAttribute(), Attributes.MP);
	ASC.SetNumericAttributeBase(UAgAttributeSet::GetUPAttribute(), Attributes.UP);
	ASC.SetNumericAttributeBase(UAgAttributeSet::GetPPAttribute(), Attributes.PP);
	ASC.SetNumericAttributeBase(UAgAttributeSet::GetATKAttribute(), Attributes.ATK);
	ASC.SetNumericAttributeBase(UAgAttributeSet::GetDEFAttribute(), Attributes.DEF);
	ASC.SetNumericAttributeBase(UAgAttributeSet::GetMOVAttribute(), Attributes.MOV);

	// 변경 델리게이트는 값이 바뀔 때만 불리므로, MOV는 여기서 한 번 적용합니다.
	UpdateMoveSpeed();
}

void AAgCharacterBase::GrantAbilities()
{
	for (const FAgAbilityData& Ability : CharacterData->Abilities)
	{
		FGameplayAbilitySpec Spec(UAgGameplayAbility::StaticClass());
		AbilityIds.Add(Spec.Handle, Ability.Id);
		Spec.GetDynamicSpecSourceTags() = Ability.Tags;
		for (const FGameplayTag& Tag : Ability.Activation.TriggerEvents)
		{
			FAbilityTriggerData& Trigger = Spec.DynamicAbilityTriggers.AddDefaulted_GetRef();
			Trigger.TriggerTag = Tag;
			Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
		}
		if (!AbilitySystemComponent->GiveAbility(Spec).IsValid())
		{
			AbilityIds.Remove(Spec.Handle);
		}
	}
}

FGameplayAbilitySpec* AAgCharacterBase::FindAbilitySpec(FName Id) const
{
	const FGameplayAbilitySpecHandle* Handle = AbilityIds.FindKey(Id);
	return Handle ? AbilitySystemComponent->FindAbilitySpecFromHandle(*Handle) : nullptr;
}

FName AAgCharacterBase::FindAbilityId(const FGameplayAbilitySpecHandle Handle) const
{
	return AbilityIds.FindRef(Handle);
}

const FAgAbilityData* AAgCharacterBase::FindAbility(const FGameplayAbilitySpecHandle Handle) const
{
	const FName Id = FindAbilityId(Handle);
	return Id.IsNone() ? nullptr : CharacterData->FindAbility(Id);
}

void AAgCharacterBase::HandleMOVChanged(const FOnAttributeChangeData& Data)
{
	UpdateMoveSpeed();
}

void AAgCharacterBase::UpdateMoveSpeed()
{
	GetCharacterMovement()->MaxWalkSpeed = AttributeSet->GetMOV() * GetMoveSpeedMultiplier();
}

void AAgCharacterBase::ApplyEffects()
{
	for (const TSubclassOf<UGameplayEffect>& Effect : CharacterData->Effects)
	{
		if (Effect)
		{
			AbilitySystemComponent->ApplyGameplayEffectToSelf(Effect.GetDefaultObject(), 1.f, AbilitySystemComponent->MakeEffectContext());
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
	UpdateTimeDilation();
}

void AAgCharacterBase::SetTimeCompensation(float Compensation)
{
	TimeCompensation = Compensation;
	UpdateTimeDilation();
}

void AAgCharacterBase::UpdateTimeDilation()
{
	const bool bHitstop = AbilitySystemComponent->HasMatchingGameplayTag(AgGameplayTags::State_Hitstop);
	CustomTimeDilation = (bHitstop ? HitstopTimeDilation : 1.f) * TimeCompensation;
}

void AAgCharacterBase::RestartRegenDelay(FActiveGameplayEffectHandle& Handle, const FGameplayTag& DelayTag, float Duration)
{
	if (Handle.IsValid())
	{
		AbilitySystemComponent->RemoveActiveGameplayEffect(Handle);
	}
	Handle = UAgEffect_TimedTag::Apply(AbilitySystemComponent, DelayTag, Duration);
}

void AAgCharacterBase::SetMotionConstraints(UObject& RequestOwner, const FAgCharacterMotionConstraints& Constraints)
{
	MotionConstraintOwner = &RequestOwner;
	MotionConstraints = Constraints;
}

void AAgCharacterBase::ClearMotionConstraints(const UObject& RequestOwner)
{
	if (MotionConstraintOwner.Get() == &RequestOwner)
	{
		MotionConstraintOwner.Reset();
		MotionConstraints = {};
	}
}

bool AAgCharacterBase::IsFacingFixed() const
{
	return MotionConstraintOwner.IsValid() && MotionConstraints.bControlsFacing;
}

void AAgCharacterBase::UpdateFacing(float DeltaSeconds)
{
	const AActor* Target = GetFacingTarget();
	if (IsDead() || !Target || IsFacingFixed())
	{
		return;
	}

	// 회전 속도는 캐릭터 무브먼트의 회전 속도(엔진 기본값)입니다.
	const FVector ToTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (!ToTarget.IsNearlyZero())
	{
		const FRotator Goal(0.f, ToTarget.Rotation().Yaw, 0.f);
		SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(), Goal, DeltaSeconds, GetCharacterMovement()->RotationRate.Yaw));
	}
}

FTransform AAgCharacterBase::ClampRootMotion(const FTransform& WorldRootMotion, UCharacterMovementComponent* Movement, float DeltaSeconds)
{
	if (!MotionConstraintOwner.IsValid())
	{
		return WorldRootMotion;
	}
	if (MotionConstraints.bTranslationFromVelocity)
	{
		// 회전은 워핑 결과를 쓰고, 공중 궤적은 물리 속도를 유지합니다. 착지 후 모션으로 밀려나지 않습니다.
		FTransform Result = WorldRootMotion;
		Result.SetTranslation(Movement->IsFalling() ? Movement->Velocity * DeltaSeconds : FVector::ZeroVector);
		return Result;
	}
	const AActor* Target = MotionConstraints.StopTarget.Get();
	if (!Target)
	{
		return WorldRootMotion;
	}

	// 이번 프레임의 루트 모션에서 정지 간격 안으로 들어가는 만큼을 잘라냅니다.
	FVector Delta = WorldRootMotion.GetTranslation();
	const FVector ToTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	const float Toward = FVector::DotProduct(FVector(Delta.X, Delta.Y, 0.f), ToTarget);
	const float Allowed = FMath::Max(0.f, GetHorizontalDistanceTo(Target) - MotionConstraints.StopDistance);
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
	UAnimMontage* Montage = CharacterData->FindMontage(FlinchMontageId);
	if (AnimInstance && Montage)
	{
		AnimInstance->Montage_Play(Montage);
	}
}

void AAgCharacterBase::PlayGuardFlinch()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	UAnimMontage* Montage = CharacterData->FindMontage(GuardFlinchMontageId);
	if (AnimInstance && Montage)
	{
		AnimInstance->Montage_Play(Montage);
	}
}

void AAgCharacterBase::Die(const FVector& PushVelocity, UGameplayAbility* DeathAbility, bool bRagdoll)
{
	if (IsDead())
	{
		return;
	}

	// 1-2. 모든 행동을 멈추고, 이후의 피격은 무시합니다(IsDead). 몽타주의 워핑과 열린 판정도 함께 정리합니다.
	AbilitySystemComponent->AddLooseGameplayTag(AgGameplayTags::State_Dead);
	AbilitySystemComponent->CancelAllAbilities(DeathAbility);
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && bRagdoll)
	{
		AnimInstance->StopAllMontages(0.f);
	}

	// 3. 입력(플레이어)과 이동을 멈춥니다. 보스의 AI는 사망 상태로 넘어가 이동 입력을 더 주지 않습니다. 처형 사망
	// 모션은 루트 모션으로 계속 움직입니다.
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		DisableInput(PlayerController);
	}
	GetCharacterMovement()->StopMovementImmediately();

	// 4. 어빌리티 취소가 각 판정 Task를 끝냈으므로 더는 상대 캐릭터를 막지 않습니다.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	// 5. 마지막 공격의 피격 방향으로 밀리며 래그돌이 됩니다.
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
