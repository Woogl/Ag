// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_Transition.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Data/AgJsonRead.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "TimerManager.h"

void FAgTransitionParams::GetFieldRules(FAgJsonFieldRules& Rules) const
{
	Rules.Optional.Add(GET_MEMBER_NAME_CHECKED(FAgTransitionParams, RequiredTags));
	Rules.Optional.Add(GET_MEMBER_NAME_CHECKED(FAgTransitionParams, BlockedTags));
	Rules.Optional.Add(GET_MEMBER_NAME_CHECKED(FAgTransitionParams, Event));
	Rules.Optional.Add(GET_MEMBER_NAME_CHECKED(FAgTransitionParams, BeforeTagEnds));
	Rules.Optional.Add(GET_MEMBER_NAME_CHECKED(FAgTransitionParams, bOptional));
	Rules.Optional.Add(TEXT("Next.BlendTime"));
}

void FAgTransitionParams::Validate(const FJsonObject& Json, EAgTaskPlacement Placement, const FString& Path, TArray<FString>& Errors)
{
	const bool bEvent = Json.HasField(TEXT("Event"));
	const bool bTagEnds = Json.HasField(TEXT("BeforeTagEnds"));
	if (Placement == EAgTaskPlacement::Point && (bEvent || bTagEnds || Json.HasField(TEXT("bOptional"))))
	{
		Errors.Add(Path + TEXT(": a point uses no Event, BeforeTagEnds or bOptional"));
	}
	if (Placement == EAgTaskPlacement::Window && bEvent && bTagEnds)
	{
		Errors.Add(Path + TEXT(": use Event or BeforeTagEnds, not both"));
	}
	if (Placement == EAgTaskPlacement::Window && !bEvent && !bTagEnds && RequiredTags.IsEmpty() && BlockedTags.IsEmpty())
	{
		Errors.Add(Path + TEXT(": a window without Event or BeforeTagEnds needs RequiredTags or BlockedTags"));
	}
}

UAbilityTask* FAgTransitionParams::MakeTask(const FAgTaskContext& Context) const
{
	return UAgAbilityTask_Transition::Transition(&Context.Runner, *this, Context.Placement == EAgTaskPlacement::Point,
		Context.MontageInstanceId);
}

UAgAbilityTask_Transition* UAgAbilityTask_Transition::Transition(UGameplayAbility* Owner, const FAgTransitionParams& InParams, bool bInPoint, int32 InMontageInstanceId)
{
	UAgAbilityTask_Transition* Task = NewAbilityTask<UAgAbilityTask_Transition>(Owner);
	Task->Params = InParams;
	Task->bPoint = bInPoint;
	Task->MontageInstanceId = InMontageInstanceId;
	return Task;
}

void UAgAbilityTask_Transition::Activate()
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (!ASC || bPoint)
	{
		TryTransition();
		if (!IsFinished())
		{
			EndTask();
		}
		return;
	}
	if (Params.Event.IsValid())
	{
		EventHandle = ASC->GenericGameplayEventCallbacks.FindOrAdd(Params.Event).AddUObject(this, &ThisClass::HandleEvent);
		return;
	}
	FGameplayTagContainer Watched = Params.RequiredTags;
	Watched.AppendTags(Params.BlockedTags);
	if (Params.BeforeTagEnds.IsValid())
	{
		// 태그가 먼저 사라지면(효과가 지워짐) 바로 전환하고, 그때 조건이 맞지 않았으면 조건이 맞게 될 때 전환하도록 함께 지켜봅니다.
		Watched.AddTag(Params.BeforeTagEnds);
		WatchTags(*ASC, Watched);
		if (!ASC->HasMatchingGameplayTag(Params.BeforeTagEnds))
		{
			HandleTagEnding();
			return;
		}

		// 효과의 지속 시간은 월드 시간이라, 남은 시간에서 Next 섹션 길이를 뺀 때를 월드 타이머로 기다립니다. 끝나는 때를 모르는 태그
		// (무한 효과, 효과가 주지 않은 태그)는 사라질 때를 기다립니다.
		const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(Params.BeforeTagEnds));
		const TArray<float> Times = ASC->GetActiveEffectsTimeRemaining(Query);
		bool bFinite = !Times.IsEmpty();
		float Remaining = 0.f;
		for (const float Time : Times)
		{
			bFinite &= Time >= 0.f;
			Remaining = FMath::Max(Remaining, Time);
		}
		if (!bFinite)
		{
			return;
		}
		const UAgGameplayAbility* Runner = Cast<UAgGameplayAbility>(Ability);
		const float Delay = Remaining - (Runner ? Runner->GetSectionLength(Params.Next.Section) : 0.f);
		if (Delay <= 0.f)
		{
			HandleTagEnding();
			return;
		}
		GetWorld()->GetTimerManager().SetTimer(EndingTimer, FTimerDelegate::CreateUObject(this, &ThisClass::HandleTagEnding), Delay, false);
		return;
	}

	// 조건이 맞게 되는 때: 조건 태그가 바뀔 때마다 확인합니다.
	WatchTags(*ASC, Watched);
	TryTransition();
}

void UAgAbilityTask_Transition::OnDestroy(bool bInOwnerFinished)
{
	if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
	{
		if (EventHandle.IsValid())
		{
			ASC->GenericGameplayEventCallbacks.FindOrAdd(Params.Event).Remove(EventHandle);
		}
		for (const TPair<FGameplayTag, FDelegateHandle>& Pair : TagHandles)
		{
			ASC->RegisterGameplayTagEvent(Pair.Key, EGameplayTagEventType::NewOrRemoved).Remove(Pair.Value);
		}
	}
	EventHandle.Reset();
	TagHandles.Reset();
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EndingTimer);
	}
	Super::OnDestroy(bInOwnerFinished);
}

bool UAgAbilityTask_Transition::AreConditionsMet() const
{
	const UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	return ASC && ASC->HasAllMatchingGameplayTags(Params.RequiredTags) && !ASC->HasAnyMatchingGameplayTags(Params.BlockedTags);
}

void UAgAbilityTask_Transition::WatchTags(UAbilitySystemComponent& ASC, const FGameplayTagContainer& Tags)
{
	for (const FGameplayTag& Tag : Tags)
	{
		const FDelegateHandle Handle =
			ASC.RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::HandleTagChanged);
		TagHandles.Emplace(Tag, Handle);
	}
}

void UAgAbilityTask_Transition::TryTransition()
{
	UAgGameplayAbility* Runner = Cast<UAgGameplayAbility>(Ability);
	if (Runner && !IsFinished() && AreConditionsMet())
	{
		// 전환이 이 구간을 끝내므로 요청한 뒤에는 이 Task를 쓰지 않습니다.
		Runner->RequestTransition(Params.Next, MontageInstanceId);
	}
}

void UAgAbilityTask_Transition::HandleEvent(const FGameplayEventData* Payload)
{
	TryTransition();
}

void UAgAbilityTask_Transition::HandleTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	// BeforeTagEnds면 그 태그가 사라질 때와, 전환할 때가 된 뒤 조건 태그가 바뀔 때 확인합니다.
	if (!Params.BeforeTagEnds.IsValid())
	{
		TryTransition();
	}
	else if (Tag == Params.BeforeTagEnds)
	{
		if (NewCount == 0)
		{
			HandleTagEnding();
		}
	}
	else if (bTagEnding)
	{
		TryTransition();
	}
}

void UAgAbilityTask_Transition::HandleTagEnding()
{
	bTagEnding = true;
	TryTransition();
}
