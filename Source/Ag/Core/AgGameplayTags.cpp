// Copyright Woogle. All Rights Reserved.

#include "Core/AgGameplayTags.h"

namespace AgGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Attack, "Input.Attack", "기본 공격 키");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Guard, "Input.Guard", "가드 키");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Dodge, "Input.Dodge", "회피·달리기 키");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Jump, "Input.Jump", "점프 키");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Skill, "Input.Skill", "스킬 키");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Ultimate, "Input.Ultimate", "궁극기 키");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Execute, "Input.Execute", "처형 키");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_LockOn, "Input.LockOn", "락온 / 락온 해제 키");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Move, "Input.Move", "이동 입력이 있는 동안");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Action, "Ability.Action", "플레이어의 이동 외 일반 행동");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_BasicAttack, "Ability.Action.BasicAttack");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Dodge, "Ability.Action.Dodge");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_DodgeCounter, "Ability.Action.DodgeCounter");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Guard, "Ability.Action.Guard");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Action_Parry, "Ability.Action.Parry", "패리 모션 (패리 이벤트로 발동, 가드를 끝내지 않음)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Action_PerfectDodge, "Ability.Action.PerfectDodge", "극한 회피 모션 (극한 회피 이벤트로 발동, 회피를 끝냄)");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Skill, "Ability.Action.Skill");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Ultimate, "Ability.Action.Ultimate");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Execution, "Ability.Action.Execution");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_Jump, "Ability.Action.Jump");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Action_JumpAttack, "Ability.Action.JumpAttack");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Sprint, "Ability.Sprint", "달리기. 이동이라 일반 행동(Ability.Action)에 넣지 않음");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_LockOn, "Ability.LockOn", "락온. 행동이 아니라 행동 상태의 제한과 취소를 받지 않음");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Reaction, "Ability.Reaction", "이벤트로 발동하는 상태 어빌리티");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Reaction_HitReaction, "Ability.Reaction.HitReaction",
		"상태 우선순위 4의 피격 반응: 움찔 경직, 넉업(저글 포함), 넉다운, 가드 밀림, 가드 브레이크");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Reaction_HitReaction_Flinch, "Ability.Reaction.HitReaction.Flinch");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Reaction_HitReaction_KnockDown, "Ability.Reaction.HitReaction.KnockDown");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Reaction_HitReaction_KnockUp, "Ability.Reaction.HitReaction.KnockUp");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Reaction_HitReaction_Juggle, "Ability.Reaction.HitReaction.Juggle",
		"넉업으로 떠 있는 동안 움찔에 맞아 다시 조금 떠오름");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Reaction_HitReaction_GuardPushback, "Ability.Reaction.HitReaction.GuardPushback");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Reaction_HitReaction_GuardBreak, "Ability.Reaction.HitReaction.GuardBreak");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Reaction_Groggy, "Ability.Reaction.Groggy");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Reaction_Executed, "Ability.Reaction.Executed", "처형 피격 (그로기를 종료하고 처형 상태로 전환)");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Reaction_PhaseTransition, "Ability.Reaction.PhaseTransition");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Reaction_Parried, "Ability.Reaction.Parried");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Enemy_Pattern, "Ability.Enemy.Pattern", "적 패턴 어빌리티 (패턴을 한꺼번에 취소할 때 씀)");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "상태 우선순위 1: 사망");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Execution, "State.Execution", "상태 우선순위 2: 처형 중 (역할은 실행 문맥으로 구분)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Groggy, "State.Groggy", "상태 우선순위 3: 그로기");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_HitReaction, "State.HitReaction", "상태 우선순위 4의 피격 반응");
	UE_DEFINE_GAMEPLAY_TAG(State_HitReaction_Flinch, "State.HitReaction.Flinch");
	UE_DEFINE_GAMEPLAY_TAG(State_HitReaction_KnockDown, "State.HitReaction.KnockDown");
	UE_DEFINE_GAMEPLAY_TAG(State_HitReaction_KnockUp, "State.HitReaction.KnockUp");
	UE_DEFINE_GAMEPLAY_TAG(State_HitReaction_GuardPushback, "State.HitReaction.GuardPushback");
	UE_DEFINE_GAMEPLAY_TAG(State_HitReaction_GuardBreak, "State.HitReaction.GuardBreak");
	UE_DEFINE_GAMEPLAY_TAG(State_SuperArmor, "State.SuperArmor");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Busy, "State.Busy", "행동 중. 후딜 전까지 다른 행동을 받지 않음");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Acting, "State.Acting", "모션이 있는 플레이어 행동이 실행 중. 그동안 이동 입력을 무시함");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Invincible, "State.Invincible", "무적");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_PerfectDodgeWindow, "State.PerfectDodgeWindow", "극한 회피 구간");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Guarding, "State.Guarding", "가드 중");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_ParryWindow, "State.ParryWindow", "패리 구간");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_DodgeCounterChance, "State.DodgeCounterChance", "회피 반격 기회");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Hitstop, "State.Hitstop", "히트스톱. 캐릭터의 모션과 이동이 멈춤");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Sprinting, "State.Sprinting", "달리는 중");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_LockedOn, "State.LockedOn", "락온 중 (락온 어빌리티가 도는 동안)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Jumping, "State.Jumping", "점프해 공중에 있음 (캐릭터가 점프할 때 붙이고 착지할 때 뗌)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Airborne, "State.Airborne", "이동 모드가 떨어지는 중 (캐릭터가 붙임)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_PassThrough, "State.PassThrough", "캐릭터끼리 통과: 캡슐이 Pawn 채널을 무시함");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_TimeStop, "State.TimeStop", "가진 캐릭터만 빼고 게임 전체의 시간을 멈춤");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dashing, "State.Dashing", "돌진 중 (Dash Task가 도는 동안)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Cutscene, "State.Cutscene", "컷신 재생 중 (PlayCutscene Task가 도는 동안)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_NonCombat, "State.NonCombat", "적이 전투 중이 아님 (비전투 상태, 플레이어 사망 뒤 대기)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Regen_SPDelay, "State.Regen.SPDelay", "SP 리젠 대기");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Regen_PPDelay, "State.Regen.PPDelay", "PP 리젠 대기");

	UE_DEFINE_GAMEPLAY_TAG(Event_Hit_KnockDown, "Event.Hit.KnockDown");
	UE_DEFINE_GAMEPLAY_TAG(Event_Hit_KnockUp, "Event.Hit.KnockUp");
	UE_DEFINE_GAMEPLAY_TAG(Event_Hit_Flinch, "Event.Hit.Flinch");
	UE_DEFINE_GAMEPLAY_TAG(Event_Hit_Stagger, "Event.Hit.Stagger");
	UE_DEFINE_GAMEPLAY_TAG(Event_Guard_Pushback, "Event.Guard.Pushback");
	UE_DEFINE_GAMEPLAY_TAG(Event_Guard_Break, "Event.Guard.Break");
	UE_DEFINE_GAMEPLAY_TAG(Event_Groggy, "Event.Groggy");
	UE_DEFINE_GAMEPLAY_TAG(Event_Executed, "Event.Executed");
	UE_DEFINE_GAMEPLAY_TAG(Event_Death, "Event.Death");
	UE_DEFINE_GAMEPLAY_TAG(Event_PhaseTransition, "Event.PhaseTransition");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Parry, "Event.Parry", "패리함 (피격 처리가 막은 쪽에 보냄, 패리 모션을 시작함)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Parried, "Event.Parried",
		"패리당함 (피격 처리가 근접 공격을 패리당한 공격자와 반사탄에 맞은 대상에 보냄, Instigator는 패리한 캐릭터)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_PerfectDodge, "Event.PerfectDodge",
		"극한 회피함 (피격 처리가 피한 쪽에 보냄, Instigator는 공격한 캐릭터, 극한 회피 모션을 시작함)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_BasicAttackStarted, "Event.BasicAttackStarted",
		"플레이어가 기본 공격의 한 타나 점프 공격을 시작함 (Backstep 검사)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_AttackBlocked, "Event.AttackBlocked",
		"내 공격이 가드·가드 브레이크·패리로 막힘. 그 피해의 효과 컨텍스트를 실음 (DashThrust의 돌진이 멈춤)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Landed, "Event.Landed", "캐릭터가 착지함 (캐릭터가 자기에게 보냄)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Apex, "Event.Apex", "공중에서 수직 속도가 오름에서 내림으로 바뀜 (캐릭터가 자기에게 보냄)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_MoveBlocked, "Event.MoveBlocked", "이동이 벽에 막힘. 턱을 넘지 못한 충돌만 (캐릭터가 자기에게 보냄)");

	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_SP, "SetByCaller.SP");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_MP, "SetByCaller.MP");
	UE_DEFINE_GAMEPLAY_TAG(SetByCaller_UP, "SetByCaller.UP");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_HPMultiplier, "SetByCaller.HPMultiplier", "공격의 HP 배율");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_PPMultiplier, "SetByCaller.PPMultiplier", "공격의 PP 배율");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Damage, "Effect.Damage", "대미지 효과");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Cooldown, "Effect.Cooldown", "행동 쿨다운 효과");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Damage_Reaction_Flinch, "Effect.Damage.Reaction.Flinch", "피격 반응 움찔");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Damage_Reaction_KnockUp, "Effect.Damage.Reaction.KnockUp", "피격 반응 넉업");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Damage_Reaction_KnockDown, "Effect.Damage.Reaction.KnockDown", "피격 반응 넉다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Damage_Unblockable, "Effect.Damage.Unblockable", "가드와 패리로 막을 수 없는 공격 (가드 불가)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Damage_Finisher, "Effect.Damage.Finisher", "마지막 일격 (처형의 마지막 일격, 궁극기 마무리)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Damage_NoAttackerHitstop, "Effect.Damage.NoAttackerHitstop", "적중해도 공격자는 히트스톱으로 멈추지 않는 공격 (보스 GroundSlam 착지 충격파)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Damage_Result_PerfectDodge, "Effect.Damage.Result.PerfectDodge", "피격 결과 극한 회피");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Damage_Result_Parry, "Effect.Damage.Result.Parry", "피격 결과 패리");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Damage_Result_Guard, "Effect.Damage.Result.Guard", "피격 결과 가드");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Damage_Result_GuardBreak, "Effect.Damage.Result.GuardBreak", "피격 결과 가드 브레이크");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Damage_Result_Hit, "Effect.Damage.Result.Hit", "피격 결과 일반 피격");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_UltimateScreen, "GameplayCue.UltimateScreen", "궁극기 포스트프로세스");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_PerfectDodgeScreen, "GameplayCue.PerfectDodgeScreen", "극한 회피 성공 포스트프로세스");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_CameraShake_Small, "GameplayCue.CameraShake.Small", "작은 카메라 셰이크");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_CameraShake_Medium, "GameplayCue.CameraShake.Medium", "보통 카메라 셰이크");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_CameraShake_Large, "GameplayCue.CameraShake.Large", "큰 카메라 셰이크");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Hit, "GameplayCue.Hit", "일반 피격의 이펙트와 효과음");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Guard, "GameplayCue.Guard", "가드·가드 브레이크의 이펙트와 효과음");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Parry, "GameplayCue.Parry", "패리 전용 이펙트와 효과음");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_UltimateSlashes, "GameplayCue.UltimateSlashes",
		"궁극기 베기 선: 켜진 동안 실행마다 하나씩 새기고, 꺼지면 선마다 터뜨린 뒤 모두 지움");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_DamageFloater, "GameplayCue.DamageFloater", "대미지 숫자");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_UnblockableTelegraph, "GameplayCue.UnblockableTelegraph", "가드 불가 예고");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_ParryTelegraph, "GameplayCue.ParryTelegraph", "패리 예고");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_WeaponTrail, "GameplayCue.WeaponTrail", "무기 궤적");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Action_Escape, "UI.Action.Escape", "Esc: 일시정지 메뉴 열기, 팝업·메뉴 닫기");
} // namespace AgGameplayTags
