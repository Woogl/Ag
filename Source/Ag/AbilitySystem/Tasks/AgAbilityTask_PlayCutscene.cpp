// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_PlayCutscene.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Core/AgGameplayTags.h"
#include "DefaultLevelSequenceInstanceData.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"

namespace
{
	// 레벨 시퀀스에서 재생하는 캐릭터를 받는 바인딩에 붙인 태그와 같아야 합니다.
	const FName OwnerBindingTag(TEXT("Owner"));
}

UAbilityTask* FAgPlayCutsceneParams::MakeTask(const FAgTaskContext& Context) const
{
	UAgAbilityTask_PlayCutscene* Task = UAbilityTask::NewAbilityTask<UAgAbilityTask_PlayCutscene>(&Context.Ability);
	Task->Params = *this;
	return Task;
}

UAgAbilityTask_PlayCutscene::UAgAbilityTask_PlayCutscene()
{
	bTickingTask = true;
}

void UAgAbilityTask_PlayCutscene::Activate()
{
	if (!Params.Sequence || !GetAvatarActor() || !AbilitySystemComponent.IsValid())
	{
		EndTask();
		return;
	}
	bTagAdded = true;
	AbilitySystemComponent->AddLooseGameplayTag(AgGameplayTags::State_Cutscene);

	FMovieSceneSequencePlaybackSettings Settings;
	Settings.bDisableLookAtInput = true;
	Settings.bDisableMovementInput = true;
	Settings.bHideHud = true;
	Settings.PlayRate = GetOwnerTimeDilation();
	ALevelSequenceActor* NewActor = nullptr;
	ULevelSequencePlayer* Player = ULevelSequencePlayer::CreateLevelSequencePlayer(this, Params.Sequence, Settings, NewActor);
	if (!Player || !NewActor)
	{
		EndTask();
		return;
	}
	SequenceActor = NewActor;
	SequenceActor->bOverrideInstanceData = true;
	CastChecked<UDefaultLevelSequenceInstanceData>(SequenceActor->DefaultInstanceData)->TransformOriginActor = GetAvatarActor();
	SequenceActor->SetBindingByTag(OwnerBindingTag, {GetAvatarActor()});
	Player->OnFinished.AddDynamic(this, &ThisClass::HandleFinished);
	Player->Play();
}

void UAgAbilityTask_PlayCutscene::TickTask(float DeltaTime)
{
	// 시간 정지가 이 구간보다 먼저 시작하든 나중에 시작하든 같은 속도가 되도록 매 프레임 맞춥니다.
	ULevelSequencePlayer* Player = SequenceActor ? SequenceActor->GetSequencePlayer() : nullptr;
	if (Player && Player->IsPlaying())
	{
		Player->SetPlayRate(GetOwnerTimeDilation());
	}
}

void UAgAbilityTask_PlayCutscene::OnDestroy(bool bInOwnerFinished)
{
	if (SequenceActor)
	{
		if (ULevelSequencePlayer* Player = SequenceActor->GetSequencePlayer())
		{
			Player->OnFinished.RemoveAll(this);
			if (Player->IsPlaying())
			{
				Player->Stop();
			}
		}
		SequenceActor->Destroy();
		SequenceActor = nullptr;
	}
	if (bTagAdded && AbilitySystemComponent.IsValid())
	{
		// 태그를 떼면 Tag Transition이 섹션을 옮기며 이 Task를 다시 끝내므로 먼저 내립니다.
		bTagAdded = false;
		AbilitySystemComponent->RemoveLooseGameplayTag(AgGameplayTags::State_Cutscene);
	}
	Super::OnDestroy(bInOwnerFinished);
}

void UAgAbilityTask_PlayCutscene::HandleFinished()
{
	EndTask();
}

float UAgAbilityTask_PlayCutscene::GetOwnerTimeDilation() const
{
	const AActor* Owner = GetAvatarActor();
	return Owner ? Owner->CustomTimeDilation : 1.f;
}
