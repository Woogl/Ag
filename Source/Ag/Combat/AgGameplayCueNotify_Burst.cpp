// Copyright Woogle. All Rights Reserved.

#include "Combat/AgGameplayCueNotify_Burst.h"

#include "Camera/AgPlayerCameraManager.h"
#include "Character/AgPlayerCharacter.h"
#include "Core/AgGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"

namespace AgGameplayCueNotify_BurstPrivate
{
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

		const bool bHeavy = Parameters.AggregatedTargetTags.HasTagExact(AgGameplayTags::Event_Hit_KnockUp) ||
		                    Parameters.AggregatedTargetTags.HasTagExact(AgGameplayTags::Event_Hit_KnockDown);
		CameraManager.PlayCameraReaction(bHeavy ? EAgCameraSituation::PlayerHitHeavy : EAgCameraSituation::PlayerHit, Target);
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
}

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
		const bool bHeavy = Parameters.AggregatedTargetTags.HasTagExact(AgGameplayTags::Event_Hit_KnockUp) ||
		                    Parameters.AggregatedTargetTags.HasTagExact(AgGameplayTags::Event_Hit_KnockDown);
		CameraManager->PlayCameraReaction(bHeavy ? EAgCameraSituation::PlayerHurtHeavy : EAgCameraSituation::PlayerHurt, Attacker);
	}
	else if (CueTag == AgGameplayTags::GameplayCue_Guard)
	{
		EAgCameraSituation Situation = EAgCameraSituation::Guard;
		if (Parameters.AggregatedSourceTags.HasTagExact(AgGameplayTags::Effect_Damage_Result_GuardBreak))
		{
			Situation = EAgCameraSituation::GuardBreak;
		}
		else if (Parameters.AggregatedTargetTags.HasTagExact(AgGameplayTags::Event_Guard_Pushback))
		{
			Situation = EAgCameraSituation::GuardPushback;
		}
		CameraManager->PlayCameraReaction(Situation, Attacker);
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
	if (MagnitudeNiagaraParameter.IsNone())
	{
		const bool bResult = Super::OnExecute_Implementation(Target, Parameters);
		PlayCombatCameraReactions(Target, Parameters);
		return bResult;
	}

	// 엔진은 생성한 효과를 BP 전용 OnBurst에만 넘기므로, 부모의 실행 순서를 따라 부르며 그 사이에 값을 넣습니다.
	FGameplayCueNotify_SpawnContext SpawnContext(Target ? Target->GetWorld() : GetWorld(), Target, Parameters);
	SpawnContext.SetDefaultSpawnCondition(&DefaultSpawnCondition);
	SpawnContext.SetDefaultPlacementInfo(&DefaultPlacementInfo);
	if (DefaultSpawnCondition.ShouldSpawn(SpawnContext))
	{
		FGameplayCueNotify_SpawnResult SpawnResult;
		BurstEffects.ExecuteEffects(SpawnContext, SpawnResult);
		for (UFXSystemComponent* FX : SpawnResult.FxSystemComponents)
		{
			if (UNiagaraComponent* Niagara = Cast<UNiagaraComponent>(FX))
			{
				Niagara->SetVariableFloat(MagnitudeNiagaraParameter, Parameters.RawMagnitude);
			}
		}
		OnBurst(Target, Parameters, SpawnResult);
	}
	PlayCombatCameraReactions(Target, Parameters);
	return false;
}
