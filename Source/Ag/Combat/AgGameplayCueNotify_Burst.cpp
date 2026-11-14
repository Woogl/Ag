// Copyright Woogle. All Rights Reserved.

#include "Combat/AgGameplayCueNotify_Burst.h"

#include "Camera/AgPlayerCameraManager.h"
#include "Character/AgPlayerCharacter.h"
#include "Core/AgGameplayTags.h"
#include "Kismet/GameplayStatics.h"

namespace AgGameplayCueNotify_BurstPrivate
{
	bool IsHeavyHit(const FGameplayCueParameters& Parameters)
	{
		return Parameters.AggregatedTargetTags.HasTagExact(AgGameplayTags::Event_Hit_KnockUp) ||
		       Parameters.AggregatedTargetTags.HasTagExact(AgGameplayTags::Event_Hit_KnockDown);
	}

	void PlayAttackCameraReactions(AAgPlayerCameraManager& CameraManager, const AActor* Target, const FGameplayCueParameters& Parameters)
	{
		const FGameplayTagContainer& Tags = Parameters.AggregatedSourceTags;
		if (Tags.HasTagExact(AgGameplayTags::Ability_Action_Execution))
		{
			const bool bFinisher = Tags.HasTagExact(AgGameplayTags::Effect_Damage_Finisher);
			CameraManager.PlayCameraReaction(bFinisher ? EAgCameraSituation::ExecutionBlow : EAgCameraSituation::ExecutionStab, Target);
			return;
		}
		if (Parameters.OriginalTag != AgGameplayTags::GameplayCue_Hit)
		{
			return;
		}

		CameraManager.PlayCameraReaction(IsHeavyHit(Parameters) ? EAgCameraSituation::PlayerHitHeavy : EAgCameraSituation::PlayerHit, Target);
		if (Tags.HasTagExact(AgGameplayTags::Ability_Action_DodgeCounter))
		{
			CameraManager.PlayCameraReaction(EAgCameraSituation::DodgeCounterHit, Target);
		}
		else if (Tags.HasTagExact(AgGameplayTags::Ability_Action_GuardCounter))
		{
			CameraManager.PlayCameraReaction(EAgCameraSituation::GuardCounterHit, Target);
		}
		else if (Tags.HasTagExact(AgGameplayTags::Ability_Action_Skill_1))
		{
			CameraManager.PlayCameraReaction(EAgCameraSituation::BlinkRushHit, Target);
		}
	}
} // namespace AgGameplayCueNotify_BurstPrivate

void UAgGameplayCueNotify_Burst::PlayCombatCameraReactions(AActor* Target, const FGameplayCueParameters& Parameters)
{
	const FGameplayTag CueTag = Parameters.OriginalTag;
	if (CueTag != AgGameplayTags::GameplayCue_Hit && CueTag != AgGameplayTags::GameplayCue_Guard &&
		CueTag != AgGameplayTags::GameplayCue_Parry && CueTag != AgGameplayTags::GameplayCue_PerfectDodge)
	{
		return;
	}

	const AActor* Attacker = Parameters.GetInstigator();
	const AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(Attacker);
	if (!Player)
	{
		Player = Cast<AAgPlayerCharacter>(Target);
	}
	AAgPlayerCameraManager* CameraManager = Player ? Cast<AAgPlayerCameraManager>(UGameplayStatics::GetPlayerCameraManager(Player, 0)) : nullptr;
	if (!CameraManager)
	{
		return;
	}

	if (Player == Attacker)
	{
		AgGameplayCueNotify_BurstPrivate::PlayAttackCameraReactions(*CameraManager, Target, Parameters);
	}
	else if (CueTag == AgGameplayTags::GameplayCue_Hit)
	{
		const bool bHeavy = AgGameplayCueNotify_BurstPrivate::IsHeavyHit(Parameters);
		CameraManager->PlayCameraReaction(bHeavy ? EAgCameraSituation::PlayerHurtHeavy : EAgCameraSituation::PlayerHurt, Attacker);
	}
	else if (CueTag == AgGameplayTags::GameplayCue_Guard)
	{
		const bool bPushback = Parameters.AggregatedTargetTags.HasTagExact(AgGameplayTags::Event_Guard_Pushback);
		CameraManager->PlayCameraReaction(bPushback ? EAgCameraSituation::GuardPushback : EAgCameraSituation::Guard, Attacker);
	}
	else if (CueTag == AgGameplayTags::GameplayCue_Parry)
	{
		CameraManager->PlayCameraReaction(EAgCameraSituation::Parry, Attacker);
	}
	else if (CueTag == AgGameplayTags::GameplayCue_PerfectDodge)
	{
		CameraManager->PlayCameraReaction(EAgCameraSituation::PerfectDodge, Attacker);
	}
}

bool UAgGameplayCueNotify_Burst::OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) const
{
	const bool bResult = Super::OnExecute_Implementation(Target, Parameters);
	PlayCombatCameraReactions(Target, Parameters);
	return bResult;
}
