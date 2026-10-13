// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgBossPattern_SwordWave.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/Tasks/AgAbilityTask_AttackMovement.h"
#include "AbilitySystem/Tasks/AgAbilityTask_SpawnProjectile.h"
#include "Ag.h"
#include "Animation/AnimMontage.h"
#include "Combat/AgProjectile.h"
#include "Data/AgAttackData.h"
#include "Data/AgCharacterData.h"

UAgBossPattern_SwordWave::UAgBossPattern_SwordWave()
{
	SetupBossPattern();
}

void UAgBossPattern_SwordWave::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	const FAgBossSwordWaveParams* Params = GetAbilityParams<FAgBossSwordWaveParams>();
	UAnimMontage* Montage = Params ? Data->FindMontage(GetAbilityId()) : nullptr;
	if (!Montage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogAg, Verbose, TEXT("Boss pattern %s starts (play rate %.2f)"), *GetAbilityId().ToString(), Data->PatternPlayRate);

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage, Data->PatternPlayRate);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->ReadyForActivation();

	// 몽타주를 재생하지 못하면 OnCancelled가 그 자리에서 어빌리티를 끝냅니다. 끝난 어빌리티에서 발사를 기다리지 않습니다.
	if (!IsActive())
	{
		return;
	}

	// 정지 간격과 접근은 공용 패턴과 같은 규칙입니다(UAgBossPattern_Common).
	UAgAbilityTask_AttackMovement::Attack(this, Params->Hits.Num(), /*InTargetingRange*/ -1.f, Params->MaxApproachDistance)->ReadyForActivation();

	UAgAbilityTask_SpawnProjectile::SpawnProjectiles(this, Data->SwordWaveClass, Params->Hits, Params->Wave)->ReadyForActivation();
}

void UAgBossPattern_SwordWave::HandleMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UAgBossPattern_SwordWave::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
