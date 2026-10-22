// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_AreaHit.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystem/AgGameplayEffectContext.h"
#include "AbilitySystem/AgTargeting.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Character/AgCharacterBase.h"

UAbilityTask* FAgAreaHitParams::MakeTask(const FAgTaskContext& Context) const
{
	UAgAbilityTask_AreaHit* Task = UAbilityTask::NewAbilityTask<UAgAbilityTask_AreaHit>(&Context.Ability);
	Task->Params = *this;
	return Task;
}

void UAgAbilityTask_AreaHit::Activate()
{
	AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor());
	const UAgGameplayAbility* AgAbility = CastChecked<UAgGameplayAbility>(Ability);
	if (Character)
	{
		const TSharedPtr<FAgHitRewardState> HitReward = FAgHitRewardState::Make(Params.HitReward);
		for (AActor* Target : AgTargeting::FindTargets(Params.Area, *Character))
		{
			UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
			const FGameplayEffectSpecHandle Spec = AgAbility->MakeDamageSpec(Params.Damage, HitReward);
			if (TargetASC && Spec.IsValid())
			{
				Spec.Data->GetContext().AddOrigin(Target->GetActorLocation());
				AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);
			}
			// 피해가 행동을 끝냈으면 남은 대상은 치지 않습니다.
			if (IsFinished() || !AgAbility->IsActive())
			{
				break;
			}
		}
	}
	EndTask();
}
