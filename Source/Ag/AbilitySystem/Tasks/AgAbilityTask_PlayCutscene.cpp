// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_PlayCutscene.h"

#include "Combat/AgTimeSubsystem.h"
#include "DefaultLevelSequenceInstanceData.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"

UAgAbilityTask_PlayCutscene* UAgAbilityTask_PlayCutscene::PlayCutscene(UGameplayAbility* OwningAbility, ULevelSequence* Cutscene, AActor* Origin)
{
	UAgAbilityTask_PlayCutscene* Task = NewAbilityTask<UAgAbilityTask_PlayCutscene>(OwningAbility);
	Task->Cutscene = Cutscene;
	Task->Origin = Origin;
	return Task;
}

void UAgAbilityTask_PlayCutscene::Activate()
{
	if (!Cutscene)
	{
		HandleFinished();
		return;
	}

	// 시퀀스는 액터의 시간 배율이 아니라 게임 전체 시간 배율을 따르므로, 시간 정지 중에는 재생 속도로 보정합니다.
	FMovieSceneSequencePlaybackSettings Settings;
	Settings.bDisableLookAtInput = true;
	Settings.bDisableMovementInput = true;
	Settings.bHideHud = true;
	Settings.PlayRate = GetWorld()->GetSubsystem<UAgTimeSubsystem>()->GetTimeStopCompensation();
	ALevelSequenceActor* NewActor = nullptr;
	ULevelSequencePlayer* Player = ULevelSequencePlayer::CreateLevelSequencePlayer(this, Cutscene, Settings, NewActor);
	SequenceActor = NewActor;
	SequenceActor->bOverrideInstanceData = true;
	CastChecked<UDefaultLevelSequenceInstanceData>(SequenceActor->DefaultInstanceData)->TransformOriginActor = Origin;
	Player->OnFinished.AddDynamic(this, &ThisClass::HandleFinished);
	Player->Play();
}

void UAgAbilityTask_PlayCutscene::HandleFinished()
{
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnFinished.Broadcast();
	}
	EndTask();
}

void UAgAbilityTask_PlayCutscene::OnDestroy(bool bInOwnerFinished)
{
	if (SequenceActor)
	{
		ULevelSequencePlayer* Player = SequenceActor->GetSequencePlayer();
		Player->OnFinished.RemoveAll(this);
		if (Player->IsPlaying())
		{
			Player->Stop();
		}
		SequenceActor->Destroy();
		SequenceActor = nullptr;
	}
	Super::OnDestroy(bInOwnerFinished);
}
