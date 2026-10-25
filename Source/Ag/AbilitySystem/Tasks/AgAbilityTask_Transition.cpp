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
	UAgAbilityTask_Transition* Task = UAbilityTask::NewAbilityTask<UAgAbilityTask_Transition>(&Context.Ability);
	Task->Params = *this;
	Task->Playback = Context.Playback;
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
	if (ASC && Params.Tag.IsValid())
	{
		// 같은 시각에 시작한 항목이 아직 태그를 붙이지 않았을 수 있어, 없으면 다음 틱에 다시 봅니다.
		if (!WaitForTag())
		{
			EndingTimer = GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ThisClass::RecheckTag));
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
	// 섹션 길이는 몽타주 시간이라, 재생 배율로 나눠 효과의 지속 시간과 같은 월드 시간으로 맞춥니다.
	const float SectionLength = SectionIndex == INDEX_NONE ? 0.f : Montage->GetSectionLength(SectionIndex);
	const float PlayRate = FMath::Abs(Montage ? Montage->RateScale : 1.f);
	return FMath::Max(Times) - (PlayRate > UE_SMALL_NUMBER ? SectionLength / PlayRate : SectionLength);
}

bool UAgAbilityTask_Transition::WaitForTag()
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (!ASC || !ASC->HasMatchingGameplayTag(Params.Tag))
	{
		return false;
	}
	TagHandle = ASC->RegisterGameplayTagEvent(Params.Tag, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::HandleTagChanged);
	const TOptional<float> Delay = Params.bSectionEndsWithTag ? FindTagEndingDelay(*ASC) : NullOpt;
	if (Delay)
	{
		if (*Delay <= 0.f)
		{
			TryTransition();
		}
		else
		{
			// 효과의 지속 시간은 월드 시간이므로 월드 타이머로 기다립니다.
			GetWorld()->GetTimerManager().SetTimer(EndingTimer, FTimerDelegate::CreateUObject(this, &ThisClass::TryTransition), *Delay, false);
		}
	}
	return true;
}

void UAgAbilityTask_Transition::RecheckTag()
{
	if (!IsFinished() && !WaitForTag())
	{
		TryTransition();
	}
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
		if (Params.BlendTime)
		{
			Playback->ReplaySection(Params.Section, *Params.BlendTime);
		}
		else
		{
			Playback->JumpToSection(Params.Section);
		}
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
