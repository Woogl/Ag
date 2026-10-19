// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_Transition.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "TimerManager.h"

UAbilityTask* FAgTransitionParams::MakeTask(const FAgTaskContext& Context) const
{
	return UAgAbilityTask_Transition::Transition(&Context.Runner, *this, Context.Placement != EAgTaskPlacement::Window,
		Context.MontageInstanceId);
}

UAgAbilityTask_Transition* UAgAbilityTask_Transition::Transition(UGameplayAbility* Owner, const FAgTransitionParams& InParams, bool bInOnce, int32 InMontageInstanceId)
{
	UAgAbilityTask_Transition* Task = NewAbilityTask<UAgAbilityTask_Transition>(Owner);
	Task->Params = InParams;
	Task->bOnce = bInOnce;
	Task->MontageInstanceId = InMontageInstanceId;
	return Task;
}

void UAgAbilityTask_Transition::Activate()
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (!ASC || bOnce)
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
		Watched.AddTag(Params.BeforeTagEnds);
		WatchTags(*ASC, Watched);
		if (!ASC->HasMatchingGameplayTag(Params.BeforeTagEnds))
		{
			HandleTagEnding();
			return;
		}

		// 효과의 지속 시간은 월드 시간이므로 월드 타이머로 기다립니다.
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
		UAnimInstance* Anim = ASC->AbilityActorInfo.IsValid() ? ASC->AbilityActorInfo->GetAnimInstance() : nullptr;
		const FAnimMontageInstance* Instance = Anim ? Anim->GetMontageInstanceForID(MontageInstanceId) : nullptr;
		const UAnimMontage* Montage = Instance ? Instance->Montage.Get() : nullptr;
		const int32 NextSection = Montage ? Montage->GetSectionIndex(Params.Next.Section) : INDEX_NONE;
		const float Delay = Remaining - (NextSection == INDEX_NONE ? 0.f : Montage->GetSectionLength(NextSection));
		if (Delay <= 0.f)
		{
			HandleTagEnding();
			return;
		}
		GetWorld()->GetTimerManager().SetTimer(EndingTimer, FTimerDelegate::CreateUObject(this, &ThisClass::HandleTagEnding), Delay, false);
		return;
	}

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
