// Copyright Woogle. All Rights Reserved.

#include "Character/AgBossCharacter.h"

#include "AbilitySystem/Abilities/AgAbility_Execution.h"
#include "AbilitySystem/Abilities/AgBossPatternAbility.h"
#include "AbilitySystemComponent.h"
#include "AI/AgBossAIController.h"
#include "Animation/AgAnimNotify_GameplayEvent.h"
#include "Animation/AgAnimNotifyState_AttackWindow.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/AgLockOnComponent.h"
#include "Character/AgPlayerCharacter.h"
#include "Components/WidgetComponent.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UI/AgDamageNumber.h"

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

	if (const UAgBossData* Data = Cast<UAgBossData>(GetCharacterData()))
	{
		LockOnMarker->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, Data->LockOnBone);
		ExecutionPrompt->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, Data->ExecutionPromptBone);
	}
}

void AAgBossCharacter::GrantAbilities()
{
	Super::GrantAbilities();

	const UAgBossData* Data = Cast<UAgBossData>(GetCharacterData());
	if (!Data)
	{
		return;
	}
	for (const FAgBossPattern& Pattern : Data->Patterns)
	{
		if (Pattern.AbilityClass && Pattern.Pattern.IsValid())
		{
			FGameplayAbilitySpec Spec(Pattern.AbilityClass);
			Spec.GetDynamicSpecSourceTags().AddTag(Pattern.Pattern);
			GetAbilitySystemComponent()->GiveAbility(Spec);
		}
	}
}

FGameplayAbilitySpec* AAgBossCharacter::FindPatternSpec(const FGameplayTag& Pattern) const
{
	for (FGameplayAbilitySpec& Spec : GetAbilitySystemComponent()->GetActivatableAbilities())
	{
		if (UAgBossPatternAbility::GetPatternTag(Spec) == Pattern)
		{
			return &Spec;
		}
	}
	return nullptr;
}

void AAgBossCharacter::EnterPhase2()
{
	Phase = 2;
	UpdateMoveSpeed();
}

float AAgBossCharacter::GetPatternSpeed() const
{
	const UAgBossData* Data = Cast<UAgBossData>(GetCharacterData());
	return (Data && Phase >= 2) ? Data->Phase2PatternSpeed : 1.f;
}

float AAgBossCharacter::GetMoveSpeedMultiplier() const
{
	const UAgBossData* Data = Cast<UAgBossData>(GetCharacterData());
	return (Data && Phase >= 2) ? Data->Phase2MOVMultiplier : 1.f;
}

void AAgBossCharacter::SetGroggyTimerRunning(bool bRunning)
{
	GroggyStartTime = bRunning ? GetWorld()->GetTimeSeconds() : -1.0;
}

float AAgBossCharacter::GetGroggyElapsedTime() const
{
	return GroggyStartTime >= 0.0 ? static_cast<float>(GetWorld()->GetTimeSeconds() - GroggyStartTime) : -1.f;
}

void AAgBossCharacter::ShowDamageNumber(int32 Amount, const FVector& Location, bool bLarge) const
{
	AAgDamageNumber::Spawn(GetWorld(), DamageNumberWidget, Location, Amount, bLarge);
}

void AAgBossCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateRotation(DeltaSeconds);
	UpdateWorldWidgets();
}

void AAgBossCharacter::UpdateWorldWidgets()
{
	// Both hide from the moment either death presentation starts, and during 처형.
	const AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	const bool bInCombat = Player && !Player->IsDead() && !IsDead()
		&& !GetAbilitySystemComponent()->HasMatchingGameplayTag(AgGameplayTags::State_Execution_Executed);

	LockOnMarker->SetHiddenInGame(!(bInCombat && Player->GetLockOn()->GetTarget() == this));

	// 처형 발동 조건 1 and 2 (groggy, in range). Shown even while the player's current action holds off the execution key.
	// The range check runs a targeting query, so it only runs during groggy.
	const bool bGroggy = GetAbilitySystemComponent()->HasMatchingGameplayTag(AgGameplayTags::State_Groggy);
	ExecutionPrompt->SetHiddenInGame(!(bInCombat && bGroggy && UAgAbility_Execution::FindExecutableBoss(Player) == this));
}

void AAgBossCharacter::UpdateRotation(float DeltaSeconds)
{
	const UAgBossData* Data = Cast<UAgBossData>(GetCharacterData());
	const AActor* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (IsDead() || !Data || !Player)
	{
		return;
	}

	FGameplayTagContainer NoTurning;
	NoTurning.AddTag(AgGameplayTags::State_Groggy);
	NoTurning.AddTag(AgGameplayTags::State_Execution_Executed);
	NoTurning.AddTag(AgGameplayTags::State_Boss_RotationLocked);
	NoTurning.AddTag(AgGameplayTags::State_NonCombat);
	if (GetAbilitySystemComponent()->HasAnyMatchingGameplayTags(NoTurning))
	{
		return;
	}

	if (IsInRotationStopWindow())
	{
		// The first-hit approach target is fixed where the player is when the boss stops turning.
		StopApproach();
		return;
	}

	const FVector ToPlayer = (Player->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (!ToPlayer.IsNearlyZero())
	{
		const FRotator Goal(0.f, ToPlayer.Rotation().Yaw, 0.f);
		SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(), Goal, DeltaSeconds, Data->RotationSpeed));
	}
}

bool AAgBossCharacter::IsInRotationStopWindow() const
{
	const UAgBossData* Data = Cast<UAgBossData>(GetCharacterData());
	const UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!Data || !AnimInstance)
	{
		return false;
	}

	// The lead is in seconds at the pattern play rate; in montage time it scales with that rate.
	// Phase 2 speeds up both the montage and the lead by the same factor, so this montage-time lead doesn't change.
	const float LeadInMontageTime = Data->RotationStopLead * Data->PatternPlayRate;
	for (const FAnimMontageInstance* Instance : AnimInstance->MontageInstances)
	{
		if (!Instance || !Instance->Montage || !Instance->IsActive())
		{
			continue;
		}
		const float Position = Instance->GetPosition();
		for (const FAnimNotifyEvent& Notify : Instance->Montage->Notifies)
		{
			if (Cast<UAgAnimNotifyState_AttackWindow>(Notify.NotifyStateClass)
				&& Position >= Notify.GetTriggerTime() - LeadInMontageTime && Position <= Notify.GetEndTriggerTime())
			{
				return true;
			}

			// 보스 사양 'A6': each swing's release is its attack window.
			const UAgAnimNotify_GameplayEvent* Event = Cast<UAgAnimNotify_GameplayEvent>(Notify.Notify);
			if (Event && Event->EventTag == AgGameplayTags::Event_Boss_SwordWave
				&& Position >= Notify.GetTriggerTime() - LeadInMontageTime && Position <= Notify.GetTriggerTime())
			{
				return true;
			}
		}
	}
	return false;
}
