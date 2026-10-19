// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_ScaleRootMotion.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "Ag.h"

UAbilityTask* FAgScaleRootMotionParams::MakeTask(const FAgTaskContext& Context) const
{
	return UAgAbilityTask_ScaleRootMotion::ScaleRootMotion(&Context.Runner, Distance);
}

UAgAbilityTask_ScaleRootMotion* UAgAbilityTask_ScaleRootMotion::ScaleRootMotion(UGameplayAbility* Owner, float InDistance)
{
	UAgAbilityTask_ScaleRootMotion* Task = NewAbilityTask<UAgAbilityTask_ScaleRootMotion>(Owner);
	Task->Distance = InDistance;
	return Task;
}

void UAgAbilityTask_ScaleRootMotion::Activate()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
	const UAnimMontage* Montage = Ability ? Ability->GetCurrentMontage() : nullptr;
	const float OriginalDistance =
		Montage ? static_cast<float>(
					  Montage->ExtractRootMotionFromTrackRange(0.f, Montage->GetPlayLength(), FAnimExtractContext()).GetTranslation().Size2D())
				: 0.f;
	if (!Character || OriginalDistance <= UE_SMALL_NUMBER)
	{
		UE_LOG(LogAg, Error, TEXT("ScaleRootMotion: %s has no horizontal root motion"), *GetNameSafe(Montage));
		EndTask();
		return;
	}
	Scale = Distance / OriginalDistance;
	Character->SetAnimRootMotionTranslationScale(Scale);
}

void UAgAbilityTask_ScaleRootMotion::OnDestroy(bool bInOwnerFinished)
{
	// 다른 행동이 배율을 바꿨으면 그 값을 둡니다.
	ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
	if (Character && Scale > 0.f && Character->GetAnimRootMotionTranslationScale() == Scale)
	{
		Character->SetAnimRootMotionTranslationScale(1.f);
	}
	Super::OnDestroy(bInOwnerFinished);
}
