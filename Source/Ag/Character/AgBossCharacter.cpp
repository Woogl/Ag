// Copyright Woogle. All Rights Reserved.

#include "Character/AgBossCharacter.h"

#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystem/AgTargeting.h"
#include "AbilitySystemComponent.h"
#include "AI/AgBossAIController.h"
#include "Character/AgPlayerCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Core/AgAssetManager.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgAbilityData.h"
#include "Data/AgCharacterData.h"
#include "Data/AgEffectData.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

namespace
{
	const FName LockOnBone(TEXT("spine_05"));
	const FName ExecutionPromptBone(TEXT("head"));
} // namespace

AAgBossCharacter::AAgBossCharacter()
{
	CharacterDataClass = UAgBossData::StaticClass();
	AIControllerClass = AAgBossAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;

	LockOnMarker = CreateDefaultSubobject<UWidgetComponent>(TEXT("LockOnMarker"));
	ExecutionPrompt = CreateDefaultSubobject<UWidgetComponent>(TEXT("ExecutionPrompt"));
	LockOnMarker->SetupAttachment(GetMesh(), LockOnBone);
	ExecutionPrompt->SetupAttachment(GetMesh(), ExecutionPromptBone);
	for (UWidgetComponent* WorldWidget : {LockOnMarker.Get(), ExecutionPrompt.Get()})
	{
		WorldWidget->SetWidgetSpace(EWidgetSpace::Screen);
		WorldWidget->SetDrawAtDesiredSize(true);
		WorldWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WorldWidget->SetGenerateOverlapEvents(false);
		WorldWidget->SetHiddenInGame(true);
	}

	ExecutionPrompt->SetPivot(FVector2D(0.5f, 1.f));
}

FVector AAgBossCharacter::GetLockOnLocation() const
{
	return GetMesh()->GetBoneIndex(LockOnBone) != INDEX_NONE ? GetMesh()->GetBoneLocation(LockOnBone) : GetActorLocation();
}

void AAgBossCharacter::BeginPlay()
{
	Super::BeginPlay();

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_Groggy, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleGroggyChanged);
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_HitReaction, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleHitReactionChanged);
	ASC->OnAbilityEnded.AddUObject(this, &ThisClass::HandleAbilityEnded);
}

void AAgBossCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateWorldWidgets();
}

void AAgBossCharacter::HandleGroggyChanged(const FGameplayTag Tag, int32 NewCount)
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

void AAgBossCharacter::HandleHitReactionChanged(const FGameplayTag Tag, int32 NewCount)
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

void AAgBossCharacter::HandleAbilityEnded(const FAbilityEndedData& Data)
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
		Phase = 2;
	}
}

bool AAgBossCharacter::IsExecutionTargetOf(AAgPlayerCharacter& Player) const
{
	const UAgAbilitySystemComponent* ASC = Cast<UAgAbilitySystemComponent>(Player.GetAbilitySystemComponent());
	const FGameplayAbilitySpec* Spec = ASC ? ASC->FindSpecByTag(AgGameplayTags::Ability_Action_Execution) : nullptr;
	const FAgAbilityData* Execution = Spec ? Player.FindAbility(Spec->Handle) : nullptr;
	return Execution && Execution->Targeting && AgTargeting::FindTarget(*Execution->Targeting, Player) == this;
}

void AAgBossCharacter::UpdateWorldWidgets()
{
	// 플레이어 화면에 관한 것이라 AI의 대상이 아니라 로컬 플레이어를 기준으로 합니다.
	AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	const bool bInCombat = Player && !Player->IsDead() && !IsDead() &&
	                       !GetAbilitySystemComponent()->HasMatchingGameplayTag(AgGameplayTags::State_Execution) &&
	                       !(PlayerController && PlayerController->bCinematicMode);

	LockOnMarker->SetHiddenInGame(!(bInCombat && Player->GetLockOnTarget() == this));

	// 지금 하는 행동 때문에 처형 키를 받지 않는 동안에도 표시하므로 발동 가능 여부는 보지 않습니다. 범위 확인은
	// 대상 찾기 쿼리를 돌리므로 그로기 중에만 합니다.
	const bool bGroggy = GetAbilitySystemComponent()->HasMatchingGameplayTag(AgGameplayTags::State_Groggy);
	ExecutionPrompt->SetHiddenInGame(!(bInCombat && bGroggy && IsExecutionTargetOf(*Player)));
}
