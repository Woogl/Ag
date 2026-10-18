// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_PlayCutscene.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "Data/AgJsonRead.h"
#include "DefaultLevelSequenceInstanceData.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"

void FAgPlayCutsceneParams::GetFieldRules(FAgJsonFieldRules& Rules) const
{
	Rules.Optional.Add(TEXT("Next.BlendTime"));
}

UAbilityTask* FAgPlayCutsceneParams::MakeTask(const FAgTaskContext& Context) const
{
	return UAgAbilityTask_PlayCutscene::PlayCutscene(&Context.Runner, *this, Context.MontageInstanceId);
}

UAgAbilityTask_PlayCutscene::UAgAbilityTask_PlayCutscene()
{
	bTickingTask = true;
}

UAgAbilityTask_PlayCutscene* UAgAbilityTask_PlayCutscene::PlayCutscene(UGameplayAbility* Owner, const FAgPlayCutsceneParams& Params, int32 InMontageInstanceId)
{
	UAgAbilityTask_PlayCutscene* Task = NewAbilityTask<UAgAbilityTask_PlayCutscene>(Owner);
	Task->Cutscene = Params.Sequence;
	Task->Next = Params.Next;
	Task->MontageInstanceId = InMontageInstanceId;
	return Task;
}

void UAgAbilityTask_PlayCutscene::Activate()
{
	if (!Cutscene || !GetAvatarActor())
	{
		HandleFinished();
		return;
	}

	FMovieSceneSequencePlaybackSettings Settings;
	Settings.bDisableLookAtInput = true;
	Settings.bDisableMovementInput = true;
	Settings.bHideHud = true;
	Settings.PlayRate = GetOwnerTimeDilation();
	ALevelSequenceActor* NewActor = nullptr;
	ULevelSequencePlayer* Player = ULevelSequencePlayer::CreateLevelSequencePlayer(this, Cutscene, Settings, NewActor);
	if (!Player || !NewActor)
	{
		HandleFinished();
		return;
	}
	SequenceActor = NewActor;
	SequenceActor->bOverrideInstanceData = true;
	CastChecked<UDefaultLevelSequenceInstanceData>(SequenceActor->DefaultInstanceData)->TransformOriginActor = GetAvatarActor();
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
	Super::OnDestroy(bInOwnerFinished);
}

void UAgAbilityTask_PlayCutscene::HandleFinished()
{
	if (IsFinished())
	{
		return;
	}
	// 전환이 이 구간을 끝냅니다. 전환하지 않으면(다른 재생) 스스로 끝냅니다.
	if (UAgGameplayAbility* Runner = Cast<UAgGameplayAbility>(Ability))
	{
		Runner->RequestTransition(Next, MontageInstanceId);
	}
	if (!IsFinished())
	{
		EndTask();
	}
}

float UAgAbilityTask_PlayCutscene::GetOwnerTimeDilation() const
{
	const AActor* Owner = GetAvatarActor();
	return Owner ? Owner->CustomTimeDilation : 1.f;
}
