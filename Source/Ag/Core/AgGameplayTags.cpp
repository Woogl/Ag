// Copyright Woogle. All Rights Reserved.

#include "Core/AgGameplayTags.h"

namespace AgGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Attack, "Input.Attack", "평타 키");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Guard, "Input.Guard", "가드 키");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Dodge, "Input.Dodge", "회피·달리기 키");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Jump, "Input.Jump", "점프 키");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Skill, "Input.Skill", "스킬 키");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Ultimate, "Input.Ultimate", "궁극기 키");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Execute, "Input.Execute", "처형 키");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_GuardReleased, "Input.GuardReleased", "가드 키를 뗌");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_LockOn, "Input.LockOn", "락온 / 락온 해제 키");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_DodgeReleased, "Input.DodgeReleased", "회피·달리기 키를 뗌");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Action, "Ability.Action", "플레이어의 일반 행동");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_BasicAttack, "Ability.Action.BasicAttack");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Dodge, "Ability.Action.Dodge");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_DodgeCounter, "Ability.Action.DodgeCounter");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Guard, "Ability.Action.Guard");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Skill, "Ability.Action.Skill");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Ultimate, "Ability.Action.Ultimate");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Execution, "Ability.Action.Execution");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Sprint, "Ability.Action.Sprint");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Jump, "Ability.Action.Jump");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Reaction, "Ability.Reaction", "이벤트로 발동하는 상태 어빌리티");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Reaction_KnockBack, "Ability.Reaction.KnockBack");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Reaction_Down, "Ability.Reaction.Down");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Reaction_GuardPushback, "Ability.Reaction.GuardPushback");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Reaction_GuardBreak, "Ability.Reaction.GuardBreak");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Reaction_Groggy, "Ability.Reaction.Groggy");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Reaction_Executed, "Ability.Reaction.Executed");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Reaction_PhaseTransition, "Ability.Reaction.PhaseTransition");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Reaction_Death, "Ability.Reaction.Death");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Pattern, "Ability.Boss.Pattern", "보스 패턴");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Pattern_A1, "Ability.Boss.Pattern.A1", "2연격");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Pattern_A2, "Ability.Boss.Pattern.A2", "휩쓸기");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Pattern_A3, "Ability.Boss.Pattern.A3", "돌진 찌르기");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Pattern_A4, "Ability.Boss.Pattern.A4", "도약 내려찍기");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Pattern_A5, "Ability.Boss.Pattern.A5", "3연격");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Pattern_A6, "Ability.Boss.Pattern.A6", "검기 발사");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Boss_Pattern_B1, "Ability.Boss.Pattern.B1", "백스텝");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "상태 우선순위 1: 사망");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Execution_Executing, "State.Execution.Executing", "상태 우선순위 2: 처형하는 중");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Execution_Executed, "State.Execution.Executed", "상태 우선순위 2: 처형당하는 중");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Groggy, "State.Groggy", "상태 우선순위 3: 그로기");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_HitReaction, "State.HitReaction", "상태 우선순위 4: 넉백, 다운, 가드 밀림, 가드 브레이크");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Busy, "State.Busy", "행동 중. 후딜 전까지 다른 행동을 받지 않음");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Acting, "State.Acting", "행동 모션 재생 중 (후딜 포함). 이동 입력을 무시함");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Invincible, "State.Invincible", "무적");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_PerfectDodgeWindow, "State.PerfectDodgeWindow", "극한 회피 구간");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Guarding, "State.Guarding", "가드 중");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_ParryWindow, "State.ParryWindow", "패리 구간");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_SuperArmor, "State.SuperArmor", "슈퍼아머");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_DodgeCounterChance, "State.DodgeCounterChance", "회피 반격 기회");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Sprinting, "State.Sprinting", "달리는 중");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_InAir, "State.InAir", "공중");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_NonCombat, "State.NonCombat", "비전투 (플레이어 사망 뒤의 보스)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Regen_SPDelay, "State.Regen.SPDelay", "SP 리젠 대기");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Regen_PPDelay, "State.Regen.PPDelay", "PP 리젠 대기");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Boss_RotationLocked, "State.Boss.RotationLocked", "보스 회전 정지");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Boss_PatternRecovery, "State.Boss.PatternRecovery", "패턴 후딜레이");

	UE_DEFINE_GAMEPLAY_TAG(Event_Hit_KnockBack, "Event.Hit.KnockBack");
	UE_DEFINE_GAMEPLAY_TAG(Event_Hit_Down, "Event.Hit.Down");
	UE_DEFINE_GAMEPLAY_TAG(Event_Guard_Pushback, "Event.Guard.Pushback");
	UE_DEFINE_GAMEPLAY_TAG(Event_Guard_Break, "Event.Guard.Break");
	UE_DEFINE_GAMEPLAY_TAG(Event_Groggy, "Event.Groggy");
	UE_DEFINE_GAMEPLAY_TAG(Event_Executed, "Event.Executed");
	UE_DEFINE_GAMEPLAY_TAG(Event_Death, "Event.Death");
	UE_DEFINE_GAMEPLAY_TAG(Event_PhaseTransition, "Event.PhaseTransition");
	UE_DEFINE_GAMEPLAY_TAG(Event_PerfectDodge, "Event.PerfectDodge");
	UE_DEFINE_GAMEPLAY_TAG(Event_Parry, "Event.Parry");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_BasicAttackStarted, "Event.BasicAttackStarted", "플레이어가 평타의 한 타를 시작함 (B1 백스텝 검사)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_AttackWindowEnded, "Event.AttackWindowEnded", "판정 구간이 끝남. EventMagnitude는 몇 번째 타인지 (0부터)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_DownGrounded, "Event.DownGrounded", "다운 모션에서 바닥에 쓰러진 순간 (무적 시작)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_RecoveryStarted, "Event.RecoveryStarted", "모션의 후딜이 시작됨 (판정 구간이 없는 행동: 패리 모션)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_ExecutionBlow, "Event.ExecutionBlow", "처형 모션의 마지막 일격 (대미지 적용)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Boss_DashStart, "Event.Boss.DashStart", "A3 찌르기 모션에서 몸을 앞으로 내던지는 순간 (돌진 시작)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Boss_DashHold, "Event.Boss.DashHold", "A3 창을 다 내지른 자세 (돌진이 끝날 때까지 유지)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Boss_SwordWave, "Event.Boss.SwordWave", "A6 검기 발사 (판정 구간으로 봄)");

	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Dodge, "Cooldown.Dodge");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Skill, "Cooldown.Skill");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Boss_A2, "Cooldown.Boss.A2");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Boss_A3, "Cooldown.Boss.A3");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Boss_A4, "Cooldown.Boss.A4");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Boss_A5, "Cooldown.Boss.A5");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Boss_A6, "Cooldown.Boss.A6");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Boss_B1, "Cooldown.Boss.B1");

	UE_DEFINE_GAMEPLAY_TAG(Data_HP, "Data.HP");
	UE_DEFINE_GAMEPLAY_TAG(Data_PP, "Data.PP");
	UE_DEFINE_GAMEPLAY_TAG(Data_SP, "Data.SP");
	UE_DEFINE_GAMEPLAY_TAG(Data_MP, "Data.MP");
	UE_DEFINE_GAMEPLAY_TAG(Data_UP, "Data.UP");
	UE_DEFINE_GAMEPLAY_TAG(Data_Duration, "Data.Duration");
	UE_DEFINE_GAMEPLAY_TAG(Data_Rate, "Data.Rate");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Hit, "GameplayCue.Hit", "피격 이펙트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Parry, "GameplayCue.Parry", "패리 전용 이펙트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_UnblockableFlash, "GameplayCue.UnblockableFlash", "붉은 섬광");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_CameraShake_Small, "GameplayCue.CameraShake.Small", "카메라 셰이크 작음");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_CameraShake_Medium, "GameplayCue.CameraShake.Medium", "카메라 셰이크 보통");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_CameraShake_Large, "GameplayCue.CameraShake.Large", "카메라 셰이크 큼");
}
