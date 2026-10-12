// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgBossPattern_SwordWave.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/Tasks/AgAbilityTask_AttackMovement.h"
#include "AbilitySystem/Tasks/AgAbilityTask_HoldTag.h"
#include "AbilitySystem/Tasks/AgAbilityTask_SpawnProjectile.h"
#include "AbilitySystem/Tasks/AgAbilityTask_WaitMontagePosition.h"
#include "Ag.h"
#include "Animation/AgAnimNotify_GameplayEvent.h"
#include "Animation/AnimMontage.h"
#include "Character/AgBossCharacter.h"
#include "Combat/AgProjectile.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgAttackData.h"
#include "Data/AgCharacterData.h"

UAgBossPattern_SwordWave::UAgBossPattern_SwordWave()
{
	SetupBossPattern();
}

void UAgBossPattern_SwordWave::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AAgBossCharacter* Boss = CastChecked<AAgBossCharacter>(GetAgCharacter());
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	const FAgBossPattern* Pattern = Boss->FindPattern(Handle);
	UAnimMontage* Montage = Pattern ? Data->FindPatternMontage(Pattern->Id) : nullptr;
	const FAgBossSwordWaveParams* Params = GetPatternParams<FAgBossSwordWaveParams>();
	if (!Montage || !Params || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogAg, Verbose, TEXT("Boss pattern %s starts (play rate %.2f)"), *Pattern->Id.ToString(), Data->PatternPlayRate);

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
	UAgAbilityTask_AttackMovement::Attack(this, Pattern->Hits.Num(), /*InTargetingRange*/ -1.f, Pattern->MaxApproachDistance)->ReadyForActivation();

	WaveIndex = 0;
	RotationLockTag = nullptr;
	UAgAbilityTask_SpawnProjectile* WaveTask = UAgAbilityTask_SpawnProjectile::SpawnProjectiles(this, Data->SwordWaveClass, Pattern->Hits, Params->Wave);
	WaveTask->OnFired.AddDynamic(this, &ThisClass::HandleFired);
	WaveTask->ReadyForActivation();

	ReleaseTimes.Reset();
	for (const FAnimNotifyEvent& Notify : Montage->Notifies)
	{
		const UAgAnimNotify_GameplayEvent* Event = Cast<UAgAnimNotify_GameplayEvent>(Notify.Notify);
		if (Event && Event->EventTag == AgGameplayTags::Event_ProjectileRelease)
		{
			ReleaseTimes.Add(Notify.GetTriggerTime());
		}
	}
	ReleaseTimes.Sort();
	WaitRotationStop();
}

void UAgBossPattern_SwordWave::WaitRotationStop()
{
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	if (!ReleaseTimes.IsValidIndex(WaveIndex))
	{
		return;
	}
	UAgAbilityTask_WaitMontagePosition* StopTask = UAgAbilityTask_WaitMontagePosition::WaitMontagePosition(this, GetCurrentMontage(), ReleaseTimes[WaveIndex] - Data->GetRotationStopLeadInMontage());
	StopTask->OnReached.AddDynamic(this, &ThisClass::HandleRotationStop);
	StopTask->ReadyForActivation();
}

void UAgBossPattern_SwordWave::HandleRotationStop()
{
	if (!RotationLockTag)
	{
		RotationLockTag = UAgAbilityTask_HoldTag::HoldTag(this, AgGameplayTags::State_Boss_RotationLocked);
		RotationLockTag->ReadyForActivation();
	}
}

void UAgBossPattern_SwordWave::HandleFired(int32 FiredIndex)
{
	WaveIndex = FiredIndex + 1;
	if (RotationLockTag)
	{
		RotationLockTag->EndTask();
		RotationLockTag = nullptr;
	}
	WaitRotationStop();
}

void UAgBossPattern_SwordWave::HandleMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UAgBossPattern_SwordWave::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
