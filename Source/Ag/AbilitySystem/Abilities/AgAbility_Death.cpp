// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_Death.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/Abilities/AgAbility_Groggy.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Character/AgCharacterBase.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"

namespace
{
	/** 래그돌이 마지막 공격의 피격 방향으로 밀려나는 속도(전투 시스템 '사망 처리' 6). 기획서에 수치가 없는 구현 값입니다. */
	constexpr float DeathPushSpeed = 300.f;

	/** 처형 사망 몽타주의 ID (캐릭터 데이터의 Montages) */
	const FName ExecutedDeathMontageId(TEXT("ExecutedDeath"));
}

UAgAbility_Death::UAgAbility_Death()
{
	SetAssetTags(FGameplayTagContainer(AgGameplayTags::Ability_Reaction_Death));
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Dead);

	FAbilityTriggerData& Trigger = AbilityTriggers.AddDefaulted_GetRef();
	Trigger.TriggerTag = AgGameplayTags::Event_Death;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
}

void UAgAbility_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AAgCharacterBase* Character = GetAgCharacter();

	// 처형 사망: 처형 사망 모션은 마지막 일격 뒤까지 처형 피격 모션과 같아, 같은 재생 위치에서 튀지 않고 이어집니다.
	// 래그돌은 모션이 끝날 때 바꿉니다. 처형 피격 중에 처형 타격으로 죽었을 때만 이렇게 하고, 처형 피격 모션 중 다른
	// 공격으로 죽으면 일반 사망입니다.
	const UAgCharacterData* Data = Character->GetCharacterData();
	const UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
	const bool bKilledByExecution = TriggerEventData && TriggerEventData->InstigatorTags.HasTagExact(AgGameplayTags::Ability_Action_Execution)
		&& GetAbilitySystemComponentFromActorInfo()->HasMatchingGameplayTag(AgGameplayTags::State_Execution_Executed);
	UAnimMontage* ExecutedMontage = bKilledByExecution ? Data->FindMontage(UAgAbility_Groggy::ExecutedMontageId) : nullptr;
	UAnimMontage* ExecutedDeathMontage = bKilledByExecution ? Data->FindMontage(ExecutedDeathMontageId) : nullptr;
	if (ExecutedMontage && ExecutedDeathMontage && AnimInstance)
	{
		const float Position = AnimInstance->Montage_GetPosition(ExecutedMontage);
		Character->Die(FVector::ZeroVector, this, /*bRagdoll*/ false);

		UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, ExecutedDeathMontage, 1.f, NAME_None, /*bStopWhenAbilityEnds*/ false, 1.f, Position);
		Task->OnCompleted.AddDynamic(this, &ThisClass::HandleExecutedDeathEnded);
		Task->OnBlendOut.AddDynamic(this, &ThisClass::HandleExecutedDeathEnded);
		Task->OnInterrupted.AddDynamic(this, &ThisClass::HandleExecutedDeathEnded);
		Task->OnCancelled.AddDynamic(this, &ThisClass::HandleExecutedDeathEnded);
		Task->ReadyForActivation();
		return;
	}

	// 마지막 공격의 피격 방향으로 밀려납니다. 공격자가 없으면 밀리지 않습니다.
	const AActor* Attacker = TriggerEventData ? TriggerEventData->Instigator.Get() : nullptr;
	FVector Push = FVector::ZeroVector;
	if (Attacker)
	{
		Push = (Character->GetActorLocation() - Attacker->GetActorLocation()).GetSafeNormal2D() * DeathPushSpeed;
	}
	Character->Die(Push, this);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UAgAbility_Death::HandleExecutedDeathEnded()
{
	GetAgCharacter()->StartRagdoll(FVector::ZeroVector);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
