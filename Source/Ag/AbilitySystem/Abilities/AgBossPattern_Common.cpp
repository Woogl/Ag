// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgBossPattern_Common.h"

#include "AbilitySystem/Tasks/AgAbilityTask_Attack.h"
#include "Ag.h"
#include "Character/AgBossCharacter.h"
#include "Data/AgCharacterData.h"

UAgBossPattern_Common::UAgBossPattern_Common()
{
	SetupBossPattern();
}

void UAgBossPattern_Common::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AAgBossCharacter* Boss = CastChecked<AAgBossCharacter>(GetAgCharacter());
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	const FAgBossPattern* Pattern = Boss->FindPattern(Handle);
	UAnimMontage* Montage = Pattern ? Data->FindPatternMontage(Pattern->Id) : nullptr;
	if (!Montage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogAg, Verbose, TEXT("Boss pattern %s starts (play rate %.2f)"), *Pattern->Id.ToString(), Boss->GetPatternPlayRate());

	// 보스는 타겟팅 범위가 없고, 몽타주에 워핑 구간이 있는 타(2연격·3연격의 1타 선딜, 뛰어 내려찍기의 체공)만 다가갑니다.
	UAgAbilityTask_Attack* AttackTask = UAgAbilityTask_Attack::PlayAttack(this, *Pattern, Montage, Boss->GetPatternPlayRate());
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
