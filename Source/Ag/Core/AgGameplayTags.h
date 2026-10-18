// Copyright Woogle. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

/** 프로젝트의 모든 게임플레이 태그. 태그 ini 없이 네이티브로만 선언합니다(AGENTS.md 코딩 규칙 3). */
namespace AgGameplayTags
{
	// 입력 태그. 키의 입력 바인딩이 넘깁니다. 가드·달리기·락온은 입력 계층이 트리거 태그로 붙이고, 평타는 실행 중인 평타에
	// 게임플레이 이벤트로 넘깁니다. 이동 입력이 있는 동안은 Input.Move가 붙어 있습니다.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Attack);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Guard);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Dodge);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Jump);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Skill);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ultimate);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Execute);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_LockOn);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Sprint);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Move);

	// 어빌리티를 구분하는 태그. 태그로 어빌리티를 시작하고, 부모 태그로 묶어 취소합니다.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_BasicAttack);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Dodge);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_DodgeCounter);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Guard);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Parry);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Skill);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Ultimate);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Execution);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Jump);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_JumpAttack);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Sprint);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_LockOn);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Reaction);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Reaction_HitReaction);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Reaction_HitReaction_KnockBack);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Reaction_HitReaction_KnockUp);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Reaction_HitReaction_GuardPushback);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Reaction_HitReaction_GuardBreak);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Reaction_Groggy);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Reaction_Executed);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Reaction_PhaseTransition);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Boss_Pattern);

	// 캐릭터가 가진 상태 태그. 전투 시스템 '상태 우선순위'는 Dead > Execution > Groggy > HitReaction 순입니다.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Execution);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Groggy);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_HitReaction);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Busy);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Acting);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Invincible);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_PerfectDodgeWindow);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Guarding);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_ParryWindow);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_DodgeCounterChance);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Hitstop);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Sprinting);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Jumping);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Airborne);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_PassThrough);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_TimeStop);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_GroggyTime);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_NonCombat);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_GroggyImmune);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Regen_SPDelay);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Regen_PPDelay);

	// 게임플레이 이벤트. 어빌리티를 발동하거나, 전투 결과, 이동 사건, 행동의 시점(몽타주 시간표의 SendGameplayEvent)을 알립니다.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Hit_KnockBack);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Hit_KnockUp);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Guard_Pushback);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Guard_Break);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Groggy);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Executed);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Death);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_PhaseTransition);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Parry);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_BasicAttackStarted);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_AttackBlocked);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Landed);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Apex);

	// SetByCaller 값의 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_SP);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_MP);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_UP);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_DamageMultiplier);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_PoiseMultiplier);

	// 대미지 GE 스펙에 다는 태그: 공격의 성질(적용하는 쪽이 담)과 피격 결과(실행 계산이 담)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Damage);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Cooldown);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Damage_Reaction_Flinch);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Damage_Reaction_KnockUp);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Damage_Reaction_KnockBack);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Damage_Unguardable);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Damage_Finisher);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Damage_Result_PerfectDodge);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Damage_Result_Parry);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Damage_Result_Guard);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Damage_Result_GuardBreak);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Damage_Result_Hit);

	// 이펙트, 효과음, 대미지 숫자를 재생하는 게임플레이 큐
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Hit);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Guard);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Parry);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_UltimateFinish);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_UltimateSlashes);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_DamageNumber);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_UnblockableTelegraph);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_ParryTelegraph);
}
