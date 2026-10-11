// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_Execution.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/Tasks/AgAbilityTask_AttackMovement.h"
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

AAgCharacterBase* UAgAbility_Execution::FindExecutionTarget(const AAgPlayerCharacter* Player)
{
	const UAgPlayerData* Data = Player ? Cast<UAgPlayerData>(Player->GetCharacterData()) : nullptr;
	if (!Data)
	{
		return nullptr;
	}

	// 그로기 중이고, 이번 그로기에 아직 처형되지 않았고, 땅에 있는 적입니다(공중에서 그로기가 되면 착지 전에는 처형할 수 없음).
	return Player->FindEnemy(Data->Execution.TargetingRange, [](const AAgCharacterBase& Enemy)
	{
		const UAbilitySystemComponent* EnemyASC = Enemy.GetAbilitySystemComponent();
		return EnemyASC->HasMatchingGameplayTag(AgGameplayTags::State_Groggy) && !EnemyASC->HasMatchingGameplayTag(AgGameplayTags::State_Execution_Executed)
			&& !Enemy.GetCharacterMovement()->IsFalling();
	});
}

bool UAgAbility_Execution::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	const AAgPlayerCharacter* Player = ActorInfo ? Cast<AAgPlayerCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags) && FindExecutionTarget(Player);
}

void UAgAbility_Execution::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(GetAgCharacter());
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	AAgCharacterBase* Enemy = FindExecutionTarget(Player);
	if (!Enemy || !Data || !Data->ExecutionMontage || Data->Execution.Hits.IsEmpty() || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	Target = Enemy;
	StrikeIndex = 0;

	// 후딜이 없습니다. 행동 중이 모션 끝까지 이어져 어떤 행동도 처형을 끊지 못합니다.
	BeginBusy();
	SetExecuting(true);
	bInvincible = true;
	GetAbilitySystemComponentFromActorInfo()->AddLooseGameplayTag(AgGameplayTags::State_Invincible);

	// 대상은 같은 프레임에 플레이어를 향하고 처형 피격 몽타주를 시작하므로, 두 몽타주가 한 타임라인을 따릅니다.
	FGameplayEventData Payload;
	Payload.EventTag = AgGameplayTags::Event_Executed;
	Payload.Instigator = Player;
	Payload.Target = Enemy;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Enemy, AgGameplayTags::Event_Executed, Payload);

	// 첫 베기(워프 구간의 끝)까지 플레이어는 대상 정면, 두 모션이 맞물리는 간격에 섭니다. 정지 간격은 적용하지 않습니다.
	UAgAbilityTask_AttackMovement::FitGap(this, Enemy, Data->ExecutionDistance)->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* StrikeTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Event_ExecutionBlow);
	StrikeTask->EventReceived.AddDynamic(this, &ThisClass::HandleStrike);
	StrikeTask->ReadyForActivation();

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

void UAgAbility_Execution::HandleStrike(FGameplayEventData Payload)
{
	AAgCharacterBase* Player = GetAgCharacter();
	AAgCharacterBase* Enemy = Target.Get();
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	if (!Player || !Data || !Data->Execution.Hits.IsValidIndex(StrikeIndex))
	{
		return;
	}
	const FAgPlayerAttackHit& Hit = Data->Execution.Hits[StrikeIndex];
	++StrikeIndex;
	const bool bFinalBlow = StrikeIndex == Data->Execution.Hits.Num();

	// 타격마다 처형 대상에게 히트스톱 없이 적용합니다. 앞 타격에 이미 죽은 대상은 더 받지 않습니다.
	if (Enemy && !Enemy->IsDead() && Enemy->GetAttributeSet()->GetHP() > 0.f)
	{
		const int32 HPDamage = UAgCombatLibrary::CalculateHPDamage(Player->GetAttributeSet()->GetATK(), Hit.DamageMultiplier, Enemy->GetAttributeSet()->GetDEF());
		UAgCombatLibrary::ApplyStatChange(Player, Enemy, -HPDamage, 0.f);
		// 대미지 숫자는 보스에게만 표시합니다(전투 HUD '대미지 숫자').
		if (const AAgBossCharacter* Boss = Cast<AAgBossCharacter>(Enemy))
		{
			Boss->ShowDamageFloater(HPDamage, Boss->GetActorLocation(), Hit.bLargeDamageFloater);
		}
		FGameplayCueParameters EffectParameters;
		EffectParameters.Location = Enemy->GetActorLocation();
		Enemy->GetAbilitySystemComponent()->ExecuteGameplayCue(AgGameplayTags::GameplayCue_Hit, EffectParameters);
		if (const UAgCameraData* Camera = UAgCombatLibrary::GetCameraData())
		{
			UAgCombatLibrary::PlayCameraShake(Player, bFinalBlow ? Camera->ExecutionBlowShake : Camera->ExecutionSlashShake);
		}
		UE_LOG(LogAg, Verbose, TEXT("Execution strike %d: HP -%d (now %.0f)"), StrikeIndex, HPDamage, Enemy->GetAttributeSet()->GetHP());
		if (Enemy->GetAttributeSet()->GetHP() <= 0.f)
		{
			FGameplayEventData DeathPayload;
			DeathPayload.EventTag = AgGameplayTags::Event_Death;
			DeathPayload.Instigator = Player;
			DeathPayload.Target = Enemy;
			DeathPayload.InstigatorTags.AddTag(AgGameplayTags::Ability_Action_Execution);
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Enemy, AgGameplayTags::Event_Death, DeathPayload);
		}
	}
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
	Target.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
