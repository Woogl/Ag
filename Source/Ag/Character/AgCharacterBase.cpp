// Copyright Woogle. All Rights Reserved.

#include "Character/AgCharacterBase.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Ag.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/Skeleton.h"
#include "Character/AgHitPhysicsComponent.h"
#include "Character/AgCharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/AgAssetManager.h"
#include "Core/AgCollision.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "Data/AgEffectData.h"
#include "Game/AgGameMode.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "KismetAnimationLibrary.h"
#include "MotionWarpingComponent.h"

namespace AgCharacterBasePrivate
{
	const FName RagdollProfileName(TEXT("Ragdoll"));

	constexpr float HitstopTimeDilation = 1.e-4f;

	constexpr float DeathPushSpeed = 300.f;

	const FName WeaponTag(TEXT("Weapon"));
}

AAgCharacterBase::AAgCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UAgCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	AbilitySystemComponent = CreateDefaultSubobject<UAgAbilitySystemComponent>(TEXT("AbilitySystem"));
	AttributeSet = CreateDefaultSubobject<UAgAttributeSet>(TEXT("AttributeSet"));
	MotionWarping = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarping"));
	HitPhysics = CreateDefaultSubobject<UAgHitPhysicsComponent>(TEXT("HitPhysics"));

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	GetMesh()->SetCollisionResponseToChannel(ECC_AgWeapon, ECR_Overlap);

	// 엔진은 메시 충돌에 물리가 켜져 있을 때만 피격 흔들림의 물리 자세를 섞습니다.
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// 공격은 메시의 피직스 바디에 맞고 무기는 손을 따라가므로, 화면 밖에서도 포즈를 갱신합니다.
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
}

UAgAbilitySystemComponent* AAgCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

FGenericTeamId AAgCharacterBase::GetGenericTeamId() const
{
	// 편집 중인 월드의 캐릭터는 데이터를 연결하지 않습니다.
	return CharacterData ? FGenericTeamId(static_cast<uint8>(CharacterData->Team)) : FGenericTeamId::NoTeam;
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

bool AAgCharacterBase::CanBeHitBy(const AActor& Attacker) const
{
	return !IsDead() && FGenericTeamId::GetAttitude(&Attacker, this) != ETeamAttitude::Friendly;
}

AAgCharacterBase* AAgCharacterBase::GetLockOnTarget() const
{
	const FGameplayAbilitySpec* Spec = AbilitySystemComponent->FindSpecByTag(AgGameplayTags::Ability_LockOn);
	const UAgGameplayAbility* Instance = Spec ? Cast<UAgGameplayAbility>(Spec->GetPrimaryInstance()) : nullptr;
	return Instance && Instance->IsActive() ? Cast<AAgCharacterBase>(Instance->GetActionTarget()) : nullptr;
}

bool AAgCharacterBase::IsFacingFixed() const
{
	// 엔진 GetCurrentMontage는 블렌드 아웃을 시작한 몽타주를 돌려주지 않아, 행동이 끝나기 전 블렌드 아웃부터 돕니다.
	const UGameplayAbility* Animating = AbilitySystemComponent->GetAnimatingAbility();
	const UAnimMontage* Montage = AbilitySystemComponent->GetCurrentMontage();
	// 상체 슬롯만 쓰는 몽타주는 이동하면서 재생하므로 방향을 묶지 않습니다.
	// 피격 반응 뒤의 그로기 자세는 몽타주가 아니라 애님 그래프가 재생합니다.
	return (Animating && Animating->IsActive() && Montage && Montage->IsValidSlot(FAnimSlotGroup::DefaultSlotName)) ||
	       AbilitySystemComponent->HasMatchingGameplayTag(AgGameplayTags::State_Groggy) || IsDead();
}

bool AAgCharacterBase::IsBehind(const FVector& Location) const
{
	return FMath::Abs(UKismetAnimationLibrary::CalculateDirection(Location - GetActorLocation(), GetActorRotation())) >= 135.f;
}

void AAgCharacterBase::Kill()
{
	AbilitySystemComponent->ApplyGameplayEffectToSelf(UAgAssetManager::Get().GetEffect(EAgEffect::Kill), 1.f,
		AbilitySystemComponent->MakeEffectContext());
}

void AAgCharacterBase::SetTimeCompensation(float Compensation)
{
	TimeCompensation = Compensation;
	UpdateTimeDilation();
}

void AAgCharacterBase::PushDisableMontageRootMotion()
{
	UAnimInstance* Anim = GetMesh()->GetAnimInstance();
	if (++MontageRootMotionDisableCount == 1 && Anim)
	{
		Anim->OnMontageStarted.AddDynamic(this, &ThisClass::HandleMontageStarted);
		DisableRootMotionMontage();
	}
}

void AAgCharacterBase::PopDisableMontageRootMotion()
{
	if (MontageRootMotionDisableCount == 0 || --MontageRootMotionDisableCount > 0)
	{
		return;
	}
	if (UAnimInstance* Anim = GetMesh()->GetAnimInstance())
	{
		Anim->OnMontageStarted.RemoveDynamic(this, &ThisClass::HandleMontageStarted);
		if (FAnimMontageInstance* Instance = RootMotionDisabledMontageId != INDEX_NONE ? Anim->GetMontageInstanceForID(RootMotionDisabledMontageId) : nullptr)
		{
			Instance->PopDisableRootMotion();
		}
	}
	RootMotionDisabledMontageId = INDEX_NONE;
}

void AAgCharacterBase::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	// 편집 중인 월드(에디터, 블루프린트 미리 보기)에서는 데이터를 쓰는 플레이 코드가 돌지 않으므로 연결하지 않습니다.
	// 데이터를 아직 적지 않은 캐릭터를 놓거나 띄워도 에디터가 멈추지 않습니다.
	if (!GetWorld()->IsGameWorld())
	{
		return;
	}

	// 적 AI 컨트롤러는 컴포넌트 초기화(APawn::PostInitializeComponents) 안에서 빙의하며 적 데이터를 읽으므로,
	// 그 전에 연결합니다.
	CharacterData = UAgAssetManager::Get().FindCharacterData(CharacterDataId);
	if (!CharacterData)
	{
		// 엔진은 여기서 지운 액터의 컴포넌트 초기화와 BeginPlay를 건너뛰므로, 데이터가 있다고 보는 코드에 닿지 않습니다.
		UE_LOG(LogAg, Error, TEXT("%s was removed: character data '%s' is missing."), *GetName(), *CharacterDataId.ToString());
		Destroy();
	}
}

void AAgCharacterBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	WeaponComponent = FindComponentByTag<USceneComponent>(AgCharacterBasePrivate::WeaponTag);

	// 몽타주 타임라인은 몽타주가 나아가고 포즈가 갱신된 뒤에 판정해야 무기 판정 구간의 자세와 위치가 맞습니다.
	// 메시는 포즈 평가를 물리 뒤까지 미뤄 끝내기도 하므로(tick.AnimationDelaysEndGroup), TG_DuringPhysics에 틱하는
	// ASC가 그 전에 판정하지 않게 묶습니다.
	AbilitySystemComponent->PrimaryComponentTick.AddPrerequisite(GetMesh(), GetMesh()->PrimaryComponentTick);
}

void AAgCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UAgAttributeSet::GetMOVAttribute())
		.AddUObject(this, &ThisClass::HandleMOVChanged);
	AbilitySystemComponent->RegisterGameplayTagEvent(AgGameplayTags::State_Hitstop, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleHitstopChanged);
	AbilitySystemComponent->RegisterGameplayTagEvent(AgGameplayTags::State_PassThrough, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandlePassThroughChanged);
	AbilitySystemComponent->RegisterGameplayTagEvent(AgGameplayTags::State_Hidden, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleHiddenChanged);
	AbilitySystemComponent->RegisterGameplayTagEvent(AgGameplayTags::State_HitReaction_KnockUp, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleWeaponCapsuleTagChanged);
	AbilitySystemComponent->RegisterGameplayTagEvent(AgGameplayTags::State_HitReaction_KnockDown, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleWeaponCapsuleTagChanged);
	DeathEventHandle = AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_Death)
	                       .AddUObject(this, &ThisClass::HandleDeathEvent);
	PawnResponse = GetCapsuleComponent()->GetCollisionResponseToChannel(ECC_Pawn);
	AbilitySystemComponent->SetLooseGameplayTagCount(AgGameplayTags::State_Airborne, GetCharacterMovement()->IsFalling() ? 1 : 0);
	InitializeAttributes();
	GrantAbilities();
	ApplyEffects();

	GetCharacterMovement()->ProcessRootMotionPostConvertToWorld.BindUObject(this, &ThisClass::ClampRootMotion);
}

void AAgCharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_Death).Remove(DeathEventHandle);
	// ASC는 자기가 등록 해제될 때에야 어빌리티를 취소합니다. 그 전에 메시가 등록 해제되며 몽타주를 끊으면 살아 있는
	// 어빌리티의 콜백이 그때 메시를 바꾸므로(래그돌이 이미 지운 물리 상태를 다시 만듦) 먼저 취소합니다.
	AbilitySystemComponent->CancelAllAbilities();

	Super::EndPlay(EndPlayReason);
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

	const FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeEffectSpec(EAgEffect::FallDamage);
	Spec.Data->SetSetByCallerMagnitude(AgGameplayTags::SetByCaller_FallHeight,
		CastChecked<UAgCharacterMovementComponent>(GetCharacterMovement())->GetFallHeight(Hit));
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data);

	if (!IsDead())
	{
		SendEventToSelf(AgGameplayTags::Event_Landed);
	}
}

void AAgCharacterBase::LaunchCharacter(FVector LaunchVelocity, bool bXYOverride, bool bZOverride)
{
	Super::LaunchCharacter(LaunchVelocity, bXYOverride, bZOverride);

	// 엔진은 정점을 알리면 이 값을 끄므로, 공중에서 다시 띄울 때도 정점을 알리도록 띄울 때마다 켭니다.
	GetCharacterMovement()->bNotifyApex = true;
}

void AAgCharacterBase::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	// 엔진은 정점을 알린 뒤에만 이 값을 끄므로, 정점 전에 착지한 뒤 다음 낙하에서 잘못 알리지 않도록 착지하면 끕니다.
	if (!GetCharacterMovement()->IsFalling())
	{
		GetCharacterMovement()->bNotifyApex = false;
	}
	if (HasActorBegunPlay())
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(AgGameplayTags::State_Airborne, GetCharacterMovement()->IsFalling() ? 1 : 0);
	}
}

void AAgCharacterBase::NotifyJumpApex()
{
	Super::NotifyJumpApex();

	if (!IsDead())
	{
		SendEventToSelf(AgGameplayTags::Event_Apex);
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

void AAgCharacterBase::ApplyDeathState()
{
	AbilitySystemComponent->AddLooseGameplayTag(AgGameplayTags::State_Dead);
	AbilitySystemComponent->CancelAllAbilities();

	// 사망 모션은 루트 모션으로 계속 움직이므로 이동은 끄지 않고 멈추기만 합니다. 적 AI는 사망 상태로 넘어가
	// 이동 입력을 더 주지 않습니다.
	GetCharacterMovement()->StopMovementImmediately();

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
}

void AAgCharacterBase::NotifyGameModeOfDeath()
{
	if (AAgGameMode* GameMode = GetWorld()->GetAuthGameMode<AAgGameMode>())
	{
		GameMode->HandleCharacterDied(*this);
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

void AAgCharacterBase::DieWithRagdoll(const FVector& PushVelocity)
{
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->StopAllMontages(0.f);
	}

	ApplyDeathState();
	StartRagdoll(PushVelocity);
	NotifyGameModeOfDeath();
}

void AAgCharacterBase::StartRagdoll(const FVector& PushVelocity)
{
	GetCharacterMovement()->DisableMovement();
	// 피격 흔들림이 바디의 물리 비율을 다시 쓰면 래그돌이 켠 물리 블렌드가 꺼지므로, 래그돌 전에 멈춥니다.
	HitPhysics->Stop();

	USkeletalMeshComponent* MeshComponent = GetMesh();
	MeshComponent->SetCollisionProfileName(AgCharacterBasePrivate::RagdollProfileName);
	MeshComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	MeshComponent->SetSimulatePhysics(true);
	if (!PushVelocity.IsNearlyZero())
	{
		MeshComponent->AddImpulse(PushVelocity, NAME_None, true);
	}
}

void AAgCharacterBase::Die(const AActor* Attacker)
{
	const FVector Push =
		Attacker ? (GetActorLocation() - Attacker->GetActorLocation()).GetSafeNormal2D() * AgCharacterBasePrivate::DeathPushSpeed : FVector::ZeroVector;
	DieWithRagdoll(Push);
}

void AAgCharacterBase::HandleDeathEvent(const FGameplayEventData* Payload)
{
	// 죽은 캐릭터의 HP를 다시 0으로 쓰는 효과(테스트용 HP 설정)도 사망 이벤트를 보냅니다.
	if (!IsDead())
	{
		Die(Payload ? Payload->Instigator.Get() : nullptr);
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
		Spec.GetDynamicSpecSourceTags() = Ability.AbilityTags;
		Spec.DynamicAbilityTriggers = Ability.AbilityTriggers;
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

TArray<FString> AAgCharacterBase::GetCharacterDataIds() const
{
	TArray<FString> Options;
	for (FName Id : UAgAssetManager::Get().GetCharacterDataIds())
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
	ASC.SetNumericAttributeBase(UAgAttributeSet::GetMaxMPAttribute(), Attributes.MaxMP);
	ASC.SetNumericAttributeBase(UAgAttributeSet::GetMaxUPAttribute(), Attributes.MaxUP);
	ASC.SetNumericAttributeBase(UAgAttributeSet::GetMaxPPAttribute(), Attributes.MaxPP);
	ASC.SetNumericAttributeBase(UAgAttributeSet::GetMaxPotionAttribute(), Attributes.MaxPotion);

	ASC.SetNumericAttributeBase(UAgAttributeSet::GetHPAttribute(), Attributes.HP);
	ASC.SetNumericAttributeBase(UAgAttributeSet::GetMPAttribute(), Attributes.MP);
	ASC.SetNumericAttributeBase(UAgAttributeSet::GetUPAttribute(), Attributes.UP);
	ASC.SetNumericAttributeBase(UAgAttributeSet::GetPPAttribute(), Attributes.PP);
	ASC.SetNumericAttributeBase(UAgAttributeSet::GetPotionAttribute(), Attributes.Potion);
	ASC.SetNumericAttributeBase(UAgAttributeSet::GetATKAttribute(), Attributes.ATK);
	ASC.SetNumericAttributeBase(UAgAttributeSet::GetDEFAttribute(), Attributes.DEF);
	ASC.SetNumericAttributeBase(UAgAttributeSet::GetMOVAttribute(), Attributes.MOV);

	// 변경 델리게이트는 값이 바뀔 때만 불리므로, MOV는 여기서 한 번 적용합니다.
	GetCharacterMovement()->MaxWalkSpeed = AttributeSet->GetMOV();
}

void AAgCharacterBase::HandleMOVChanged(const FOnAttributeChangeData& Data)
{
	GetCharacterMovement()->MaxWalkSpeed = Data.NewValue;
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

void AAgCharacterBase::HandleHiddenChanged(const FGameplayTag Tag, int32 NewCount)
{
	SetActorHiddenInGame(NewCount > 0);
}

void AAgCharacterBase::HandleWeaponCapsuleTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	// 넉업·넉다운 동안은 공중 피격 모션이 몸을 뒤집고 저글이 높이를 올리거나 몸이 바닥에 붙어 무기가 피직스 바디를
	// 비껴가므로, 캡슐도 무기에 맞습니다. 누워 있는 플레이어의 무적은 대미지 실행이 지킵니다.
	const bool bKnocked = AbilitySystemComponent->HasMatchingGameplayTag(AgGameplayTags::State_HitReaction_KnockUp) ||
	                      AbilitySystemComponent->HasMatchingGameplayTag(AgGameplayTags::State_HitReaction_KnockDown);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_AgWeapon, bKnocked ? ECR_Overlap : ECR_Ignore);
}

void AAgCharacterBase::UpdateTimeDilation()
{
	const bool bHitstop = AbilitySystemComponent->HasMatchingGameplayTag(AgGameplayTags::State_Hitstop);
	CustomTimeDilation = (bHitstop ? AgCharacterBasePrivate::HitstopTimeDilation : 1.f) * TimeCompensation;
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
	const float Allowed = FMath::Max(0.f, GetHorizontalDistanceTo(Target) - Ability->GetTargetSpacing());
	if (Toward <= Allowed)
	{
		return WorldRootMotion;
	}

	Delta -= ToTarget * (Toward - Allowed);
	FTransform Clamped = WorldRootMotion;
	Clamped.SetTranslation(Delta);
	return Clamped;
}

void AAgCharacterBase::DisableRootMotionMontage()
{
	UAnimInstance* Anim = GetMesh()->GetAnimInstance();
	FAnimMontageInstance* Instance = Anim ? Anim->GetRootMotionMontageInstance() : nullptr;
	if (!Instance || Instance->GetInstanceID() == RootMotionDisabledMontageId)
	{
		return;
	}
	if (FAnimMontageInstance* Previous = RootMotionDisabledMontageId != INDEX_NONE ? Anim->GetMontageInstanceForID(RootMotionDisabledMontageId) : nullptr)
	{
		Previous->PopDisableRootMotion();
	}
	Instance->PushDisableRootMotion();
	RootMotionDisabledMontageId = Instance->GetInstanceID();
}

void AAgCharacterBase::HandleMontageStarted(UAnimMontage* Montage)
{
	DisableRootMotionMontage();
}
