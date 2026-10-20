// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_AreaHit.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystem/AgTargeting.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Character/AgCharacterBase.h"
#include "Data/AgEffectData.h"

UAbilityTask* FAgAreaHitParams::MakeTask(const FAgTaskContext& Context) const
{
	return UAgAbilityTask_AreaHit::AreaHit(&Context.Ability, *this);
}

UAgAbilityTask_AreaHit* UAgAbilityTask_AreaHit::AreaHit(UGameplayAbility* Owner, const FAgAreaHitParams& InParams)
{
	UAgAbilityTask_AreaHit* Task = NewAbilityTask<UAgAbilityTask_AreaHit>(Owner);
	Task->Params = InParams;
	return Task;
}

void UAgAbilityTask_AreaHit::Activate()
{
	AAgCharacterBase* Character = Cast<AAgCharacterBase>(GetAvatarActor());
	UAgGameplayAbility* AgAbility = Cast<UAgGameplayAbility>(Ability);
	if (Character && !Character->IsDead() && AgAbility && AgAbility->IsActive())
	{
		for (AActor* Target : AgTargeting::FindTargets(Params.Area, *Character))
		{
			UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
			const FGameplayEffectSpecHandle Spec = AgAbility->MakeEffectSpec(EAgEffect::Damage);
			if (TargetASC && Spec.IsValid())
			{
				Spec.Data->GetContext().AddOrigin(Target->GetActorLocation());
				Params.Damage.AddToSpec(*Spec.Data);
				AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);
			}
			// 피해가 행동을 끝냈으면 엔진이 이 Task도 끝냈습니다.
			if (IsFinished() || !AgAbility->IsActive())
			{
				break;
			}
		}
	}
	EndTask();
}
