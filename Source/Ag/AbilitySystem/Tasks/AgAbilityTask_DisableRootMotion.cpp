// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_DisableRootMotion.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

UAbilityTask* FAgDisableRootMotionParams::MakeTask(const FAgTaskContext& Context) const
{
	return UAgAbilityTask_DisableRootMotion::DisableRootMotion(&Context.Runner, Context.MontageInstanceId);
}

UAgAbilityTask_DisableRootMotion* UAgAbilityTask_DisableRootMotion::DisableRootMotion(UGameplayAbility* Owner, int32 InMontageInstanceId)
{
	UAgAbilityTask_DisableRootMotion* Task = NewAbilityTask<UAgAbilityTask_DisableRootMotion>(Owner);
	Task->MontageInstanceId = InMontageInstanceId;
	return Task;
}

void UAgAbilityTask_DisableRootMotion::Activate()
{
	FAnimMontageInstance* Instance = FindInstance();
	if (!Instance)
	{
		EndTask();
		return;
	}
	Instance->PushDisableRootMotion();
	bDisabled = true;
}

void UAgAbilityTask_DisableRootMotion::OnDestroy(bool bInOwnerFinished)
{
	// 그 재생이 아직 행동의 재생일 때만 되돌립니다. 다시 재생하거나, 끊기거나, 행동이 끝나 내려놓은 재생은 블렌드 아웃으로 멈추므로
	// 그동안에도 끈 채로 둡니다. 카운터는 인스턴스와 함께 사라집니다.
	const UAgGameplayAbility* Runner = Cast<UAgGameplayAbility>(Ability);
	FAnimMontageInstance* Instance = bDisabled && Runner && Runner->OwnsMontage(MontageInstanceId) ? FindInstance() : nullptr;
	if (Instance)
	{
		Instance->PopDisableRootMotion();
	}
	bDisabled = false;
	Super::OnDestroy(bInOwnerFinished);
}

FAnimMontageInstance* UAgAbilityTask_DisableRootMotion::FindInstance() const
{
	const FGameplayAbilityActorInfo* ActorInfo = Ability ? Ability->GetCurrentActorInfo() : nullptr;
	UAnimInstance* Anim = ActorInfo ? ActorInfo->GetAnimInstance() : nullptr;
	return Anim && MontageInstanceId != INDEX_NONE ? Anim->GetMontageInstanceForID(MontageInstanceId) : nullptr;
}
