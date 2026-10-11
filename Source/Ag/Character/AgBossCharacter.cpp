// Copyright Woogle. All Rights Reserved.

#include "Character/AgBossCharacter.h"

#include "AbilitySystem/Abilities/AgAbility_Execution.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Ag.h"
#include "AI/AgBossAIController.h"
#include "Character/AgLockOnComponent.h"
#include "Character/AgPlayerCharacter.h"
#include "Components/WidgetComponent.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

AAgBossCharacter::AAgBossCharacter()
{
	AIControllerClass = AAgBossAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// The boss turns toward the player by itself, not toward where it walks.
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	// Screen space: the widgets keep their own size at any distance. They attach to the data's bones on BeginPlay.
	LockOnMarker = CreateDefaultSubobject<UWidgetComponent>(TEXT("LockOnMarker"));
	ExecutionPrompt = CreateDefaultSubobject<UWidgetComponent>(TEXT("ExecutionPrompt"));
	for (UWidgetComponent* WorldWidget : { LockOnMarker.Get(), ExecutionPrompt.Get() })
	{
		WorldWidget->SetupAttachment(GetMesh());
		WorldWidget->SetWidgetSpace(EWidgetSpace::Screen);
		WorldWidget->SetDrawAtDesiredSize(true);
		WorldWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WorldWidget->SetGenerateOverlapEvents(false);
		WorldWidget->SetHiddenInGame(true);
	}

	// The prompt stands on its bone: 보스 머리 위.
	ExecutionPrompt->SetPivot(FVector2D(0.5f, 1.f));
}

void AAgBossCharacter::BeginPlay()
{
	Super::BeginPlay();

	AttachWorldWidgets();
}

void AAgBossCharacter::AttachWorldWidgets()
{
	const UAgBossData* Data = CastChecked<UAgBossData>(GetCharacterData());
	LockOnMarker->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, Data->LockOnBone);
	ExecutionPrompt->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, Data->ExecutionPromptBone);
}

void AAgBossCharacter::GrantAbilities()
{
	Super::GrantAbilities();

	const UAgBossData* Data = CastChecked<UAgBossData>(GetCharacterData());
	for (const FAgBossPattern& Pattern : Data->Patterns)
	{
		const TSubclassOf<UAgGameplayAbility> AbilityClass = Data->FindPatternAbility(Pattern.Pattern);
		if (!AbilityClass)
		{
			UE_LOG(LogAg, Error, TEXT("%s: pattern %s has no ability in %s's PatternAbilities."), *GetName(), *Pattern.Pattern.ToString(), *Data->GetName());
			continue;
		}
		if (!Data->FindPatternCooldownTag(Pattern.Pattern).IsValid())
		{
			UE_LOG(LogAg, Error, TEXT("%s: pattern %s has no cooldown tag in %s's PatternCooldownTags."), *GetName(), *Pattern.Pattern.ToString(), *Data->GetName());
		}
		FGameplayAbilitySpec Spec(AbilityClass);
		Spec.GetDynamicSpecSourceTags().AddTag(Pattern.Pattern);
		GetAbilitySystemComponent()->GiveAbility(Spec);
	}
}

FGameplayTag AAgBossCharacter::GetPatternTag(const FGameplayAbilitySpec& Spec)
{
	for (const FGameplayTag& Tag : Spec.GetDynamicSpecSourceTags())
	{
		if (Tag.MatchesTag(AgGameplayTags::Ability_Boss_Pattern))
		{
			return Tag;
		}
	}
	return FGameplayTag();
}

FGameplayAbilitySpec* AAgBossCharacter::FindPatternSpec(const FGameplayTag& Pattern) const
{
	for (FGameplayAbilitySpec& Spec : GetAbilitySystemComponent()->GetActivatableAbilities())
	{
		if (GetPatternTag(Spec) == Pattern)
		{
			return &Spec;
		}
	}
	return nullptr;
}

const FAgBossPattern* AAgBossCharacter::FindPattern(const FGameplayAbilitySpecHandle Handle) const
{
	const UAgBossData* Data = CastChecked<UAgBossData>(GetCharacterData());
	const FGameplayAbilitySpec* Spec = GetAbilitySystemComponent()->FindAbilitySpecFromHandle(Handle);
	return Spec ? Data->FindPattern(GetPatternTag(*Spec)) : nullptr;
}

float AAgBossCharacter::GetPatternPlayRate() const
{
	return CastChecked<UAgBossData>(GetCharacterData())->PatternPlayRate * GetPatternSpeed();
}

void AAgBossCharacter::EnterPhase2()
{
	Phase = 2;
	UpdateMoveSpeed();
}

float AAgBossCharacter::GetPatternSpeed() const
{
	return Phase >= 2 ? CastChecked<UAgBossData>(GetCharacterData())->Phase2PatternSpeed : 1.f;
}

float AAgBossCharacter::GetMoveSpeedMultiplier() const
{
	return Phase >= 2 ? CastChecked<UAgBossData>(GetCharacterData())->Phase2MOVMultiplier : 1.f;
}

void AAgBossCharacter::SetGroggyTimerRunning(bool bRunning)
{
	GroggyStartTime = bRunning ? GetWorld()->GetTimeSeconds() : -1.0;
}

float AAgBossCharacter::GetGroggyElapsedTime() const
{
	return GroggyStartTime >= 0.0 ? static_cast<float>(GetWorld()->GetTimeSeconds() - GroggyStartTime) : -1.f;
}

void AAgBossCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateWorldWidgets();
}

AAgCharacterBase* AAgBossCharacter::GetTarget() const
{
	const AAgBossAIController* AIController = Cast<AAgBossAIController>(GetController());
	return AIController ? AIController->GetTarget() : nullptr;
}

void AAgBossCharacter::UpdateWorldWidgets()
{
	// 둘 다 어느 쪽이든 사망 연출이 시작되는 순간부터, 처형 중에, 그리고 궁극기 컷신(플레이어 컨트롤러의 시네마틱 모드) 동안
	// 숨깁니다. 플레이어 화면에 관한 것이라 AI의 대상이 아니라 로컬 플레이어를 기준으로 합니다.
	const AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	const bool bInCombat = Player && !Player->IsDead() && !IsDead()
		&& !GetAbilitySystemComponent()->HasMatchingGameplayTag(AgGameplayTags::State_Execution_Executed)
		&& !(PlayerController && PlayerController->bCinematicMode);

	LockOnMarker->SetHiddenInGame(!(bInCombat && Player->GetLockOn()->GetTarget() == this));

	// 처형 발동 조건 1, 2(그로기, 범위 안)를 만족하는 처형 대상이면 표시합니다. 지금 하는 행동 때문에 처형 키를 받지 않는 동안에도
	// 표시합니다. 범위 확인은 대상 찾기 쿼리를 돌리므로 그로기 중에만 합니다.
	const bool bGroggy = GetAbilitySystemComponent()->HasMatchingGameplayTag(AgGameplayTags::State_Groggy);
	ExecutionPrompt->SetHiddenInGame(!(bInCombat && bGroggy && UAgAbility_Execution::FindExecutionTarget(Player) == this));
}

bool AAgBossCharacter::IsFacingFixed() const
{
	FGameplayTagContainer NoTurning;
	NoTurning.AddTag(AgGameplayTags::State_HitReaction);
	NoTurning.AddTag(AgGameplayTags::State_Groggy);
	NoTurning.AddTag(AgGameplayTags::State_Execution_Executed);
	NoTurning.AddTag(AgGameplayTags::State_Boss_RotationLocked);
	NoTurning.AddTag(AgGameplayTags::State_NonCombat);
	return Super::IsFacingFixed() || GetAbilitySystemComponent()->HasAnyMatchingGameplayTags(NoTurning);
}
