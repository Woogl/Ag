// Copyright Woogle. All Rights Reserved.

#include "Character/AgEnemyCharacter.h"

#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystem/AgTargeting.h"
#include "AbilitySystemComponent.h"
#include "AI/AgEnemyAIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/AgPlayerCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Core/AgAssetManager.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgAbilityData.h"
#include "Data/AgCharacterData.h"
#include "Data/AgEffectData.h"
#include "Engine/World.h"
#include "Game/AgGameState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UI/AgNameplateComponent.h"
#include "UI/AgNameplateWidget.h"

#if WITH_EDITORONLY_DATA
#include "Character/AgEnemySightPreviewComponent.h"
#endif

namespace
{
	const FName LockOnBone(TEXT("spine_05"));
	const FName NameplateBone(TEXT("head"));
	/** 마네킹의 머리 뼈 기준 +X가 정수리 쪽이며, 정수리까지 약 18cm입니다. */
	const FVector NameplateOffset(18.f, 0.f, 0.f);
	const FName GroggyEffectId(TEXT("Groggy"));
} // namespace

AAgEnemyCharacter::AAgEnemyCharacter()
{
	AIControllerClass = AAgEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;

	LockOnMarker = CreateDefaultSubobject<UWidgetComponent>(TEXT("LockOnMarker"));
	ExecutionPrompt = CreateDefaultSubobject<UWidgetComponent>(TEXT("ExecutionPrompt"));
	Nameplate = CreateDefaultSubobject<UAgNameplateComponent>(TEXT("Nameplate"));
	LockOnMarker->SetupAttachment(GetMesh(), LockOnBone);
	ExecutionPrompt->SetupAttachment(GetMesh(), LockOnBone);
	Nameplate->SetupAttachment(GetMesh(), NameplateBone);
	Nameplate->SetRelativeLocation(NameplateOffset);
	for (UWidgetComponent* WorldWidget : {LockOnMarker.Get(), ExecutionPrompt.Get(), static_cast<UWidgetComponent*>(Nameplate.Get())})
	{
		WorldWidget->SetWidgetSpace(EWidgetSpace::Screen);
		WorldWidget->SetDrawAtDesiredSize(true);
		WorldWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WorldWidget->SetGenerateOverlapEvents(false);
		WorldWidget->SetHiddenInGame(true);
	}

	ExecutionPrompt->SetPivot(FVector2D(0.f, 0.5f));
	Nameplate->SetPivot(FVector2D(0.5f, 1.f));

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

void AAgEnemyCharacter::GatherPatterns(float TargetDistance, bool bResponse, TArray<TPair<FName, float>>& OutCandidates) const
{
	OutCandidates.Reset();
	const UAgCharacterData* Data = GetCharacterData();
	for (FName Id : Data->Abilities)
	{
		const FAgAbilityData* Ability = Data->FindAbility(Id);
		const FAgPatternSelection* Pattern =
			Ability && Ability->AbilityTags.HasTag(AgGameplayTags::Ability_Enemy_Pattern) ? Ability->PatternSelection.GetPtrOrNull() : nullptr;
		const float Weight = Pattern ? (bResponse ? Pattern->ResponseChance : Pattern->Weight) : 0.f;
		if (Weight > 0.f && Pattern->IsAvailable(TargetDistance, Phase) && CanActivatePattern(Id))
		{
			OutCandidates.Emplace(Id, Weight);
		}
	}
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

FVector AAgEnemyCharacter::GetTargetLocation(AActor* RequestedBy) const
{
	return GetMesh()->GetBoneIndex(LockOnBone) != INDEX_NONE ? GetMesh()->GetBoneLocation(LockOnBone) : GetActorLocation();
}

void AAgEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_Groggy, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleGroggyChanged);
	GroggyEventHandle = ASC->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_Groggy).AddUObject(this, &ThisClass::HandleGroggyEvent);
	ASC->OnAbilityEnded.AddUObject(this, &ThisClass::HandleAbilityEnded);

	UAgNameplateWidget* NameplateWidget = Cast<UAgNameplateWidget>(Nameplate->GetUserWidgetObject());
	if (NameplateWidget && HasNameplate())
	{
		NameplateWidget->SetAbilitySystem(ASC);
	}
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
	GetAbilitySystemComponent()->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_Groggy).Remove(GroggyEventHandle);

	Super::EndPlay(EndPlayReason);
}

void AAgEnemyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateWorldWidgets();
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
	if (UAnimMontage* Death = Animating ? Animating->GetDefinition().DeathMontage.Get() : nullptr)
	{
		const float Position = AnimInstance ? AnimInstance->Montage_GetPosition(Playing) : 0.f;
		DieWithDeathMotion(*Death, Position);
		return;
	}

	Super::HandleDeathEvent(Payload);
}

int32 AAgEnemyCharacter::GetReachedPhase() const
{
	const TArray<float>& TransitionHPRatios = GetCharacterData()->PhaseTransitionHPRatios;
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

void AAgEnemyCharacter::DieWithDeathMotion(UAnimMontage& DeathMontage, float StartPosition)
{
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
}

void AAgEnemyCharacter::HandleGroggyEvent(const FGameplayEventData* Payload)
{
	// 그로기 모션은 이 효과의 태그로 발동하는 어빌리티가 맡고, 태그가 빠지면 엔진이 끝냅니다.
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	ASC->ApplyGameplayEffectToSelf(UAgAssetManager::Get().FindEffect(GroggyEffectId), 1.f, ASC->MakeEffectContext());
}

void AAgEnemyCharacter::HandleAbilityEnded(const FAbilityEndedData& Data)
{
	const UAgGameplayAbility* Ability = Cast<UAgGameplayAbility>(Data.AbilityThatEnded);
	if (!Ability || IsDead())
	{
		return;
	}
	const FGameplayTagContainer& DefinitionTags = Ability->GetDefinition().AbilityTags;
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

bool AAgEnemyCharacter::IsExecutionTargetOf(AAgPlayerCharacter& Player) const
{
	const FGameplayAbilitySpec* Spec = Player.GetAbilitySystemComponent()->FindSpecByTag(AgGameplayTags::Ability_Action_Execution);
	const FAgAbilityData* Execution = Spec ? Player.FindAbility(Spec->Handle) : nullptr;
	return Execution && Execution->Targeting && AgTargeting::FindTarget(*Execution->Targeting, Player) == this;
}

void AAgEnemyCharacter::UpdateWorldWidgets()
{
	// 플레이어 화면에 관한 것이라 AI의 대상이 아니라 로컬 플레이어를 기준으로 합니다.
	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	AAgPlayerCharacter* Player = PlayerController ? Cast<AAgPlayerCharacter>(PlayerController->GetPawn()) : nullptr;
	const AAgGameState* StageState = GetWorld()->GetGameState<AAgGameState>();
	const bool bStageEnded = StageState && StageState->GetStageResult() != EAgStageResult::None;
	const bool bShown = Player && !Player->IsDead() && !IsDead() && !PlayerController->bCinematicMode && !bStageEnded;

	const bool bLockOnTarget = bShown && Player->GetLockOnTarget() == this;
	LockOnMarker->SetHiddenInGame(!bLockOnTarget);

	// 지금 하는 행동 때문에 처형 키를 받지 않는 동안에도 표시하므로 발동 가능 여부는 보지 않습니다. 범위 확인은
	// 대상 찾기 쿼리를 돌리므로 그로기 중에만 합니다.
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	const bool bGroggy = ASC->HasMatchingGameplayTag(AgGameplayTags::State_Groggy);
	ExecutionPrompt->SetHiddenInGame(!(bShown && bGroggy && IsExecutionTargetOf(*Player)));

	UAgNameplateWidget* NameplateWidget = Cast<UAgNameplateWidget>(Nameplate->GetUserWidgetObject());
	if (!NameplateWidget || !HasNameplate())
	{
		return;
	}
	const bool bAllowed = bShown && IsNameplateInView(*PlayerController);
	const bool bInCombat = !ASC->HasMatchingGameplayTag(AgGameplayTags::State_NonCombat);
	Nameplate->SetFrontmost(bLockOnTarget);
	Nameplate->SetHiddenInGame(!NameplateWidget->Update(bAllowed, bLockOnTarget || bInCombat));
}

bool AAgEnemyCharacter::IsNameplateInView(const APlayerController& PlayerController) const
{
	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;
	PlayerController.GetViewportSize(ViewportWidth, ViewportHeight);
	FVector2D ScreenLocation;
	if (!PlayerController.ProjectWorldLocationToScreen(Nameplate->GetComponentLocation(), ScreenLocation) ||
		ScreenLocation.X < 0.0 || ScreenLocation.Y < 0.0 || ScreenLocation.X > ViewportWidth || ScreenLocation.Y > ViewportHeight)
	{
		return false;
	}

	// 캐릭터는 WorldStatic이 아니므로 다른 캐릭터가 앞을 지나가도 가린 것으로 보지 않습니다.
	FVector ViewLocation;
	FRotator ViewRotation;
	PlayerController.GetPlayerViewPoint(ViewLocation, ViewRotation);
	return !GetWorld()->LineTraceTestByObjectType(ViewLocation, GetActorLocation(), FCollisionObjectQueryParams(ECC_WorldStatic));
}
