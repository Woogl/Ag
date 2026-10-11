// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgBossPatternAbility.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/Tasks/AgAbilityTask_AttackMovement.h"
#include "AbilitySystemComponent.h"
#include "Ag.h"
#include "Animation/AgAnimNotifyState_AttackWindow.h"
#include "Animation/AnimMontage.h"
#include "Character/AgBossCharacter.h"
#include "Combat/AgCombatLibrary.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"

UAgBossPatternAbility::UAgBossPatternAbility()
{
	SetAssetTags(FGameplayTagContainer(AgGameplayTags::Ability_Boss_Pattern));

	ActivationBlockedTags.AddTag(AgGameplayTags::State_HitReaction);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Groggy);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Execution_Executed);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(AgGameplayTags::State_NonCombat);
}

FGameplayTag UAgBossPatternAbility::GetPatternTag(const FGameplayAbilitySpec& Spec)
{
	for (const FGameplayTag& Tag : Spec.GetDynamicSpecSourceTags())
	{
		if (Tag.MatchesTag(AgGameplayTags::Ability_Boss_Pattern))
		{
			return Tag;
		}
	}
	return FGameplayTag();
}

const FAgBossPattern* UAgBossPatternAbility::GetPattern() const
{
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	const FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec();
	return (Data && Spec) ? Data->FindPattern(GetPatternTag(*Spec)) : nullptr;
}

AAgBossCharacter* UAgBossPatternAbility::GetBoss() const
{
	return Cast<AAgBossCharacter>(GetAgCharacter());
}

AAgCharacterBase* UAgBossPatternAbility::GetTarget() const
{
	const AAgBossCharacter* Boss = GetBoss();
	return Boss ? Boss->GetTarget() : nullptr;
}

float UAgBossPatternAbility::GetPatternPlayRate() const
{
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	const AAgBossCharacter* Boss = GetBoss();
	return (Data ? Data->PatternPlayRate : 1.f) * (Boss ? Boss->GetPatternSpeed() : 1.f);
}

bool UAgBossPatternAbility::CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	// Called on the class default object too, so everything comes from ActorInfo and the spec.
	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	const AAgCharacterBase* Boss = ActorInfo ? Cast<AAgCharacterBase>(ActorInfo->AvatarActor.Get()) : nullptr;
	const UAgBossData* Data = Boss ? Cast<UAgBossData>(Boss->GetCharacterData()) : nullptr;
	const FGameplayAbilitySpec* Spec = ASC ? ASC->FindAbilitySpecFromHandle(Handle) : nullptr;
	const FGameplayTag CooldownTag = (Data && Spec) ? Data->FindPatternCooldownTag(GetPatternTag(*Spec)) : FGameplayTag();
	if (CooldownTag.IsValid() && ASC->HasMatchingGameplayTag(CooldownTag))
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(CooldownTag);
		}
		return false;
	}
	return Super::CheckCooldown(Handle, ActorInfo, OptionalRelevantTags);
}

void UAgBossPatternAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	const FAgBossPattern* Pattern = GetPattern();
	const FGameplayTag CooldownTag = (Data && Pattern) ? Data->FindPatternCooldownTag(Pattern->Pattern) : FGameplayTag();
	if (CooldownTag.IsValid() && ActorInfo)
	{
		UAgCombatLibrary::ApplyTimedTag(ActorInfo->AbilitySystemComponent.Get(), CooldownTag, Pattern->Cooldown);
	}
}

void UAgBossPatternAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	const FAgBossPattern* Pattern = GetPattern();
	Montage = Pattern ? Data->FindPatternMontage(Pattern->Pattern) : nullptr;
	AAgCharacterBase* Boss = GetAgCharacter();
	if (!Montage || !Boss || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogAg, Verbose, TEXT("Boss pattern %s starts (play rate %.2f)"), *Pattern->Pattern.ToString(), GetPatternPlayRate());
	AAgCharacterBase* Target = GetTarget();
	Boss->SetActiveAttack(Pattern->Hits);
	if (UsesSharedMovement() && Target)
	{
		// 정지 간격을 지키고, 몽타주에 워핑 구간이 있으면(A1·A5 1타 선딜, A7 체공) 그동안 다가갑니다. 다가가는 거리는 모션
		// 자체의 전진에 패턴의 최대 접근 거리까지만 더하고, 방향은 보스가 직접 돌립니다.
		const float MaxTravel = GetForwardTravelBeforeFirstWindow(Montage) + Pattern->MaxApproachDistance;
		UAgAbilityTask_AttackMovement::KeepGap(this, Target, Data->StopDistance, MaxTravel)->ReadyForActivation();
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage, GetPatternPlayRate());
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageFinished);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::HandleMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->ReadyForActivation();

	// 몽타주를 재생하지 못하면 OnCancelled가 그 자리에서 어빌리티를 끝냅니다. 끝난 어빌리티에서 특수 패턴의 이동을 시작하지
	// 않습니다.
	if (!IsActive())
	{
		return;
	}

	OnPatternStarted();
}

float UAgBossPatternAbility::GetForwardTravelBeforeFirstWindow(const UAnimMontage* InMontage)
{
	float FirstWindowStart = InMontage->GetPlayLength();
	for (const FAnimNotifyEvent& Notify : InMontage->Notifies)
	{
		if (Cast<UAgAnimNotifyState_AttackWindow>(Notify.NotifyStateClass))
		{
			FirstWindowStart = FMath::Min(FirstWindowStart, Notify.GetTriggerTime());
		}
	}
	const FTransform RootMotion = InMontage->ExtractRootMotionFromTrackRange(0.f, FirstWindowStart, FAnimExtractContext());
	return FMath::Max(0.f, RootMotion.GetTranslation().Size2D());
}

void UAgBossPatternAbility::HandleMontageFinished()
{
	EndSelf(false);
}

void UAgBossPatternAbility::HandleMontageCancelled()
{
	EndSelf(true);
}

void UAgBossPatternAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (AAgCharacterBase* Boss = GetAgCharacter())
	{
		Boss->ClearActiveAttack();
	}
	Montage = nullptr;

	// 공격 중 이동 태스크는 어빌리티와 함께 끝나며 워프 목표와 정지 간격을 지웁니다.
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
