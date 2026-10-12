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
	AAgBossCharacter* Boss = CastChecked<AAgBossCharacter>(GetAgCharacter());
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	const FAgBossPattern* Pattern = Boss->FindPattern(Handle);
	UAnimMontage* Montage = Pattern ? Data->FindPatternMontage(Pattern->Id) : nullptr;
	if (!Montage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogAg, Verbose, TEXT("Boss pattern %s starts"), *Pattern->Id.ToString());

	// 물러나는 거리: 모션의 루트 모션 전체 이동에 배율을 걸어 Backstep 거리에 맞춥니다(보스 사양 'Backstep'). 몽타주 태스크가 재생할 때
	// 배율을 걸고, 몽타주가 블렌드 아웃을 시작하면 1로 되돌립니다.
	const float MotionDistance = Montage->ExtractRootMotionFromTrackRange(0.f, Montage->GetPlayLength(), FAnimExtractContext()).GetTranslation().Size2D();
	const float RootMotionScale = MotionDistance > UE_KINDA_SMALL_NUMBER ? Data->BackstepDistance / MotionDistance : 1.f;
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage, 1.f, NAME_None, /*bStopWhenAbilityEnds*/ true, RootMotionScale);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UAgBossPattern_Backstep::HandleMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UAgBossPattern_Backstep::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
