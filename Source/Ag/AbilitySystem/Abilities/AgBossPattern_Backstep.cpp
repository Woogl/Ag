// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgBossPattern_Backstep.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Ag.h"
#include "Animation/AnimMontage.h"
#include "Character/AgBossCharacter.h"
#include "Data/AgCharacterData.h"

UAgBossPattern_Backstep::UAgBossPattern_Backstep()
{
	SetupBossPattern();
}

void UAgBossPattern_Backstep::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AAgBossCharacter* Boss = Cast<AAgBossCharacter>(GetAgCharacter());
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	const FAgBossPattern* Pattern = Boss ? Boss->FindPattern(Handle) : nullptr;
	UAnimMontage* Montage = (Data && Pattern) ? Data->FindPatternMontage(Pattern->Pattern) : nullptr;
	if (!Montage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogAg, Verbose, TEXT("Boss pattern %s starts (play rate %.2f)"), *Pattern->Pattern.ToString(), Boss->GetPatternSpeed());
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage, Boss->GetPatternSpeed());
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageFinished);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::HandleMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UAgBossPattern_Backstep::HandleMontageFinished()
{
	EndSelf(false);
}

void UAgBossPattern_Backstep::HandleMontageCancelled()
{
	EndSelf(true);
}
