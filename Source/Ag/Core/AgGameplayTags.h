// Copyright Woogle. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

/**
 * Every gameplay tag in the project, declared natively (AGENTS.md coding rule 3).
 * Tag ini files are not used.
 */
namespace AgGameplayTags
{
	// Input tags. An ability activates when its input tag is pressed.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Attack);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Guard);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Dodge);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Jump);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Skill);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ultimate);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Execute);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_GuardReleased);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_LockOn);

	// Ability identity tags, used to cancel or block abilities by group.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_BasicAttack);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Dodge);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_DodgeCounter);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Guard);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Skill);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Ultimate);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Execution);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Sprint);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Jump);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Reaction);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Reaction_KnockBack);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Reaction_Down);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Reaction_GuardPushback);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Reaction_GuardBreak);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Reaction_Groggy);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Reaction_Executed);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Reaction_PhaseTransition);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Reaction_Death);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Pattern);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Pattern_A1);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Pattern_A2);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Pattern_A3);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Pattern_A4);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Pattern_A5);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Pattern_A6);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Pattern_B1);

	// State tags owned by a character. 상태 우선순위 (전투 시스템) maps onto Dead > Execution > Groggy > HitReaction.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Execution_Executing);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Execution_Executed);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Groggy);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_HitReaction);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Busy);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Acting);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Invincible);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_PerfectDodgeWindow);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Guarding);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_ParryWindow);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_SuperArmor);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_DodgeCounterChance);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Sprinting);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_InAir);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_NonCombat);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Regen_SPDelay);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Regen_PPDelay);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Boss_RotationLocked);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Boss_PatternRecovery);

	// Gameplay events that start state abilities or report combat results.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Hit_KnockBack);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Hit_Down);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Guard_Pushback);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Guard_Break);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Groggy);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Executed);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Death);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_PhaseTransition);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_PerfectDodge);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Parry);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_BasicAttackStarted);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_AttackWindowEnded);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_DownGrounded);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_RecoveryStarted);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_ExecutionBlow);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Boss_DashStart);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Boss_DashHold);

	// Cooldown tags granted by cooldown GameplayEffects.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Dodge);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Skill);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Boss_A2);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Boss_A3);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Boss_A4);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Boss_A5);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Boss_A6);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Boss_B1);

	// SetByCaller magnitudes.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_HP);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_PP);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_SP);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_MP);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_UP);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Duration);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Rate);

	// GameplayCues for effects and camera shakes.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Hit);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Parry);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_UnblockableFlash);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_CameraShake_Small);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_CameraShake_Medium);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_CameraShake_Large);
}
