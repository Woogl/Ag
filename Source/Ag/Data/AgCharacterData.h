// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/AgAttackData.h"
#include "Engine/DataAsset.h"
#include "Math/Interval.h"
#include "AgCharacterData.generated.h"

class AAgSwordWave;
class UAgGameplayAbility;
class UAgEffect_Regen;
class UAnimMontage;
class UAnimSequenceBase;
class ULevelSequence;
class UBlendSpace;
class UGameplayAbility;
class UStateTree;
class UTargetingPreset;

/** The '스탯' tables of 플레이어 사양 and 보스 사양: starting value and maximum of each stat. */
USTRUCT(BlueprintType)
struct FAgStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = 0))
	float HP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = 0))
	float MaxHP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = 0))
	float SP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = 0))
	float MaxSP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = 0))
	float MP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = 0))
	float MaxMP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = 0))
	float UP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = 0))
	float MaxUP = 0.f;

	/** Ignored when bInfinitePP is set. */
	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = 0, EditCondition = "!bInfinitePP"))
	float PP = 0.f;

	/** Ignored when bInfinitePP is set. */
	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = 0, EditCondition = "!bInfinitePP"))
	float MaxPP = 0.f;

	/** PP 무한: PP never decreases, so the character never becomes groggy. */
	UPROPERTY(EditAnywhere, Category = "Stats")
	bool bInfinitePP = false;

	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = 0))
	float ATK = 0.f;

	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = 0))
	float DEF = 0.f;

	/** 기본 이동 속도 */
	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = 0, Units = "cm/s"))
	float MOV = 0.f;
};

/**
 * 플레이어와 보스가 함께 쓰는 데이터(구현 계획서 '데이터').
 * 데이터 에셋(DA_Player, DA_Boss)에는 에셋 참조와 기획서에 없는 값(구현 값, 어빌리티, 소켓과 본)을 저장합니다.
 * Transient 필드는 기획자가 조절하는 기획 값이고, 에셋이 로드될 때마다 에셋 매니저가 ValuesFile의 JSON에서 채웁니다.
 * 필드마다 기획서의 어느 항목인지는 데이터 위치표(Docs/Program/데이터 위치표.md)를 따릅니다.
 */
UCLASS(Abstract, BlueprintType)
class UAgCharacterData : public UDataAsset
{
	GENERATED_BODY()

public:
	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditUndo() override;
#endif

	/** Data file in the project's Data folder that fills the Transient fields (Player.json, Boss.json). */
	UPROPERTY(EditDefaultsOnly, Category = "Data")
	FString ValuesFile;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Stats", meta = (ShowOnlyInnerProperties))
	FAgStats Stats;

	/** PP 리젠: 초당 회복량 after the regen delay (전투 시스템 'PP 리젠'). 0 for no regen. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Stats", meta = (ClampMin = 0))
	float PPRegenRate = 0.f;

	/** 사망 연출 시간. 끝나면 결과 UI를 띄웁니다. (플레이어 사양·보스 사양 '사망') */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Death", meta = (Units = "s", ClampMin = 0))
	float DeathPresentationTime = 0.f;

	/** 공격 중 이동: 정지 간격 (캡슐 중심 사이 수평 거리) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Attack Movement", meta = (Units = "cm", ClampMin = 0))
	float StopDistance = 0.f;

	/** 공격 중 이동: 회전 정지. 각 판정 구간이 시작되기 이만큼 전부터 그 판정 구간이 끝날 때까지 돌지 않습니다. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Attack Movement", meta = (Units = "s", ClampMin = 0))
	float RotationStopLead = 0.f;

	/** 회전 정지 시간을 공격 몽타주의 시간으로 바꾼 값 */
	virtual float GetRotationStopLeadInMontage() const { return RotationStopLead; }

	/** Abilities granted at the start: actions, reactions, death. */
	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TArray<TSubclassOf<UGameplayAbility>> Abilities;

	/** 시작할 때 거는 리젠 효과(SP·PP 리젠, 달리기의 SP 소모). 회복량은 각 효과가 이 데이터에서 읽습니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TArray<TSubclassOf<UAgEffect_Regen>> RegenEffects;

	/** 움찔: additive montage played on top of whatever the character is doing. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> FlinchMontage;

	/** 가드 움찔: 가드 자세 위에 덧입혀 재생하는 몽타주. 가드하지 않는 캐릭터는 비워 둡니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> GuardFlinchMontage;

	/** 넉백: 밀려나 쓰러졌다가 일어남 */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> KnockBackMontage;

	/** Locomotion while not guarding. Axes: movement direction relative to facing (deg), ground speed (cm/s). */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Animation")
	TObjectPtr<UBlendSpace> MoveBlendSpace;

	/** Locomotion while guarding, same axes as MoveBlendSpace. Leave empty for a character that never guards. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Animation")
	TObjectPtr<UBlendSpace> GuardMoveBlendSpace;

	/** Loop played while falling. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Animation")
	TObjectPtr<UAnimSequenceBase> AirborneAnimation;
};

/** DA_Player, Player.json */
UCLASS(BlueprintType)
class UAgPlayerData : public UAgCharacterData
{
	GENERATED_BODY()

public:
	/** 선입력: how long a key pressed while its action isn't allowed is remembered. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Input", meta = (Units = "s", ClampMin = 0))
	float InputBufferTime = 0.f;

	/** 평타: one action per hit, from the first hit. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Basic Attack")
	TArray<FAgPlayerAttack> BasicAttack;

	/** 점프 공격 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Jump Attack")
	FAgPlayerAttack JumpAttack;

	/** 점프 공격 '내리꽂기': 내려베기가 끝난 뒤 떨어지는 속도 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Jump Attack", meta = (Units = "cm/s", ClampMin = 0))
	float JumpAttackPlungeSpeed = 0.f;

	/** 점프 공격 '착지': 착지부터 후딜이 시작될 때까지의 시간 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Jump Attack", meta = (Units = "s", ClampMin = 0))
	float JumpAttackRecoveryStart = 0.f;

	/** 자원 회복: SP regained per second after the regen delay, while not guarding. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Resources", meta = (ClampMin = 0))
	float SPRegenRate = 0.f;

	/** 자원 회복: SP regained per second while guarding. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Resources", meta = (ClampMin = 0))
	float SPRegenRateGuarding = 0.f;

	/** 자원 회복: regen delay after the last SP use. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Resources", meta = (Units = "s", ClampMin = 0))
	float SPRegenDelay = 0.f;

	/** 자원 회복: regen delay when SP reaches 0. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Resources", meta = (Units = "s", ClampMin = 0))
	float SPRegenDelayEmpty = 0.f;

	/** MP 충전: 극한 회피 성공 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Resources", meta = (ClampMin = 0))
	float PerfectDodgeMPCharge = 0.f;

	/** UP 충전: 극한 회피 성공 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Resources", meta = (ClampMin = 0))
	float PerfectDodgeUPCharge = 0.f;

	/** MP 충전: 패리 성공 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Resources", meta = (ClampMin = 0))
	float ParryMPCharge = 0.f;

	/** UP 충전: 패리 성공 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Resources", meta = (ClampMin = 0))
	float ParryUPCharge = 0.f;

	/** SP 소모: 회피. Usable while SP is above 0 even if lower. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Dodge", meta = (ClampMin = 0))
	float DodgeSPCost = 0.f;

	/** 회피: 쿨다운 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Dodge", meta = (Units = "s", ClampMin = 0))
	float DodgeCooldown = 0.f;

	/** 회피: 무적 구간, from the start. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Dodge", meta = (Units = "s", ClampMin = 0))
	float DodgeInvincibleTime = 0.f;

	/** 회피: 극한 회피 구간, from the start. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Dodge", meta = (Units = "s", ClampMin = 0))
	float PerfectDodgeTime = 0.f;

	/** 회피: 후딜 시작 when rolling. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Dodge", meta = (Units = "s", ClampMin = 0))
	float RollRecoveryStart = 0.f;

	/** 회피: 후딜 시작 when stepping back. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Dodge", meta = (Units = "s", ClampMin = 0))
	float BackstepRecoveryStart = 0.f;

	/** SP 소모: a guarded attack costs this share of its HP damage before the guard reduction. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Guard", meta = (ClampMin = 0))
	float GuardSPCostRatio = 0.f;

	/** 이동: 가드 중 이동 속도, as a multiple of MOV. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Guard", meta = (ClampMin = 0))
	float GuardMoveSpeedRatio = 1.f;

	/** 가드 반응: 가드 움찔로 공격 반대쪽으로 밀려나는 거리 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Guard", meta = (Units = "cm", ClampMin = 0))
	float GuardFlinchPushDistance = 0.f;

	/** 패리: 패리 구간, from a new guard start. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Guard", meta = (Units = "s", ClampMin = 0))
	float ParryWindowTime = 0.f;

	/** 상태별 허용 입력: 패리 구간이 생기는 가드 시작 후 이동할 수 없는 시간 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Guard", meta = (Units = "s", ClampMin = 0))
	float GuardMoveLockTime = 0.f;

	/** 패리: the attacker loses this share of its MaxPP. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Guard", meta = (ClampMin = 0, ClampMax = 1))
	float ParryPPRatio = 0.f;

	/** 패리: 공격 반대쪽으로 밀려나는 거리 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Guard", meta = (Units = "cm", ClampMin = 0))
	float ParryPushDistance = 0.f;

	/** 패리와 가드 움찔로 밀려나는 데 걸리는 시간. 기획서에 수치가 없는 구현 값입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Guard", meta = (Units = "s", ClampMin = 0))
	float BlockPushTime = 0.f;

	/** 스킬 (돌진 베기) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Skill")
	FAgPlayerAttack Skill;

	/** 스킬: MP 소모 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Skill", meta = (ClampMin = 0))
	float SkillMPCost = 0.f;

	/** 스킬: 쿨다운 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Skill", meta = (Units = "s", ClampMin = 0))
	float SkillCooldown = 0.f;

	/** 궁극기 (심판): 마무리의 1타 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Ultimate")
	FAgPlayerAttackHit UltimateHit;

	/** 궁극기: 범위 (플레이어와 적의 캡슐 중심 사이 수평 거리) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Ultimate", meta = (Units = "cm", ClampMin = 0))
	float UltimateRadius = 0.f;

	/** 베기 선: 개수 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Ultimate", meta = (ClampMin = 0))
	int32 UltimateSlashCount = 0;

	/** 베기 선: 위치. 카메라가 보는 쪽으로 플레이어 앞 이 거리 안 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Ultimate")
	FFloatInterval UltimateSlashDistance = FFloatInterval(0.f, 0.f);

	/** 베기 선: 위치. 바닥에서 이 높이 안 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Ultimate")
	FFloatInterval UltimateSlashHeight = FFloatInterval(0.f, 0.f);

	/** 슬로우모션: 궁극기 마무리 적중의 속도 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Ultimate", meta = (ClampMin = 0.01, ClampMax = 1))
	float UltimateSlowMotionSpeed = 1.f;

	/** 슬로우모션: 궁극기 마무리 적중의 지속 시간 (실제 시간) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Ultimate", meta = (Units = "s", ClampMin = 0))
	float UltimateSlowMotionDuration = 0.f;

	/** 궁극기 컷신(LS_UltimateCutscene). 카메라만 담고, 플레이어 위치·방향을 기준으로 재생합니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Ultimate")
	TObjectPtr<ULevelSequence> UltimateCutscene;

	/** 처형. The targeting range is the 처형 타겟팅 범위. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Execution")
	FAgPlayerAttack Execution;

	/**
	 * Gap (capsule centers) the player closes to in front of the boss by the execution's first slash, where the two
	 * execution motions line up. The spec gives no number; measured from the two motions' authored placement.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Execution", meta = (Units = "cm", ClampMin = 0))
	float ExecutionDistance = 0.f;

	/** 락온 타겟팅 범위 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Lock-On", meta = (Units = "cm", ClampMin = 0))
	float LockOnRange = 0.f;

	/** 락온 해제 조건: the lock-on ends when the target is farther than this. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Lock-On", meta = (Units = "cm", ClampMin = 0))
	float LockOnReleaseRange = 0.f;

	/** TP_Enemy: 공격 중 이동, 락온, 처형의 대상 후보(플레이어 주변의 캐릭터)를 모읍니다. 범위와 조건은 쓰는 곳마다 따로 확인합니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	TObjectPtr<UTargetingPreset> EnemyTargeting;

	/** 이동 '달리기': speed as a multiple of MOV. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Sprint", meta = (ClampMin = 0))
	float SprintSpeedRatio = 1.f;

	/** SP 소모: 달리기, per second. Usable while SP is above 0. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Sprint", meta = (ClampMin = 0))
	float SprintSPCost = 0.f;

	/** 회피 반격 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Dodge Counter")
	FAgPlayerAttack DodgeCounter;

	/** 회피 반격 기회: 유효 시간, from the 극한 회피. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Dodge Counter", meta = (Units = "s", ClampMin = 0))
	float DodgeCounterChanceTime = 0.f;

	/** 슬로우모션: 속도, the scale of game time. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Slow Motion", meta = (ClampMin = 0.01, ClampMax = 1))
	float SlowMotionSpeed = 1.f;

	/** 슬로우모션: 지속 시간, in real time. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Slow Motion", meta = (Units = "s", ClampMin = 0))
	float SlowMotionDuration = 0.f;

	/** 평타 montages, in the same order as BasicAttack. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TArray<TObjectPtr<UAnimMontage>> BasicAttackMontages;

	/** 회피: rolling (toward the movement input). */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> RollMontage;

	/** 회피: stepping back (no movement input). */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> BackstepMontage;

	/** 가드 밀림 */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> GuardPushbackMontage;

	/** 가드 브레이크 */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> GuardBreakMontage;

	/** 패리 모션 */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> ParryMontage;

	/** 스킬 */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> SkillMontage;

	/** 회피 반격 */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> DodgeCounterMontage;

	/** 점프: 시작 모션. 점프할 때마다 재생합니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> JumpMontage;

	/** 점프: the landing, played when landing without movement input. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> LandMontage;

	/**
	 * 점프 공격: 섹션 Start(내려베기) → Fall(떨어지는 자세, 반복) → Land(착지해 바닥을 찍음). 착지하면 Land로 넘깁니다.
	 * 판정 구간 하나가 내려베기부터 착지해 바닥을 찍을 때까지 이어지고, 내려베기가 끝날 때의 노티파이(Event.JumpAttackPlunge)가
	 * 내리꽂기를 시작합니다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> JumpAttackMontage;

	/**
	 * 궁극기: 섹션 Start(발도 자세) → Hold(기다림, 반복) → Judgment(칼 넣기) → Draw(칼 뽑기). 컷신이 끝나면 Judgment로 넘기고,
	 * Judgment가 끝나는 순간의 노티파이(Event.UltimateFinish)가 마무리입니다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> UltimateMontage;

	/** 처형 */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> ExecutionMontage;

	/** 넉업: 기상. KnockUpFallMontage 뒤에 블렌드해 잇습니다(자세가 조금 다름). */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> GetUpMontage;

	/** 넉업: the rising pose, repeated until KnockUpFallMontage blends in around the top of the flight. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> KnockUpMontage;

	/**
	 * 넉업: section Fall (the falling pose, repeated) and section Land (hitting the ground), played on landing.
	 * Its blend-in time is centered on the top of the flight.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> KnockUpFallMontage;
};

/** DA_Boss, Boss.json */
UCLASS(BlueprintType)
class UAgBossData : public UAgCharacterData
{
	GENERATED_BODY()

public:
	const FAgBossPattern* FindPattern(FName Id) const;
	UAnimMontage* FindPatternMontage(FName Id) const;
	TSubclassOf<UAgGameplayAbility> FindPatternAbility(FName Id) const;

	/** 회전 정지 시간(1페이즈 재생 속도의 초)을 패턴 몽타주의 시간으로 바꾼 값. 2페이즈에서는 몽타주와 함께 빨라지므로 그대로입니다. */
	virtual float GetRotationStopLeadInMontage() const override { return RotationStopLead * PatternPlayRate; }

	/** 패턴 목록 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Patterns")
	TArray<FAgBossPattern> Patterns;

	/** 모션 재생 속도: pattern motions play at this rate in phase 1. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Pattern Rules", meta = (ClampMin = 0.01))
	float PatternPlayRate = 1.f;

	/** 패턴 선택: 근거리의 상한 (캡슐 중심 사이 수평 거리) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "AI", meta = (Units = "cm", ClampMin = 0))
	float NearDistance = 0.f;

	/** 패턴 선택: 중거리의 상한. 이보다 멀면 원거리입니다. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "AI", meta = (Units = "cm", ClampMin = 0))
	float FarDistance = 0.f;

	/** 패턴 선택: with no candidate, the boss walks to the player and retries this often. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "AI", meta = (Units = "s", ClampMin = 0.01))
	float RetryInterval = 0.5f;

	/** 패턴 후딜레이 대기 시간 after a pattern in phase 1 (random in range). */
	UPROPERTY(Transient, VisibleAnywhere, Category = "AI")
	FFloatInterval PatternRecoveryWaitPhase1 = FFloatInterval(0.f, 0.f);

	/** 패턴 후딜레이 대기 시간 after a pattern in phase 2 (random in range). */
	UPROPERTY(Transient, VisibleAnywhere, Category = "AI")
	FFloatInterval PatternRecoveryWaitPhase2 = FFloatInterval(0.f, 0.f);

	/** 패턴 후딜레이 대기 시간 after the level start, a knockback, the phase transition, groggy and being executed. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "AI", meta = (Units = "s", ClampMin = 0))
	float FixedRecoveryWait = 0.f;

	/** 페이즈: phase 2 starts once HP is at or below this share of MaxHP. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Phase", meta = (ClampMin = 0, ClampMax = 1))
	float Phase2HPRatio = 0.f;

	/** 2페이즈 변화: 패턴 진행 속도 (attack motions and the movement inside patterns). */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Phase", meta = (ClampMin = 0.01))
	float Phase2PatternSpeed = 1.f;

	/** 2페이즈 변화: MOV multiplier (walking). */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Phase", meta = (ClampMin = 0))
	float Phase2MOVMultiplier = 1.f;

	/** 돌진 찌르기 돌진 규칙: 속도 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Dash Thrust", meta = (Units = "cm/s", ClampMin = 0))
	float DashSpeed = 0.f;

	/** 돌진 찌르기 돌진 규칙: 목표 지점, this far past the player. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Dash Thrust", meta = (Units = "cm", ClampMin = 0))
	float DashOvershoot = 0.f;

	/** 돌진 찌르기 돌진 규칙: 최대 이동 거리 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Dash Thrust", meta = (Units = "cm", ClampMin = 0))
	float DashMaxDistance = 0.f;

	/** 도약 내려찍기 체공 시간: 도약부터 착지까지. 엔진 중력과 함께 최고 높이를 정합니다. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Leap Slam", meta = (Units = "s", ClampMin = 0.01))
	float LeapAirTime = 1.f;

	/** 도약 내려찍기: 착지 목표, this far short of the player toward the boss. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Leap Slam", meta = (Units = "cm", ClampMin = 0))
	float LeapTargetOffset = 0.f;

	/** 도약 내려찍기: 최대 도약 거리 (horizontal). */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Leap Slam", meta = (Units = "cm", ClampMin = 0))
	float LeapMaxDistance = 0.f;

	/** 도약 내려찍기: the landing hits within this radius of the landing point. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Leap Slam", meta = (Units = "cm", ClampMin = 0))
	float LeapHitRadius = 0.f;

	/** 백스텝: the player must be this close (capsule centers, horizontal). */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Backstep", meta = (Units = "cm", ClampMin = 0))
	float BackstepRange = 0.f;

	/** 백스텝: chance, checked every time the player starts a basic attack hit. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Backstep", meta = (ClampMin = 0, ClampMax = 1))
	float BackstepChance = 0.f;

	/** 백스텝: 뒤로 물러나는 거리. 모션의 루트 모션을 이 거리에 맞춰 늘립니다. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Backstep", meta = (Units = "cm", ClampMin = 0))
	float BackstepDistance = 0.f;

	/** 검기 발사: the projectile (BP_SwordWave). Its hit box (폭, 높이) and look are set on the blueprint. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Sword Wave")
	TSubclassOf<AAgSwordWave> SwordWaveClass;

	/** 검기 발사: 속도 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Sword Wave", meta = (Units = "cm/s", ClampMin = 0))
	float SwordWaveSpeed = 0.f;

	/** 검기 발사: 최대 사거리 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Sword Wave", meta = (Units = "cm", ClampMin = 0))
	float SwordWaveRange = 0.f;

	/** 락온 카메라: bone of the 락온 지점 (chest height) the camera looks at. The lock-on marker shows there too. */
	UPROPERTY(EditDefaultsOnly, Category = "Lock-On")
	FName LockOnBone;

	/** 이름, shown in 전투 HUD '보스 상태'. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "UI")
	FText DisplayName;

	/** 전투 HUD '처형 안내': the prompt shows above this bone (보스 머리 위). */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	FName ExecutionPromptBone;

	/** 패턴 ID마다 그 패턴을 실행하는 어빌리티. 2연격, 휩쓸기, 3연격, 뛰어 내려찍기는 공용 패턴 어빌리티(UAgBossPattern_Common)를 씁니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Patterns")
	TMap<FName, TSubclassOf<UAgGameplayAbility>> PatternAbilities;

	/** 보스 AI의 흐름(ST_Boss). 보스 AI 컨트롤러가 빙의할 때 실행합니다. */
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TObjectPtr<UStateTree> AIStateTree;

	/** 패턴 ID마다 그 패턴의 몽타주 */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TMap<FName, TObjectPtr<UAnimMontage>> PatternMontages;

	/** 그로기: sections Start, Loop (repeats) and End; End is jumped to so the motion ends with the groggy time. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> GroggyMontage;

	/**
	 * 처형 피격: the motion paired with the player's execution; the boss stands up at its end. It waits on its first frame
	 * until the execution's first slash, so it starts in the same frame as the execution montage and stays in step.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> ExecutedMontage;

	/** 처형 사망: same as ExecutedMontage until after the final blow, then stays down. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> ExecutedDeathMontage;

	/** 페이즈 전환: the roar. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> PhaseTransitionMontage;
};
