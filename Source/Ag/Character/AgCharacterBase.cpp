// Copyright Woogle. All Rights Reserved.

#include "Character/AgCharacterBase.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystem/Tasks/AgAbilityTask_Launch.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "Ag.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/AgHitPhysicsComponent.h"
#include "Character/AgCharacterMovementComponent.h"
#include "Combat/AgTimeSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/AgAssetManager.h"
#include "Core/AgCollision.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "Data/AgEffectData.h"
#include "Game/AgBossStageGameMode.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/RootMotionSource.h"
#include "MotionWarpingComponent.h"

namespace
{
	const FName RagdollProfileName(TEXT("Ragdoll"));

	constexpr float HitstopTimeDilation = 1.e-4f;

	constexpr float DeathPushSpeed = 300.f;
}

const FName AAgCharacterBase::WeaponTag(TEXT("Weapon"));

AAgCharacterBase::AAgCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UAgCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// 이동과 물리 갱신 뒤 이동 사건을 처리합니다.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	AbilitySystemComponent = CreateDefaultSubobject<UAgAbilitySystemComponent>(TEXT("AbilitySystem"));
	AttributeSet = CreateDefaultSubobject<UAgAttributeSet>(TEXT("AttributeSet"));
	MotionWarping = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarping"));
	HitPhysics = CreateDefaultSubobject<UAgHitPhysicsComponent>(TEXT("HitPhysics"));
	CharacterDataClass = UAgCharacterData::StaticClass();

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	GetMesh()->SetCollisionResponseToChannel(ECC_AgWeapon, ECR_Overlap);

	// 엔진은 메시 충돌에 물리가 켜져 있을 때만 피격 흔들림의 물리 자세를 섞습니다.
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// 공격은 메시의 피직스 바디에 맞고 무기는 손을 따라가므로, 화면 밖에서도 포즈를 갱신합니다.
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
}

UAbilitySystemComponent* AAgCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
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

bool AAgCharacterBase::IsDead() const
{
	return AbilitySystemComponent->HasMatchingGameplayTag(AgGameplayTags::State_Dead);
}

bool AAgCharacterBase::CanLosePP() const
{
	return !AbilitySystemComponent->HasMatchingGameplayTag(AgGameplayTags::State_GroggyImmune) &&
	       !AbilitySystemComponent->HasMatchingGameplayTag(AgGameplayTags::State_Groggy) &&
	       !AbilitySystemComponent->HasMatchingGameplayTag(AgGameplayTags::State_Execution) &&
	       !AbilitySystemComponent->HasMatchingGameplayTag(AgGameplayTags::State_Invincible);
}

AAgCharacterBase* AAgCharacterBase::GetLockOnTarget() const
{
	const UAgAbilitySystemComponent* ASC = Cast<UAgAbilitySystemComponent>(AbilitySystemComponent);
	const FGameplayAbilitySpec* Spec = ASC ? ASC->FindSpecByTag(AgGameplayTags::Ability_LockOn) : nullptr;
	const UAgGameplayAbility* Instance = Spec ? Cast<UAgGameplayAbility>(Spec->GetPrimaryInstance()) : nullptr;
	return Instance ? Cast<AAgCharacterBase>(Instance->GetActionTarget()) : nullptr;
}

bool AAgCharacterBase::IsFacingFixed() const
{
	const UGameplayAbility* Animating = AbilitySystemComponent->GetAnimatingAbility();
	return (Animating && Animating->IsActive()) || AbilitySystemComponent->HasMatchingGameplayTag(AgGameplayTags::State_NonCombat);
}

void AAgCharacterBase::Kill()
{
	if (IsDead())
	{
		return;
	}

	AbilitySystemComponent->ApplyGameplayEffectToSelf(UAgAssetManager::Get().GetEffect(EAgEffect::Kill), 1.f,
		AbilitySystemComponent->MakeEffectContext());
}

void AAgCharacterBase::SetTimeCompensation(float Compensation)
{
	TimeCompensation = Compensation;
	UpdateTimeDilation();
}

void AAgCharacterBase::NotifyLanded()
{
	// 엔진은 소스를 즉시 지우지 않고 제거 표시만 하므로, 종료된 Launch는 없는 것으로 봅니다.
	const TSharedPtr<FRootMotionSource> LaunchSource = GetCharacterMovement()->GetRootMotionSource(UAgAbilityTask_Launch::SourceName);
	const bool bLaunching = LaunchSource.IsValid() && !LaunchSource->Status.HasFlag(ERootMotionSourceStatusFlags::MarkedForRemoval);
	if (!IsDead() && !bLaunching)
	{
		SendEventToSelf(AgGameplayTags::Event_Landed);
	}
}

void AAgCharacterBase::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	// 보스 AI 컨트롤러는 컴포넌트 초기화(APawn::PostInitializeComponents) 안에서 빙의하며 보스 데이터를 읽으므로,
	// 그 전에 연결합니다.
	CharacterData = UAgAssetManager::Get().FindCharacterData(CharacterDataId);
	// 다시 읽기에 실패해도 기존 정상 값을 유지하므로, 여기서 멈추는 것은 처음 읽기에 실패한 경우뿐입니다.
	if (!CharacterData || !CharacterData->IsA(CharacterDataClass) || !CharacterData->HasValidJsonData())
	{
		UE_LOG(LogAg, Fatal, TEXT("%s cannot start: character data '%s' is missing, has the wrong type or is not valid."), *GetName(),
			*CharacterDataId.ToString());
	}
}

void AAgCharacterBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	WeaponComponent = FindComponentByTag<USceneComponent>(WeaponTag);
}

void AAgCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UAgAttributeSet::GetMOVAttribute())
		.AddUObject(this, &ThisClass::HandleMOVChanged);
	AbilitySystemComponent->RegisterGameplayTagEvent(AgGameplayTags::State_Hitstop, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleHitstopChanged);
	AbilitySystemComponent->RegisterGameplayTagEvent(AgGameplayTags::State_PassThrough, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandlePassThroughChanged);
	AbilitySystemComponent->RegisterGameplayTagEvent(AgGameplayTags::State_TimeStop, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleTimeStopChanged);
	DeathEventHandle = AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_Death)
	                       .AddUObject(this, &ThisClass::HandleDeathEvent);
	PawnResponse = GetCapsuleComponent()->GetCollisionResponseToChannel(ECC_Pawn);
	AbilitySystemComponent->SetLooseGameplayTagCount(AgGameplayTags::State_Airborne, GetCharacterMovement()->IsFalling() ? 1 : 0);
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
	// 어빌리티나 사망 모션의 콜백이 그때 메시를 바꿉니다(처형 사망의 래그돌이 이미 지운 물리 상태를 다시 만듦).
	bEndingPlay = true;
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (FAnimMontageInstance* DeathMotion = AnimInstance ? AnimInstance->GetMontageInstanceForID(DeathMotionInstanceId) : nullptr)
	{
		DeathMotion->OnMontageBlendingOutStarted.Unbind();
	}
	AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_Death).Remove(DeathEventHandle);
	AbilitySystemComponent->CancelAllAbilities();

	Super::EndPlay(EndPlayReason);
}

void AAgCharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateApex();
}

void AAgCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// 빙의할 때마다 ASC가 새 컨트롤러를 알도록 액터 정보를 갱신합니다.
	AbilitySystemComponent->RefreshAbilityActorInfo();
}

void AAgCharacterBase::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	NotifyLanded();
}

void AAgCharacterBase::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	if (HasActorBegunPlay())
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(AgGameplayTags::State_Airborne, GetCharacterMovement()->IsFalling() ? 1 : 0);
	}
}

void AAgCharacterBase::MoveBlockedBy(const FHitResult& Impact)
{
	Super::MoveBlockedBy(Impact);

	if (!IsDead())
	{
		SendEventToSelf(AgGameplayTags::Event_MoveBlocked);
	}
}

void AAgCharacterBase::FellOutOfWorld(const UDamageType& DamageType)
{
	Kill();
}

void AAgCharacterBase::UpdateMoveSpeed()
{
	GetCharacterMovement()->MaxWalkSpeed = AttributeSet->GetMOV() * GetMoveSpeedMultiplier();
}

void AAgCharacterBase::Die(const FVector& PushVelocity, bool bRagdoll)
{
	if (IsDead())
	{
		return;
	}

	AbilitySystemComponent->AddLooseGameplayTag(AgGameplayTags::State_Dead);
	AbilitySystemComponent->CancelAllAbilities();
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && bRagdoll)
	{
		AnimInstance->StopAllMontages(0.f);
	}

	// 사망 모션은 루트 모션으로 계속 움직이므로 이동은 끄지 않고 멈추기만 합니다. 보스 AI는 사망 상태로 넘어가
	// 이동 입력을 더 주지 않습니다.
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		DisableInput(PlayerController);
	}
	GetCharacterMovement()->StopMovementImmediately();

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	if (bRagdoll)
	{
		StartRagdoll(PushVelocity);
	}

	OnDied.Broadcast(this);
}

void AAgCharacterBase::StartRagdoll(const FVector& PushVelocity)
{
	GetCharacterMovement()->DisableMovement();
	// 피격 흔들림이 바디의 물리 비율을 다시 쓰면 래그돌이 켠 물리 블렌드가 꺼지므로, 래그돌 전에 멈춥니다.
	HitPhysics->Stop();

	USkeletalMeshComponent* MeshComponent = GetMesh();
	MeshComponent->SetCollisionProfileName(RagdollProfileName);
	MeshComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	MeshComponent->SetSimulatePhysics(true);
	if (!PushVelocity.IsNearlyZero())
	{
		MeshComponent->AddImpulse(PushVelocity, NAME_None, true);
	}
}

void AAgCharacterBase::GrantAbilities()
{
	for (FName AbilityId : CharacterData->Abilities)
	{
		// 캐릭터의 어빌리티 ID는 데이터를 읽을 때 검사했습니다. 엔진은 부여할 때 스펙의 트리거를 등록합니다.
		const FAgAbilityData& Ability = *UAgAssetManager::Get().FindAbility(AbilityId);
		FGameplayAbilitySpec Spec(UAgGameplayAbility::StaticClass());
		// GiveAbility 안의 OnGiveAbility가 이 ID로 정의를 찾으므로 먼저 연결합니다.
		AbilityIds.Add(Spec.Handle, AbilityId);
		Spec.GetDynamicSpecSourceTags() = Ability.Tags;
		for (const FAgAbilityTrigger& Trigger : Ability.Activation.Triggers)
		{
			FAbilityTriggerData& Data = Spec.DynamicAbilityTriggers.AddDefaulted_GetRef();
			Data.TriggerTag = Trigger.Tag;
			Data.TriggerSource = Trigger.Source == EAgTriggerSource::OwnedTagPresent ? EGameplayAbilityTriggerSource::OwnedTagPresent
			                                                                         : EGameplayAbilityTriggerSource::GameplayEvent;
		}
		if (!AbilitySystemComponent->GiveAbility(Spec).IsValid())
		{
			AbilityIds.Remove(Spec.Handle);
		}
	}
}

void AAgCharacterBase::ApplyEffects()
{
	for (FName EffectId : CharacterData->Effects)
	{
		AbilitySystemComponent->ApplyGameplayEffectToSelf(UAgAssetManager::Get().FindEffect(EffectId), 1.f,
			AbilitySystemComponent->MakeEffectContext());
	}
}

void AAgCharacterBase::SendEventToSelf(const FGameplayTag& EventTag)
{
	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.Instigator = this;
	Payload.Target = this;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, EventTag, Payload);
}

TArray<FString> AAgCharacterBase::GetCharacterDataIds() const
{
	TArray<FString> Options;
	for (FName Id : UAgAssetManager::Get().GetCharacterDataIds(CharacterDataClass))
	{
		Options.Add(Id.ToString());
	}
	return Options;
}

void AAgCharacterBase::InitializeAttributes()
{
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

void AAgCharacterBase::HandleMOVChanged(const FOnAttributeChangeData& Data)
{
	UpdateMoveSpeed();
}

void AAgCharacterBase::HandleHitstopChanged(const FGameplayTag Tag, int32 NewCount)
{
	UpdateTimeDilation();
}

void AAgCharacterBase::HandlePassThroughChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (!IsDead())
	{
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, NewCount > 0 ? ECR_Ignore : PawnResponse.GetValue());
	}
}

void AAgCharacterBase::HandleTimeStopChanged(const FGameplayTag Tag, int32 NewCount)
{
	UAgTimeSubsystem* TimeSubsystem = GetWorld()->GetSubsystem<UAgTimeSubsystem>();
	if (NewCount > 0)
	{
		TimeSubsystem->StartTimeStop(*this, *this);
	}
	else
	{
		TimeSubsystem->EndTimeStop(*this);
	}
}

void AAgCharacterBase::HandleDeathEvent(const FGameplayEventData* Payload)
{
	if (IsDead() || bEndingPlay)
	{
		return;
	}

	// 행동을 취소하면 재생 중인 몽타주가 멈추므로, 재생 위치는 Die 전에 읽습니다.
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	for (const FAgDeathMotion& Motion : CharacterData->DeathMotions)
	{
		UAnimMontage* Playing = CharacterData->FindMontage(Motion.Montage);
		UAnimMontage* Death = CharacterData->FindMontage(Motion.Death);
		if (!AnimInstance || !Playing || !Death || !Payload || !Payload->InstigatorTags.HasTagExact(Motion.KilledBy) ||
			!AnimInstance->Montage_IsPlaying(Playing))
		{
			continue;
		}
		const float Position = AnimInstance->Montage_GetPosition(Playing);
		Die(FVector::ZeroVector, false);
		FAnimMontageInstance* Instance = AnimInstance->Montage_Play(Death, 1.f, EMontagePlayReturnType::MontageLength, Position) > 0.f
		                                     ? AnimInstance->GetActiveInstanceForMontage(Death)
		                                     : nullptr;
		if (!Instance)
		{
			StartRagdoll(FVector::ZeroVector);
			return;
		}
		DeathMotionInstanceId = Instance->GetInstanceID();
		Instance->OnMontageBlendingOutStarted = FOnMontageBlendingOutStarted::CreateUObject(this, &ThisClass::HandleDeathMotionBlendingOut);
		return;
	}

	const AActor* Attacker = Payload ? Payload->Instigator.Get() : nullptr;
	const FVector Push =
		Attacker ? (GetActorLocation() - Attacker->GetActorLocation()).GetSafeNormal2D() * DeathPushSpeed : FVector::ZeroVector;
	Die(Push);
}

void AAgCharacterBase::HandleDeathMotionBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	DeathMotionInstanceId = INDEX_NONE;
	if (!bEndingPlay)
	{
		StartRagdoll(FVector::ZeroVector);
	}
}

void AAgCharacterBase::UpdateTimeDilation()
{
	const bool bHitstop = AbilitySystemComponent->HasMatchingGameplayTag(AgGameplayTags::State_Hitstop);
	CustomTimeDilation = (bHitstop ? HitstopTimeDilation : 1.f) * TimeCompensation;
}

FTransform AAgCharacterBase::ClampRootMotion(const FTransform& WorldRootMotion, UCharacterMovementComponent* Movement, float DeltaSeconds)
{
	const UAgGameplayAbility* Ability = Cast<UAgGameplayAbility>(AbilitySystemComponent->GetAnimatingAbility());
	const AActor* Target = Ability ? Ability->GetTargetingTarget() : nullptr;
	if (!Target)
	{
		return WorldRootMotion;
	}

	FVector Delta = WorldRootMotion.GetTranslation();
	const FVector ToTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	const float Toward = FVector::DotProduct(FVector(Delta.X, Delta.Y, 0.f), ToTarget);
	const float Allowed = FMath::Max(0.f, GetHorizontalDistanceTo(Target) - Ability->GetStopDistance());
	if (Toward <= Allowed)
	{
		return WorldRootMotion;
	}

	Delta -= ToTarget * (Toward - Allowed);
	FTransform Clamped = WorldRootMotion;
	Clamped.SetTranslation(Delta);
	return Clamped;
}

void AAgCharacterBase::UpdateApex()
{
	// 루트 모션 점프(넉업)도 캐릭터 무브먼트의 속도가 경로를 따릅니다.
	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	const float VelocityZ = Movement->IsFalling() ? static_cast<float>(Movement->Velocity.Z) : 0.f;
	if (PreviousVelocityZ > 0.f && VelocityZ <= 0.f && Movement->IsFalling() && !IsDead())
	{
		SendEventToSelf(AgGameplayTags::Event_Apex);
	}
	PreviousVelocityZ = VelocityZ;
}
