// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgBossPattern_Common.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/Tasks/AgAbilityTask_AttackMovement.h"
#include "Ag.h"
#include "Animation/AgAnimNotifyState_AttackWindow.h"
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
	if (AAgCharacterBase* Target = Boss->GetTarget())
	{
		// 정지 간격을 지키고, 몽타주에 워핑 구간이 있으면(A1·A5 1타 선딜, A7 체공) 그동안 다가갑니다. 다가가는 거리는 모션
		// 자체의 전진에 패턴의 최대 접근 거리까지만 더하고, 방향은 보스가 직접 돌립니다.
		const float MaxTravel = UAgAnimNotifyState_AttackWindow::GetTravelBeforeFirstWindow(Montage) + Pattern->MaxApproachDistance;
		UAgAbilityTask_AttackMovement::KeepGap(this, Target, Data->StopDistance, MaxTravel)->ReadyForActivation();
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage, Boss->GetPatternPlayRate());
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageFinished);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::HandleMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UAgBossPattern_Common::HandleMontageFinished()
{
	EndSelf(false);
}

void UAgBossPattern_Common::HandleMontageCancelled()
{
	EndSelf(true);
}

void UAgBossPattern_Common::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (AAgCharacterBase* Boss = GetAgCharacter())
	{
		Boss->ClearActiveAttack();
	}

	// 공격 중 이동 태스크는 어빌리티와 함께 끝나며 워프 목표와 정지 간격을 지웁니다.
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
