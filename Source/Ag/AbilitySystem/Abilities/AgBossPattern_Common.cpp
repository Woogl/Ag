// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgBossPattern_Common.h"

#include "AbilitySystem/Tasks/AgAbilityTask_Attack.h"
#include "Ag.h"
#include "Data/AgCharacterData.h"

UAgBossPattern_Common::UAgBossPattern_Common()
{
	SetupBossPattern();
}

void UAgBossPattern_Common::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	const FAgBossPatternParams* Pattern = GetAbilityParams<FAgBossPatternParams>();
	UAnimMontage* Montage = Pattern ? Data->FindMontage(GetAbilityId()) : nullptr;
	if (!Montage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogAg, Verbose, TEXT("Boss pattern %s starts (play rate %.2f)"), *GetAbilityId().ToString(), Data->PatternPlayRate);

	// 보스는 타겟팅 범위가 없고, 몽타주에 워핑 구간이 있는 타(DoubleStrike 1타의 선딜, JumpSlam의 체공)만 다가갑니다.
	UAgAbilityTask_Attack* AttackTask = UAgAbilityTask_Attack::PlayAttack(this, *Pattern, Montage, Data->PatternPlayRate);
	AttackTask->OnCompleted.AddDynamic(this, &ThisClass::HandleAttackCompleted);
	AttackTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleAttackInterrupted);
	AttackTask->ReadyForActivation();
}

void UAgBossPattern_Common::HandleAttackCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UAgBossPattern_Common::HandleAttackInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
