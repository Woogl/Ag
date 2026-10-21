// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_PlayMontage.h"

#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/AgCharacterBase.h"
#include "Core/AgAssetManager.h"
#include "Data/AgCharacterData.h"
#include "Data/AgMontageTimeline.h"
#include "Ag.h"

namespace
{
	UAnimInstance* FindAnimInstance(const UGameplayAbility* Ability)
	{
		const FGameplayAbilityActorInfo* Info = Ability ? Ability->GetCurrentActorInfo() : nullptr;
		return Info ? Info->GetAnimInstance() : nullptr;
	}
}

UAbilityTask* FAgPlayMontageParams::MakeTask(const FAgTaskContext& Context) const
{
	UAgAbilityTask_PlayMontage* Task = UAbilityTask::NewAbilityTask<UAgAbilityTask_PlayMontage>(&Context.Ability);
	Task->Params = *this;
	return Task;
}

UAgAbilityTask_PlayMontage::UAgAbilityTask_PlayMontage()
{
	// ASC는 Task를 TG_DuringPhysics에 틱하므로, 몽타주가 진행된 뒤(TG_PrePhysics의 이동·메시 틱) 판정해 같은 프레임의
	// 공격 판정(TG_PostPhysics)보다 먼저 구간을 엽니다.
	bTickingTask = true;
}

void UAgAbilityTask_PlayMontage::Activate()
{
	AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor());
	const UAgGameplayAbility* AgAbility = Cast<UAgGameplayAbility>(Ability);
	const UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (!Character || !AgAbility || !ASC || !ASC->HasAllMatchingGameplayTags(Params.RequiredTags) ||
		ASC->HasAnyMatchingGameplayTags(Params.BlockedTags))
	{
		EndTask();
		return;
	}
	Montage = Character->GetCharacterData()->FindMontage(Params.Montage);
	if (!Montage)
	{
		UE_LOG(LogAg, Error, TEXT("Abilities.%s: montage %s is missing in the character's Montages"), *AgAbility->GetAbilityId().ToString(),
			*Params.Montage.ToString());
		CancelAbility();
		return;
	}
	Play(Params.Section, -1.f);
}

void UAgAbilityTask_PlayMontage::TickTask(float DeltaTime)
{
	if (TimelineEntries.IsEmpty() || IsFinished())
	{
		return;
	}
	UAnimInstance* Anim = FindAnimInstance(Ability);
	const FAnimMontageInstance* Instance = Anim ? Anim->GetMontageInstanceForID(MontageInstanceId) : nullptr;
	if (!Instance)
	{
		EndTimeline();
		return;
	}
	EvaluateTimeline(Instance->GetPosition(), EAgTimelineMode::Advance);
}

void UAgAbilityTask_PlayMontage::OnDestroy(bool bInOwnerFinished)
{
	// 행동이 끝나 엔진이 끝내면 남은 Task도 엔진이 끝내므로, 스스로 끝날 때만 시간표 Task를 끝냅니다.
	if (!bInOwnerFinished)
	{
		EndTimeline();
		EndPointTasks();
	}
	StopTimeline();
	PointTasks.Reset();

	const int32 InstanceId = MontageInstanceId;
	MontageInstanceId = INDEX_NONE;
	UAnimInstance* Anim = InstanceId != INDEX_NONE ? FindAnimInstance(Ability) : nullptr;
	FAnimMontageInstance* Instance = Anim ? Anim->GetMontageInstanceForID(InstanceId) : nullptr;
	if (Instance && Instance->Montage && !Instance->IsStopped())
	{
		Instance->OnMontageBlendingOutStarted.Unbind();
		Instance->OnMontageEnded.Unbind();
		Instance->Stop(FAlphaBlend(Instance->Montage->BlendOut));
	}
	Super::OnDestroy(bInOwnerFinished);
}

void UAgAbilityTask_PlayMontage::RequestTransition(FName Section, const TOptional<float>& BlendTime)
{
	if (IsFinished() || Section.IsNone() || MontageInstanceId == INDEX_NONE)
	{
		return;
	}
	if (!BlendTime)
	{
		JumpToSection(Section);
		return;
	}
	if (UAgGameplayAbility* AgAbility = Cast<UAgGameplayAbility>(Ability))
	{
		AgAbility->RefreshTarget();
	}
	Play(Section, *BlendTime);
}

void UAgAbilityTask_PlayMontage::Play(FName Section, float BlendTime)
{
	UAgAbilitySystemComponent* ASC = Cast<UAgAbilitySystemComponent>(AbilitySystemComponent.Get());
	if (!ASC)
	{
		CancelAbility();
		return;
	}

	// 끝나는 구간이 요청하는 전환은 이전 재생의 것이므로 받지 않도록 재생 ID를 먼저 비웁니다.
	const int32 PreviousInstanceId = MontageInstanceId;
	MontageInstanceId = INDEX_NONE;
	EndTimeline();
	if (IsFinished())
	{
		return;
	}
	FAnimMontageInstance* Instance = ASC->PlayMontageWithBlendIn(*Ability, *Montage, BlendTime, Section);
	if (IsFinished())
	{
		// 이전 재생이 끊기며 부른 콜백이 이 행동을 끝냈습니다. 새 재생은 ASC가 멈췄습니다.
		return;
	}
	if (!Instance)
	{
		UE_LOG(LogAg, Error, TEXT("PlayMontage: failed to play %s (section %s)"), *Montage->GetName(), *Section.ToString());
		// 이전 재생이 남아 있으면 Task가 끝날 때 멈추도록 다시 맡습니다.
		MontageInstanceId = PreviousInstanceId;
		CancelAbility();
		return;
	}
	const int32 InstanceId = Instance->GetInstanceID();
	MontageInstanceId = InstanceId;
	Instance->OnMontageBlendingOutStarted =
		FOnMontageBlendingOutStarted::CreateUObject(this, &ThisClass::HandleMontageBlendingOut, InstanceId);
	Instance->OnMontageEnded = FOnMontageEnded::CreateUObject(this, &ThisClass::HandleMontageEnded, InstanceId);

	// 시작 위치의 판정에서 전환하거나 행동이 끝날 수 있으므로 마지막에 합니다.
	BeginTimeline(*Instance);
}

void UAgAbilityTask_PlayMontage::JumpToSection(FName Section)
{
	UAnimInstance* Anim = FindAnimInstance(Ability);
	const FAnimMontageInstance* Instance =
		Anim && MontageInstanceId != INDEX_NONE ? Anim->GetMontageInstanceForID(MontageInstanceId) : nullptr;
	if (!Instance)
	{
		return;
	}

	// 엔진은 블렌드 아웃 중인 재생을 옮기지 않으므로 위치가 바뀌었을 때만 시간표를 다시 맞춥니다.
	const float Before = Instance->GetPosition();
	Anim->Montage_JumpToSection(Section, Instance->Montage);
	if (!IsFinished() && Instance->GetPosition() != Before)
	{
		EvaluateTimeline(Instance->GetPosition(), EAgTimelineMode::Jump);
	}
}

void UAgAbilityTask_PlayMontage::CancelAbility()
{
	// 블루프린트에서 PlayMontageAndWait의 OnInterrupted를 CancelAbility에 잇는 것과 같습니다.
	if (Ability && Ability->IsActive())
	{
		Ability->CancelAbility(Ability->GetCurrentAbilitySpecHandle(), Ability->GetCurrentActorInfo(), Ability->GetCurrentActivationInfo(), true);
	}
	EndTask();
}

void UAgAbilityTask_PlayMontage::HandleMontageBlendingOut(UAnimMontage* EndedMontage, bool bInterrupted, int32 InstanceId)
{
	if (bInterrupted && OwnsMontage(InstanceId))
	{
		FinishPlayback(true);
	}
}

void UAgAbilityTask_PlayMontage::HandleMontageEnded(UAnimMontage* EndedMontage, bool bInterrupted, int32 InstanceId)
{
	if (OwnsMontage(InstanceId))
	{
		FinishPlayback(bInterrupted);
	}
}

void UAgAbilityTask_PlayMontage::FinishPlayback(bool bInterrupted)
{
	// 끝나는 구간이 요청하는 전환을 받지 않도록 재생 ID를 먼저 비웁니다.
	MontageInstanceId = INDEX_NONE;
	if (bInterrupted)
	{
		CancelAbility();
		return;
	}
	EndTask();
}

void UAgAbilityTask_PlayMontage::BeginTimeline(const FAnimMontageInstance& Instance)
{
	UAnimMontage* PlayedMontage = Instance.Montage;
	const FAgMontageTimeline* Timeline = UAgAssetManager::Get().GetMontageTimelines().Find(PlayedMontage);
	if (!Timeline)
	{
		return;
	}

	// 데이터를 다시 읽어도 이 재생이 끝날 때까지 같은 항목을 쓰도록 복사합니다.
	for (const TPair<FName, FAgTimelineSection>& Section : Timeline->Sections)
	{
		for (const TInstancedStruct<FAgTaskParams>& Item : Section.Value.Tasks)
		{
			const FAgTaskParams& ItemParams = Item.Get();
			float Start = 0.f;
			float End = 0.f;
			if (!AgMontageTimeline::ResolveTimes(*PlayedMontage, Section.Key, ItemParams, Start, End))
			{
				UE_LOG(LogAg, Error, TEXT("%s: section %s or %s of the timeline is missing in the montage"), *GetNameSafe(PlayedMontage),
					*Section.Key.ToString(), *ItemParams.EndSection.ToString());
				continue;
			}
			FAgTimelineEntry& Entry = TimelineEntries.AddDefaulted_GetRef();
			Entry.Params = Item;
			Entry.bWindow = ItemParams.Start.IsSet();
			Entry.Start = Start;
			Entry.End = End;
		}
	}
	TimelinePosition = Instance.GetPosition();
	EvaluateTimeline(TimelinePosition, EAgTimelineMode::Jump);
}

void UAgAbilityTask_PlayMontage::EvaluateTimeline(float NewPosition, EAgTimelineMode Mode)
{
	enum class EPhase : uint8
	{
		Begin,
		End,
		Point
	};
	struct FPending
	{
		float Time = 0.f;
		EPhase Phase = EPhase::Point;
		int32 Entry = INDEX_NONE;
	};
	TArray<FPending> Pending;

	// 재생 시작과 섹션 이동의 위치는 섹션 시작 시각 그대로라 섹션 0초 지점의 시각과 정확히 같습니다.
	const float Previous = TimelinePosition;
	const bool bForward = Mode == EAgTimelineMode::Advance && NewPosition >= Previous;
	for (int32 Index = 0; Index < TimelineEntries.Num(); ++Index)
	{
		const FAgTimelineEntry& Entry = TimelineEntries[Index];
		if (!Entry.bWindow)
		{
			const bool bReached = bForward ? (Previous < Entry.Start && Entry.Start <= NewPosition)
			                               : (Mode == EAgTimelineMode::Jump && Entry.Start == NewPosition);
			if (bReached)
			{
				Pending.Add(FPending{Entry.Start, EPhase::Point, Index});
			}
			continue;
		}
		if (bForward && !Entry.bActive && Previous < Entry.Start && Entry.Start <= NewPosition)
		{
			Pending.Add(FPending{Entry.Start, EPhase::Begin, Index});
			if (Entry.End <= NewPosition)
			{
				Pending.Add(FPending{Entry.End, EPhase::End, Index});
			}
			continue;
		}
		if (bForward && Entry.bActive && Entry.End <= NewPosition)
		{
			Pending.Add(FPending{Entry.End, EPhase::End, Index});
			continue;
		}
		const bool bInside = Entry.Start <= NewPosition && NewPosition < Entry.End;
		if (Entry.bActive != bInside)
		{
			Pending.Add(FPending{NewPosition, bInside ? EPhase::Begin : EPhase::End, Index});
		}
	}
	TimelinePosition = NewPosition;
	Pending.Sort([](const FPending& A, const FPending& B)
	{
		return A.Time < B.Time;
	});

	const int32 InstanceId = MontageInstanceId;
	for (const FPending& Item : Pending)
	{
		// 항목 실행이 재생을 끝내거나 바꿀 수 있으므로 매번 확인합니다. 이 Task가 시작하는 중(첫 판정)에는 행동이 끝나도
		// 엔진이 이 Task를 끝내지 않으므로 행동도 봅니다. 항목 실행 안에서 전환해 위치가 바뀌었으면 옮긴 위치의 판정이
		// 이미 끝났으므로 남은 항목은 실행하지 않습니다. 그래서 전환 지점과 같은 시각에는 다른 항목을 두지 않습니다.
		if (IsFinished() || !Ability->IsActive() || MontageInstanceId != InstanceId || TimelinePosition != NewPosition)
		{
			return;
		}
		FAgTimelineEntry& Entry = TimelineEntries[Item.Entry];
		switch (Item.Phase)
		{
			case EPhase::Begin:
				Entry.bActive = true;
				StartTimelineWindow(Item.Entry);
				break;
			case EPhase::End:
				Entry.bActive = false;
				EndTimelineWindow(Item.Entry);
				break;
			case EPhase::Point:
				RunTimelinePoint(Item.Entry);
				break;
		}
	}
}

void UAgAbilityTask_PlayMontage::EndTimeline()
{
	// Task가 끝나며 다시 재생해도 버린 시간표에 닿지 않도록, 켜진 구간의 Task를 모아 시간표를 버린 뒤 끝냅니다.
	TArray<UAbilityTask*> Active;
	for (const FAgTimelineEntry& Entry : TimelineEntries)
	{
		if (Entry.Task)
		{
			Active.Add(Entry.Task);
		}
	}
	StopTimeline();
	for (UAbilityTask* Task : Active)
	{
		Task->EndTask();
	}
}

void UAgAbilityTask_PlayMontage::StopTimeline()
{
	TimelineEntries.Reset();
	TimelinePosition = 0.f;
}

void UAgAbilityTask_PlayMontage::StartTimelineWindow(int32 Entry)
{
	FAgTimelineEntry& Item = TimelineEntries[Entry];
	UAbilityTask* Task = Item.Params.Get().MakeTask(FAgTaskContext{*CastChecked<UAgGameplayAbility>(Ability), Item.Start, Item.End, this});
	if (!Task)
	{
		return;
	}
	// 시작하는 동안 전환하면 시간표가 바뀔 수 있으므로 항목에는 시작하기 전에 둡니다.
	Item.Task = Task;
	Task->ReadyForActivation();
}

void UAgAbilityTask_PlayMontage::EndTimelineWindow(int32 Entry)
{
	UAbilityTask* Task = TimelineEntries[Entry].Task;
	TimelineEntries[Entry].Task = nullptr;
	if (Task)
	{
		Task->EndTask();
	}
}

void UAgAbilityTask_PlayMontage::RunTimelinePoint(int32 Entry)
{
	// 지점 Task는 다시 재생해도 남고, 재생이 끝날 때 끝납니다.
	const FAgTimelineEntry& Item = TimelineEntries[Entry];
	UAbilityTask* Task = Item.Params.Get().MakeTask(FAgTaskContext{*CastChecked<UAgGameplayAbility>(Ability), Item.Start, Item.Start, this});
	if (!Task)
	{
		return;
	}
	PointTasks.Add(Task);
	Task->ReadyForActivation();
}

void UAgAbilityTask_PlayMontage::EndPointTasks()
{
	const TArray<TObjectPtr<UAbilityTask>> Tasks = MoveTemp(PointTasks);
	for (UAbilityTask* Task : Tasks)
	{
		if (Task)
		{
			Task->EndTask();
		}
	}
}
