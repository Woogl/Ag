// Copyright Woogle. All Rights Reserved.

#include "Character/AgEnemyCharacter.h"

#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AI/AgEnemyAIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Core/AgAssetManager.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "Data/AgEffectData.h"
#include "GameFramework/CharacterMovementComponent.h"

#if WITH_EDITORONLY_DATA
#include "Character/AgEnemySightPreviewComponent.h"
#endif

AAgEnemyCharacter::AAgEnemyCharacter()
{
	CharacterDataClass = UAgEnemyData::StaticClass();
	Team = EAgTeam::Enemy;
	AIControllerClass = AAgEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;

#if WITH_EDITORONLY_DATA
	SightPreview = CreateEditorOnlyDefaultSubobject<UAgEnemySightPreviewComponent>(TEXT("SightPreview"));
#endif
}

bool AAgEnemyCharacter::CanLosePP() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	return ASC && !ASC->HasMatchingGameplayTag(AgGameplayTags::State_Groggy) &&
	       !ASC->HasMatchingGameplayTag(AgGameplayTags::State_Execution) &&
	       !ASC->HasMatchingGameplayTag(AgGameplayTags::State_Invincible);
}

int32 AAgEnemyCharacter::GetReachedPhase() const
{
	const TArray<float>& TransitionHPRatios = CastChecked<UAgEnemyData>(GetCharacterData())->PhaseTransitionHPRatios;
	const UAgAttributeSet* Stats = GetAttributeSet();
	for (int32 Index = TransitionHPRatios.Num() - 1; Index >= 0; --Index)
	{
		if (Stats->GetHP() <= Stats->GetMaxHP() * TransitionHPRatios[Index])
		{
			return Index + 2;
		}
	}
	return 1;
}

bool AAgEnemyCharacter::IsPhaseTransitionPending() const
{
	return GetReachedPhase() > StartedTransitionPhase;
}

bool AAgEnemyCharacter::StartPhaseTransition()
{
	const int32 TargetPhase = GetReachedPhase();
	SendEventToSelf(AgGameplayTags::Event_PhaseTransition);
	if (!IsPhaseTransitionActive())
	{
		return false;
	}
	StartedTransitionPhase = TargetPhase;
	return true;
}

bool AAgEnemyCharacter::IsPhaseTransitionActive() const
{
	const FGameplayAbilitySpec* Spec = GetAbilitySystemComponent()->FindSpecByTag(AgGameplayTags::Ability_Reaction_PhaseTransition);
	return Spec && Spec->IsActive();
}

bool AAgEnemyCharacter::CanActivatePattern(FName Id) const
{
	const FGameplayAbilitySpec* Spec = FindAbilitySpec(Id);
	const UGameplayAbility* Ability = Spec ? Spec->GetPrimaryInstance() : nullptr;
	return Ability && Ability->CanActivateAbility(Spec->Handle, GetAbilitySystemComponent()->AbilityActorInfo.Get());
}

void AAgEnemyCharacter::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	// AI 컨트롤러는 컴포넌트 초기화 안에서 빙의하며 StateTree를 시작하므로, 그 전에 처음 자리를 기록합니다.
	HomeTransform = GetActorTransform();
}

void AAgEnemyCharacter::GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const
{
	// 엔진 시야는 눈 위치와 컨트롤러 방향으로 보므로, 기획서의 기준인 캡슐 중심과 몸의 정면을 돌려줍니다.
	OutLocation = GetActorLocation();
	OutRotation = GetActorRotation();
}

void AAgEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_Groggy, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleGroggyChanged);
	ASC->OnAbilityEnded.AddUObject(this, &ThisClass::HandleAbilityEnded);
}

void AAgEnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 사망 모션은 캐릭터가 재생하고 블렌드 아웃 콜백이 래그돌을 켜므로, 끝내는 동안 재생이 끊겨도 부르지 않도록
	// Super보다 먼저 풉니다.
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (FAnimMontageInstance* DeathMotion = AnimInstance ? AnimInstance->GetMontageInstanceForID(DeathMotionInstanceId) : nullptr)
	{
		DeathMotion->OnMontageBlendingOutStarted.Unbind();
	}

	Super::EndPlay(EndPlayReason);
}

void AAgEnemyCharacter::HandleDeathEvent(const FGameplayEventData* Payload)
{
	if (IsDead())
	{
		return;
	}

	// 행동을 취소하면 재생 중인 몽타주가 멈추므로, 재생 위치는 사망 처리 전에 읽습니다.
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	const UAnimMontage* Playing = AnimInstance ? GetAbilitySystemComponent()->GetCurrentMontage() : nullptr;
	const UAgGameplayAbility* Animating = Playing ? Cast<UAgGameplayAbility>(GetAbilitySystemComponent()->GetAnimatingAbility()) : nullptr;
	const FAgAbilityData* AnimatingData = Animating ? GetCharacterData()->FindAbility(Animating->GetAbilityId()) : nullptr;
	if (UAnimMontage* Death = AnimatingData && !AnimatingData->DeathMontage.IsNone() ? GetCharacterData()->FindMontage(AnimatingData->DeathMontage) : nullptr)
	{
		const float Position = AnimInstance ? AnimInstance->Montage_GetPosition(Playing) : 0.f;
		DieWithDeathMotion(*Death, Position);
		return;
	}

	Super::HandleDeathEvent(Payload);
}

void AAgEnemyCharacter::DieWithDeathMotion(UAnimMontage& DeathMontage, float StartPosition)
{
	if (IsDead())
	{
		return;
	}

	ApplyDeathState();
	OnDied.Broadcast(this);

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	FAnimMontageInstance* Instance = AnimInstance && AnimInstance->Montage_Play(&DeathMontage, 1.f, EMontagePlayReturnType::MontageLength, StartPosition) > 0.f
	                                     ? AnimInstance->GetActiveInstanceForMontage(&DeathMontage)
	                                     : nullptr;
	if (!Instance)
	{
		StartRagdoll(FVector::ZeroVector);
		return;
	}
	DeathMotionInstanceId = Instance->GetInstanceID();
	Instance->OnMontageBlendingOutStarted = FOnMontageBlendingOutStarted::CreateUObject(this, &ThisClass::HandleDeathMotionBlendingOut);
}

void AAgEnemyCharacter::HandleDeathMotionBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	DeathMotionInstanceId = INDEX_NONE;
	StartRagdoll(FVector::ZeroVector);
}

void AAgEnemyCharacter::HandleGroggyChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0 || IsDead())
	{
		return;
	}
	UAgAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	// 처형으로 끝난 그로기는 처형 상태 태그가 붙어 효과가 지워진 것입니다.
	if (!ASC->HasMatchingGameplayTag(AgGameplayTags::State_Execution))
	{
		ASC->ApplyGameplayEffectToSelf(UAgAssetManager::Get().GetEffect(EAgEffect::PPRefill), 1.f, ASC->MakeEffectContext());
	}
	ASC->CancelAbilitiesWithSpecTags(FGameplayTagContainer(AgGameplayTags::Ability_Reaction_Groggy));
}

void AAgEnemyCharacter::HandleAbilityEnded(const FAbilityEndedData& Data)
{
	const UAgGameplayAbility* Ability = Cast<UAgGameplayAbility>(Data.AbilityThatEnded);
	if (!Ability || IsDead())
	{
		return;
	}
	const FGameplayTagContainer& DefinitionTags = Ability->GetDefinitionTags();
	const bool bPhaseTransitionCompleted = !Data.bWasCancelled && DefinitionTags.HasTagExact(AgGameplayTags::Ability_Reaction_PhaseTransition);
	if (!DefinitionTags.HasTagExact(AgGameplayTags::Ability_Reaction_Executed) && !bPhaseTransitionCompleted)
	{
		return;
	}
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	ASC->ApplyGameplayEffectToSelf(UAgAssetManager::Get().GetEffect(EAgEffect::PPRefill), 1.f, ASC->MakeEffectContext());
	if (bPhaseTransitionCompleted)
	{
		Phase = GetReachedPhase();
	}
}
