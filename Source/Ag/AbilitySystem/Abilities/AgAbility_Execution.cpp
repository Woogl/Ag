// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_Execution.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Ag.h"
#include "Character/AgBossCharacter.h"
#include "Character/AgPlayerCharacter.h"
#include "Combat/AgCombatLibrary.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCameraData.h"
#include "Data/AgCharacterData.h"
#include "GameFramework/CharacterMovementComponent.h"

UAgAbility_Execution::UAgAbility_Execution()
{
	SetupPlayerAction(AgGameplayTags::Ability_Action_Execution);
}

AAgBossCharacter* UAgAbility_Execution::FindExecutableBoss(const AAgPlayerCharacter* Player)
{
	const UAgPlayerData* Data = Player ? Cast<UAgPlayerData>(Player->GetCharacterData()) : nullptr;
	AAgBossCharacter* Boss = Data ? Player->FindBoss(Data->Execution.TargetingRange) : nullptr;
	if (!Boss)
	{
		return nullptr;
	}

	// Groggy, not executed yet in this groggy, and on the ground (a boss broken in the air can't be executed before it lands).
	const UAbilitySystemComponent* BossASC = Boss->GetAbilitySystemComponent();
	const bool bExecutable = BossASC->HasMatchingGameplayTag(AgGameplayTags::State_Groggy) && !BossASC->HasMatchingGameplayTag(AgGameplayTags::State_Execution_Executed)
		&& !Boss->GetCharacterMovement()->IsFalling();
	return bExecutable ? Boss : nullptr;
}

bool UAgAbility_Execution::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	const AAgPlayerCharacter* Player = ActorInfo ? Cast<AAgPlayerCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags) && FindExecutableBoss(Player);
}

void UAgAbility_Execution::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(GetAgCharacter());
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	AAgBossCharacter* Boss = FindExecutableBoss(Player);
	if (!Boss || !Data || !Data->ExecutionMontage || Data->Execution.Hits.IsEmpty() || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	Target = Boss;
	bBlowLanded = false;

	BeginBusy();
	SetExecuting(true);
	bInvincible = true;
	GetAbilitySystemComponentFromActorInfo()->AddLooseGameplayTag(AgGameplayTags::State_Invincible);

	// The boss turns to the player and plays its 처형 피격 motion.
	FGameplayEventData Payload;
	Payload.EventTag = AgGameplayTags::Event_Executed;
	Payload.Instigator = Player;
	Payload.Target = Boss;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Boss, AgGameplayTags::Event_Executed, Payload);

	// The player moves to the boss's front, to the gap where the two motions line up. No stop distance applies.
	Player->StartApproach(Boss, Data->ExecutionDistance, 0.f, 0.f, /*bFaceTarget*/ true, /*bExactGap*/ true);

	UAbilityTask_WaitGameplayEvent* BlowTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Event_ExecutionBlow);
	BlowTask->EventReceived.AddDynamic(this, &ThisClass::HandleFinalBlow);
	BlowTask->ReadyForActivation();

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Data->ExecutionMontage);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UAgAbility_Execution::SetExecuting(bool bNewExecuting)
{
	if (bExecuting == bNewExecuting)
	{
		return;
	}
	bExecuting = bNewExecuting;
	if (bExecuting)
	{
		GetAbilitySystemComponentFromActorInfo()->AddLooseGameplayTag(AgGameplayTags::State_Execution_Executing);
	}
	else
	{
		GetAbilitySystemComponentFromActorInfo()->RemoveLooseGameplayTag(AgGameplayTags::State_Execution_Executing);
	}
}

void UAgAbility_Execution::HandleFinalBlow(FGameplayEventData Payload)
{
	AAgCharacterBase* Player = GetAgCharacter();
	AAgBossCharacter* Boss = Target.Get();
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	if (bBlowLanded || !Player || !Boss || !Data || Boss->IsDead())
	{
		return;
	}
	bBlowLanded = true;

	// The damage lands once on the executed boss, without hitstop.
	const FAgAttackHit& Hit = Data->Execution.Hits[0];
	const int32 HPDamage = UAgCombatLibrary::CalculateHPDamage(Player->GetAttributeSet()->GetATK(), Hit.DamageMultiplier, Boss->GetAttributeSet()->GetDEF());
	UAgCombatLibrary::ApplyStatChange(Player, Boss, -HPDamage, 0.f);
	Boss->ShowDamageNumber(HPDamage, Boss->GetActorLocation(), Hit.bLargeDamageNumber);
	FGameplayCueParameters EffectParameters;
	EffectParameters.Location = Boss->GetActorLocation();
	Boss->GetAbilitySystemComponent()->ExecuteGameplayCue(AgGameplayTags::GameplayCue_Hit, EffectParameters);
	if (const UAgCameraData* Camera = UAgCombatLibrary::GetCameraData())
	{
		UAgCombatLibrary::PlayCameraShake(Player, Camera->ExecutionBlowShake);
	}
	UE_LOG(LogAg, Verbose, TEXT("Execution blow: HP -%d (now %.0f)"), HPDamage, Boss->GetAttributeSet()->GetHP());
	if (Boss->GetAttributeSet()->GetHP() <= 0.f)
	{
		FGameplayEventData DeathPayload;
		DeathPayload.EventTag = AgGameplayTags::Event_Death;
		DeathPayload.Instigator = Player;
		DeathPayload.Target = Boss;
		DeathPayload.InstigatorTags.AddTag(AgGameplayTags::Ability_Action_Execution);
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Boss, AgGameplayTags::Event_Death, DeathPayload);
	}

	// 후딜: after the final blow the player can cancel; the boss's motion plays on either way.
	Player->StopApproach();
	SetExecuting(false);
	EndBusy();
}

void UAgAbility_Execution::HandleMontageFinished()
{
	EndSelf(false);
}

void UAgAbility_Execution::HandleMontageCancelled()
{
	EndSelf(true);
}

void UAgAbility_Execution::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	SetExecuting(false);
	if (bInvincible)
	{
		bInvincible = false;
		GetAbilitySystemComponentFromActorInfo()->RemoveLooseGameplayTag(AgGameplayTags::State_Invincible);
	}
	if (AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(GetAgCharacter()))
	{
		Player->ClearAttackMovement();
	}
	Target.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
