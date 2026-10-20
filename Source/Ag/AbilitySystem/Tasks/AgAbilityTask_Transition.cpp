// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_Transition.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystem/Tasks/AgAbilityTask_PlayMontage.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "TimerManager.h"

UAbilityTask* FAgTransitionParams::MakeTask(const FAgTaskContext& Context) const
{
	return UAgAbilityTask_Transition::Transition(&Context.Ability, *this, Context.Playback);
}

UAgAbilityTask_Transition* UAgAbilityTask_Transition::Transition(UGameplayAbility* Owner, const FAgTransitionParams& InParams, UAgAbilityTask_PlayMontage* InPlayback)
{
	UAgAbilityTask_Transition* Task = NewAbilityTask<UAgAbilityTask_Transition>(Owner);
	Task->Params = InParams;
	Task->Playback = InPlayback;
	return Task;
}

void UAgAbilityTask_Transition::Activate()
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (ASC && Params.Event.IsValid())
	{
		EventHandle = ASC->GenericGameplayEventCallbacks.FindOrAdd(Params.Event).AddUObject(this, &ThisClass::HandleEvent);
		return;
	}
	if (ASC && Params.Tag.IsValid() && ASC->HasMatchingGameplayTag(Params.Tag))
	{
		TagHandle = ASC->RegisterGameplayTagEvent(Params.Tag, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::HandleTagChanged);
		const TOptional<float> Delay = Params.bSectionEndsWithTag ? FindTagEndingDelay(*ASC) : TOptional<float>();
		if (Delay && *Delay <= 0.f)
		{
			TryTransition();
		}
		else if (Delay)
		{
			// 효과의 지속 시간은 월드 시간이므로 월드 타이머로 기다립니다.
			GetWorld()->GetTimerManager().SetTimer(EndingTimer, FTimerDelegate::CreateUObject(this, &ThisClass::TryTransition), *Delay, false);
		}
		return;
	}
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
		if (TagHandle.IsValid())
		{
			ASC->RegisterGameplayTagEvent(Params.Tag, EGameplayTagEventType::NewOrRemoved).Remove(TagHandle);
		}
	}
	EventHandle.Reset();
	TagHandle.Reset();
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EndingTimer);
	}
	Super::OnDestroy(bInOwnerFinished);
}

TOptional<float> UAgAbilityTask_Transition::FindTagEndingDelay(const UAbilitySystemComponent& ASC) const
{
	const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(Params.Tag));
	const TArray<float> Times = ASC.GetActiveEffectsTimeRemaining(Query);
	if (Times.IsEmpty() || Times.ContainsByPredicate([](float Time) { return Time < 0.f; }))
	{
		return {};
	}
	const UAnimMontage* Montage = Ability ? Ability->GetCurrentMontage() : nullptr;
	const int32 SectionIndex = Montage ? Montage->GetSectionIndex(Params.Section) : INDEX_NONE;
	const float SectionLength = SectionIndex == INDEX_NONE ? 0.f : Montage->GetSectionLength(SectionIndex);
	return FMath::Max(Times) - SectionLength;
}

void UAgAbilityTask_Transition::TryTransition()
{
	const UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (IsFinished())
	{
		return;
	}
	const bool bMet =
		ASC && ASC->HasAllMatchingGameplayTags(Params.RequiredTags) && !ASC->HasAnyMatchingGameplayTags(Params.BlockedTags);
	if (bMet && Playback.IsValid())
	{
		Playback->RequestTransition(Params.Section, Params.BlendTime);
	}
	// 이벤트를 기다리는 전환만 조건이 맞지 않을 때 다음 이벤트를 기다립니다.
	if (bMet || !Params.Event.IsValid())
	{
		EndTask();
	}
}

void UAgAbilityTask_Transition::HandleEvent(const FGameplayEventData* Payload)
{
	TryTransition();
}

void UAgAbilityTask_Transition::HandleTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount == 0)
	{
		TryTransition();
	}
}
