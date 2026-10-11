// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgBossPattern_Common.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/Tasks/AgAbilityTask_AttackMovement.h"
#include "Ag.h"
#include "Animation/AnimMontage.h"
#include "Character/AgBossCharacter.h"
#include "Data/AgCharacterData.h"

UAgBossPattern_Common::UAgBossPattern_Common()
{
	SetupBossPattern();
}

void UAgBossPattern_Common::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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

	UE_LOG(LogAg, Verbose, TEXT("Boss pattern %s starts (play rate %.2f)"), *Pattern->Pattern.ToString(), Boss->GetPatternPlayRate());
	Boss->SetActiveAttack(Pattern->Hits);

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage, Boss->GetPatternPlayRate());
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageFinished);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::HandleMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->ReadyForActivation();

	// 몽타주를 재생하지 못하면 OnCancelled가 그 자리에서 어빌리티를 끝냅니다. 공격 중 이동은 재생 중인 몽타주에서 접근 상한을
	// 잽니다. 보스는 타겟팅 범위가 없고, 몽타주에 워핑 구간이 있는 타(A1·A5 1타 선딜, A7 체공)만 다가갑니다.
	if (IsActive())
	{
		UAgAbilityTask_AttackMovement::Attack(this, Pattern->Hits.Num(), /*InTargetingRange*/ -1.f, Pattern->MaxApproachDistance)->ReadyForActivation();
	}
}

void UAgBossPattern_Common::HandleMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UAgBossPattern_Common::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UAgBossPattern_Common::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (AAgCharacterBase* Boss = GetAgCharacter())
	{
		Boss->ClearActiveAttack();
	}

	// 공격 중 이동 태스크는 어빌리티와 함께 끝나며 워프 목표, 정지 간격, 회전 대상을 지웁니다.
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
