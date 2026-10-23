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
#include "TimerManager.h"

AAgEnemyCharacter::AAgEnemyCharacter()
{
	CharacterDataClass = UAgEnemyData::StaticClass();
	Team = EAgTeam::Enemy;
	AIControllerClass = AAgEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
}

bool AAgEnemyCharacter::CanLosePP() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	return ASC && !ASC->HasMatchingGameplayTag(AgGameplayTags::State_GroggyImmune) &&
	       !ASC->HasMatchingGameplayTag(AgGameplayTags::State_Groggy) &&
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
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_HitReaction, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleHitReactionChanged);
	ASC->OnAbilityEnded.AddUObject(this, &ThisClass::HandleAbilityEnded);
}

void AAgEnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (FAnimMontageInstance* DeathMotion = AnimInstance ? AnimInstance->GetMontageInstanceForID(DeathMotionInstanceId) : nullptr)
	{
		DeathMotion->OnMontageBlendingOutStarted.Unbind();
	}

	Super::EndPlay(EndPlayReason);
}

void AAgEnemyCharacter::HandleDeathEvent(const FGameplayEventData* Payload)
{
	if (IsDead() || bEndingPlay)
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
	if (!bEndingPlay)
	{
		StartRagdoll(FVector::ZeroVector);
	}
}

void AAgEnemyCharacter::HandleGroggyChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0 || IsDead())
	{
		return;
	}
	UAgAbilitySystemComponent* ASC = CastChecked<UAgAbilitySystemComponent>(GetAbilitySystemComponent());
	// 엔진은 처형 상태 태그를 먼저 붙인 뒤 그로기 행동을 취소합니다.
	if (!ASC->HasMatchingGameplayTag(AgGameplayTags::State_Execution))
	{
		ASC->ApplyGameplayEffectToSelf(UAgAssetManager::Get().GetEffect(EAgEffect::PPRefill), 1.f, ASC->MakeEffectContext());
	}
	ASC->CancelAbilitiesWithSpecTags(FGameplayTagContainer(AgGameplayTags::Ability_Reaction_Groggy));
}

void AAgEnemyCharacter::HandleHitReactionChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		return;
	}
	// 그로기 행동은 피격 반응 중에 시작하지 못하므로(행동 정의의 BlockedTags), PP가 0으로 남아 있으면 다시 보냅니다.
	// 재발동(저글)은 같은 프레임 안에서 태그를 뗐다 다시 붙이므로 다음 틱에 확인합니다.
	GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]
	{
		UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
		if (!IsDead() && !ASC->HasMatchingGameplayTag(AgGameplayTags::State_HitReaction) && GetAttributeSet()->GetPP() <= 0.f)
		{
			// 그로기 행동은 발동하는 동안 이 태그로 일어선 자세에 맞는 시작 섹션을 고릅니다.
			ASC->AddLooseGameplayTag(AgGameplayTags::State_GroggyDelayed);
			SendEventToSelf(AgGameplayTags::Event_Groggy);
			ASC->RemoveLooseGameplayTag(AgGameplayTags::State_GroggyDelayed);
		}
	}));
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
