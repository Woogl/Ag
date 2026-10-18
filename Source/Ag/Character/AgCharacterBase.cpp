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
#include "Combat/AgTimeSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/AgAssetManager.h"
#include "Core/AgCollision.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "Data/AgCombatRules.h"
#include "Data/AgEffectData.h"
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

	/** 사망할 때 공격 쪽에서 밀려나는 속도(cm/s) */
	constexpr float DeathPushSpeed = 300.f;

	/** 움찔·가드 움찔 몽타주의 ID (캐릭터 데이터의 Montages) */
	const FName FlinchMontageId(TEXT("Flinch"));
	const FName GuardFlinchMontageId(TEXT("GuardFlinch"));

}

const FName AAgCharacterBase::WeaponTag(TEXT("Weapon"));
const FName AAgCharacterBase::WeaponCollisionTag(TEXT("WeaponCollision"));

AAgCharacterBase::AAgCharacterBase()
{
	// 이동과 물리 갱신 뒤 자동 회전과 이동 사건을 처리합니다.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	AbilitySystemComponent = CreateDefaultSubobject<UAgAbilitySystemComponent>(TEXT("AbilitySystem"));
	AttributeSet = CreateDefaultSubobject<UAgAttributeSet>(TEXT("AttributeSet"));
	MotionWarping = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarping"));
	CharacterDataClass = UAgCharacterData::StaticClass();

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

TArray<FString> AAgCharacterBase::GetCharacterDataIds() const
{
	TArray<FString> Options;
	for (FName Id : UAgAssetManager::Get().GetCharacterDataIds(CharacterDataClass))
	{
		Options.Add(Id.ToString());
	}
	return Options;
}

void AAgCharacterBase::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	// 보스 AI 컨트롤러는 컴포넌트 초기화(APawn::PostInitializeComponents) 안에서 빙의하며 보스 데이터를 읽으므로, 그 전에 연결합니다.
	CharacterData = UAgAssetManager::Get().FindCharacterData(CharacterDataId);
	// 처음 읽기에 실패한 데이터로는 전투를 시작하지 않습니다. 다시 읽기 실패는 기존 정상 값을 유지합니다.
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
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UAgAttributeSet::GetPPAttribute())
	    .AddUObject(this, &ThisClass::HandlePPChanged);
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
	UpdateFacing(DeltaSeconds);
}

void AAgCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// 빙의할 때마다 ASC가 새 컨트롤러를 알도록 액터 정보를 갱신합니다.
	AbilitySystemComponent->RefreshAbilityActorInfo();
}

void AAgCharacterBase::SendEventToSelf(const FGameplayTag& EventTag)
{
	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.Instigator = this;
	Payload.Target = this;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, EventTag, Payload);
}

void AAgCharacterBase::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	// 띄우는 동안의 착지 가운데 Launch가 착지로 보지 않은 것(체공 시간의 앞부분)은 다시 떠오르므로 알리지 않습니다. 착지로 본
	// Launch는 엔진 착지 알림(Super::Landed) 안에서 소스를 이미 지웠습니다.
	if (!IsDead() && !GetCharacterMovement()->GetRootMotionSource(UAgAbilityTask_Launch::SourceName).IsValid())
	{
		SendEventToSelf(AgGameplayTags::Event_Landed);
	}
}

void AAgCharacterBase::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	if (HasActorBegunPlay())
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(AgGameplayTags::State_Airborne, GetCharacterMovement()->IsFalling() ? 1 : 0);
	}
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

void AAgCharacterBase::GrantAbilities()
{
	for (FName AbilityId : CharacterData->Abilities)
	{
		// 어빌리티 정의는 데이터를 읽을 때 캐릭터의 목록과 함께 검사했습니다. 엔진은 부여할 때 스펙의 트리거를 등록합니다.
		const FAgAbilityData& Ability = *UAgAssetManager::Get().FindAbility(AbilityId);
		FGameplayAbilitySpec Spec(UAgGameplayAbility::StaticClass());
		AbilityIds.Add(Spec.Handle, Ability.Id);
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
	for (FName EffectId : CharacterData->Effects)
	{
		AbilitySystemComponent->ApplyGameplayEffectToSelf(
		    UAgAssetManager::Get().FindEffect(EffectId), 1.f, AbilitySystemComponent->MakeEffectContext());
	}
}

void AAgCharacterBase::HandlePPChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue < Data.OldValue)
	{
		RestartRegenDelay(PPRegenDelay, EAgEffect::PPRegenDelay);
	}
}

void AAgCharacterBase::HandleHitstopChanged(const FGameplayTag Tag, int32 NewCount)
{
	// 멈춘 동안 다시 맞으면 태그가 하나 더 붙고, 마지막 것이 끝날 때 풀립니다.
	UpdateTimeDilation();
}

void AAgCharacterBase::HandlePassThroughChanged(const FGameplayTag Tag, int32 NewCount)
{
	// 태그는 겹쳐도 개수로 세므로 마지막 것이 빠질 때 되돌립니다. 죽은 캐릭터는 캐릭터 간 충돌을 끈 채로 둡니다.
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

void AAgCharacterBase::RestartRegenDelay(FActiveGameplayEffectHandle& Handle, EAgEffect DelayEffect)
{
	if (Handle.IsValid())
	{
		AbilitySystemComponent->RemoveActiveGameplayEffect(Handle);
	}
	Handle = AbilitySystemComponent->ApplyGameplayEffectToSelf(
	    UAgAssetManager::Get().GetEffect(DelayEffect), 1.f, AbilitySystemComponent->MakeEffectContext());
}

bool AAgCharacterBase::IsFacingFixed() const
{
	// 행동 중의 몸 돌리기는 그 행동의 Face와 추적 구간(Track)이 맡습니다.
	const UGameplayAbility* Animating = AbilitySystemComponent->GetAnimatingAbility();
	return (Animating && Animating->IsActive()) || AbilitySystemComponent->HasMatchingGameplayTag(AgGameplayTags::State_NonCombat);
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
	const UAgGameplayAbility* Runner = Cast<UAgGameplayAbility>(AbilitySystemComponent->GetAnimatingAbility());
	const AActor* Target = Runner ? Runner->GetTargetingTarget() : nullptr;
	if (!Target)
	{
		return WorldRootMotion;
	}

	// 이번 프레임의 루트 모션에서 그 행동의 간격 안으로 들어가는 만큼을 잘라냅니다.
	FVector Delta = WorldRootMotion.GetTranslation();
	const FVector ToTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	const float Toward = FVector::DotProduct(FVector(Delta.X, Delta.Y, 0.f), ToTarget);
	const float Allowed = FMath::Max(0.f, GetHorizontalDistanceTo(Target) - Runner->GetStopDistance());
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

void AAgCharacterBase::HandleDeathEvent(const FGameplayEventData* Payload)
{
	if (IsDead() || bEndingPlay)
	{
		return;
	}

	// 사망 모션: 다른 행동을 취소하기 전에 재생 위치를 보관하고, 그 위치에서 사망 모션을 이어 재생합니다(처형 사망).
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

	// 마지막 공격의 피격 방향으로 밀립니다.
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

void AAgCharacterBase::Die(const FVector& PushVelocity, bool bRagdoll)
{
	if (IsDead())
	{
		return;
	}

	// 1-2. 모든 행동을 멈추고, 이후의 피격은 무시합니다(IsDead). 몽타주의 워핑과 열린 판정도 함께 정리합니다.
	AbilitySystemComponent->AddLooseGameplayTag(AgGameplayTags::State_Dead);
	AbilitySystemComponent->CancelAllAbilities();
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && bRagdoll)
	{
		AnimInstance->StopAllMontages(0.f);
	}

	// 3. 입력(플레이어)과 이동을 멈춥니다. 보스의 AI는 사망 상태로 넘어가 이동 입력을 더 주지 않습니다. 사망 모션은
	// 루트 모션으로 계속 움직입니다.
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

	AbilitySystemComponent->ApplyGameplayEffectToSelf(
	    UAgAssetManager::Get().GetEffect(EAgEffect::Kill), 1.f, AbilitySystemComponent->MakeEffectContext());
}

void AAgCharacterBase::FellOutOfWorld(const UDamageType& DamageType)
{
	Kill();
}
