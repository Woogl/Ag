// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/AgAttackData.h"
#include "Engine/DataAsset.h"
#include "Math/Interval.h"
#include "AgCharacterData.generated.h"

class AAgSwordWave;
class UAnimMontage;
class UAnimSequenceBase;
class UBlendSpace;
class UGameplayAbility;
class UStreamableRenderAsset;

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
 * Data shared by the player and the boss (DA_Player, DA_Boss).
 * Numbers come from the specs; the 데이터 위치표 in the implementation plan maps each field to its spec item.
 * Numbers and asset references live in separate categories so tuning numbers never touches references.
 */
UCLASS(Abstract, BlueprintType)
class UAgCharacterData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (ShowOnlyInnerProperties))
	FAgStats Stats;

	/** 슈퍼아머 상시 적용 (전투 시스템 '슈퍼아머'): knockback and down never happen. */
	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	bool bAlwaysSuperArmor = false;

	/** PP 리젠: 초당 회복량 after the regen delay (전투 시스템 'PP 리젠'). 0 for no regen. */
	UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (ClampMin = 0))
	float PPRegenRate = 0.f;

	/** 사망 연출 시간. 끝나면 결과 UI를 띄웁니다. (플레이어 사양·보스 사양 '사망') */
	UPROPERTY(EditDefaultsOnly, Category = "Death", meta = (Units = "s", ClampMin = 0))
	float DeathPresentationTime = 0.f;

	/** Start of the weapon's damaging edge, in the weapon mesh's space. Attacks sweep the line from start to end. */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (Units = "cm"))
	FVector WeaponEdgeStart = FVector::ZeroVector;

	/** End (tip) of the weapon's damaging edge, in the weapon mesh's space. */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (Units = "cm"))
	FVector WeaponEdgeEnd = FVector::ZeroVector;

	/** Thickness of the swept edge (sphere radius). The spec gives no number; an implementation value for QA to tune. */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (Units = "cm", ClampMin = 0))
	float WeaponEdgeRadius = 0.f;

	/** Abilities granted at the start: actions, reactions, death. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Abilities")
	TArray<TSubclassOf<UGameplayAbility>> Abilities;

	/** 움찔: additive montage played on top of whatever the character is doing. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> FlinchMontage;

	/** Locomotion while not guarding. Axes: movement direction relative to facing (deg), ground speed (cm/s). */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Animation")
	TObjectPtr<UBlendSpace> MoveBlendSpace;

	/** Locomotion while guarding, same axes as MoveBlendSpace. Leave empty for a character that never guards. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Animation")
	TObjectPtr<UBlendSpace> GuardMoveBlendSpace;

	/** Loop played while falling. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Animation")
	TObjectPtr<UAnimSequenceBase> AirborneAnimation;

	/** Weapon mesh: the katana is a static mesh, the halberd a skeletal mesh. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Weapon", meta = (AllowedClasses = "/Script/Engine.StaticMesh,/Script/Engine.SkeletalMesh"))
	TObjectPtr<UStreamableRenderAsset> WeaponMesh;

	/** Socket on SK_Mannequin the weapon attaches to. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Weapon")
	FName WeaponSocket;
};

/** DA_Player */
UCLASS(BlueprintType)
class UAgPlayerData : public UAgCharacterData
{
	GENERATED_BODY()

public:
	/** 평타: one action per hit, from the first hit. */
	UPROPERTY(EditDefaultsOnly, Category = "Basic Attack")
	TArray<FAgPlayerAttack> BasicAttack;

	/** 공격 중 이동: the gap the player keeps from the target (capsule centers, horizontal). */
	UPROPERTY(EditDefaultsOnly, Category = "Attack Movement", meta = (Units = "cm", ClampMin = 0))
	float AttackStopDistance = 0.f;

	/** 자원 회복: SP regained per second after the regen delay, while not guarding. */
	UPROPERTY(EditDefaultsOnly, Category = "Resources", meta = (ClampMin = 0))
	float SPRegenRate = 0.f;

	/** 자원 회복: SP regained per second while guarding. */
	UPROPERTY(EditDefaultsOnly, Category = "Resources", meta = (ClampMin = 0))
	float SPRegenRateGuarding = 0.f;

	/** 자원 회복: regen delay after the last SP use. */
	UPROPERTY(EditDefaultsOnly, Category = "Resources", meta = (Units = "s", ClampMin = 0))
	float SPRegenDelay = 0.f;

	/** 자원 회복: regen delay when SP reaches 0. */
	UPROPERTY(EditDefaultsOnly, Category = "Resources", meta = (Units = "s", ClampMin = 0))
	float SPRegenDelayEmpty = 0.f;

	/** MP 충전: 극한 회피 성공 */
	UPROPERTY(EditDefaultsOnly, Category = "Resources", meta = (ClampMin = 0))
	float PerfectDodgeMPCharge = 0.f;

	/** UP 충전: 극한 회피 성공 */
	UPROPERTY(EditDefaultsOnly, Category = "Resources", meta = (ClampMin = 0))
	float PerfectDodgeUPCharge = 0.f;

	/** MP 충전: 패리 성공 */
	UPROPERTY(EditDefaultsOnly, Category = "Resources", meta = (ClampMin = 0))
	float ParryMPCharge = 0.f;

	/** UP 충전: 패리 성공 */
	UPROPERTY(EditDefaultsOnly, Category = "Resources", meta = (ClampMin = 0))
	float ParryUPCharge = 0.f;

	/** SP 소모: 회피. Usable while SP is above 0 even if lower. */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge", meta = (ClampMin = 0))
	float DodgeSPCost = 0.f;

	/** 회피: 쿨다운 */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge", meta = (Units = "s", ClampMin = 0))
	float DodgeCooldown = 0.f;

	/** 회피: 무적 구간, from the start. */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge", meta = (Units = "s", ClampMin = 0))
	float DodgeInvincibleTime = 0.f;

	/** 회피: 극한 회피 구간, from the start. */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge", meta = (Units = "s", ClampMin = 0))
	float PerfectDodgeTime = 0.f;

	/** 회피: 후딜 시작 when rolling. */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge", meta = (Units = "s", ClampMin = 0))
	float RollRecoveryStart = 0.f;

	/** 회피: 후딜 시작 when stepping back. */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge", meta = (Units = "s", ClampMin = 0))
	float BackstepRecoveryStart = 0.f;

	/** SP 소모: a guarded attack costs this share of its HP damage before the guard reduction. */
	UPROPERTY(EditDefaultsOnly, Category = "Guard", meta = (ClampMin = 0))
	float GuardSPCostRatio = 0.f;

	/** 패리: 패리 구간, from a new guard start. */
	UPROPERTY(EditDefaultsOnly, Category = "Guard", meta = (Units = "s", ClampMin = 0))
	float ParryWindowTime = 0.f;

	/** 패리: the attacker loses this share of its MaxPP. */
	UPROPERTY(EditDefaultsOnly, Category = "Guard", meta = (ClampMin = 0, ClampMax = 1))
	float ParryPPRatio = 0.f;

	/** 스킬 (돌진 베기) */
	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	FAgPlayerAttack Skill;

	/** 스킬: MP 소모 */
	UPROPERTY(EditDefaultsOnly, Category = "Skill", meta = (ClampMin = 0))
	float SkillMPCost = 0.f;

	/** 스킬: 쿨다운 */
	UPROPERTY(EditDefaultsOnly, Category = "Skill", meta = (Units = "s", ClampMin = 0))
	float SkillCooldown = 0.f;

	/** 궁극기 (난무) */
	UPROPERTY(EditDefaultsOnly, Category = "Ultimate")
	FAgPlayerAttack Ultimate;

	/** 궁극기: UP 소모. Usable only when UP is at MaxUP. */
	UPROPERTY(EditDefaultsOnly, Category = "Ultimate", meta = (ClampMin = 0))
	float UltimateUPCost = 0.f;

	/** 처형. The targeting range is the 처형 타겟팅 범위. */
	UPROPERTY(EditDefaultsOnly, Category = "Execution")
	FAgPlayerAttack Execution;

	/**
	 * Gap (capsule centers) the player closes to in front of the boss so the two execution motions line up.
	 * The spec gives no number; set by eye in the editor.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Execution", meta = (Units = "cm", ClampMin = 0))
	float ExecutionDistance = 0.f;

	/** 락온 타겟팅 범위 */
	UPROPERTY(EditDefaultsOnly, Category = "Lock-On", meta = (Units = "cm", ClampMin = 0))
	float LockOnRange = 0.f;

	/** 락온 해제 조건: the lock-on ends when the target is farther than this. */
	UPROPERTY(EditDefaultsOnly, Category = "Lock-On", meta = (Units = "cm", ClampMin = 0))
	float LockOnReleaseRange = 0.f;

	/** 이동 '달리기': speed as a multiple of MOV. */
	UPROPERTY(EditDefaultsOnly, Category = "Sprint", meta = (ClampMin = 0))
	float SprintSpeedRatio = 1.f;

	/** SP 소모: 달리기, per second. Usable while SP is above 0. */
	UPROPERTY(EditDefaultsOnly, Category = "Sprint", meta = (ClampMin = 0))
	float SprintSPCost = 0.f;

	/** 회피 반격 */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge Counter")
	FAgPlayerAttack DodgeCounter;

	/** 회피 반격 기회: 유효 시간, from the 극한 회피. */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge Counter", meta = (Units = "s", ClampMin = 0))
	float DodgeCounterChanceTime = 0.f;

	/** 슬로우모션: 속도, the scale of game time. */
	UPROPERTY(EditDefaultsOnly, Category = "Slow Motion", meta = (ClampMin = 0.01, ClampMax = 1))
	float SlowMotionSpeed = 1.f;

	/** 슬로우모션: 지속 시간, in real time. */
	UPROPERTY(EditDefaultsOnly, Category = "Slow Motion", meta = (Units = "s", ClampMin = 0))
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

	/** 가드 움찔: additive montage played on the guard pose. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> GuardFlinchMontage;

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

	/** 점프: the take-off, played when jumping without movement input. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> JumpMontage;

	/** 점프: the landing, played when landing without movement input. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> LandMontage;

	/** 궁극기: montages played one after another, each blended into the next. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TArray<TObjectPtr<UAnimMontage>> UltimateMontages;

	/** 처형 */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> ExecutionMontage;

	/** 넉백 */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> KnockBackMontage;

	/** 다운: falling down. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> DownMontage;

	/** 다운: getting up, blended in after DownMontage (the poses differ slightly). */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> GetUpMontage;
};

/** DA_Boss */
UCLASS(BlueprintType)
class UAgBossData : public UAgCharacterData
{
	GENERATED_BODY()

public:
	const FAgBossPattern* FindPattern(const FGameplayTag& Pattern) const;
	UAnimMontage* FindPatternMontage(const FGameplayTag& Pattern) const;

	/** 패턴 목록 */
	UPROPERTY(EditDefaultsOnly, Category = "Patterns")
	TArray<FAgBossPattern> Patterns;

	/** 모션 재생 속도: pattern motions play at this rate in phase 1. */
	UPROPERTY(EditDefaultsOnly, Category = "Pattern Rules", meta = (ClampMin = 0.01))
	float PatternPlayRate = 1.f;

	/** 이동: 정지 간격 (capsule centers, horizontal). */
	UPROPERTY(EditDefaultsOnly, Category = "Pattern Rules", meta = (Units = "cm", ClampMin = 0))
	float StopDistance = 0.f;

	/** 이동: 최대 접근 거리 of the first-hit approach (A1, A5). */
	UPROPERTY(EditDefaultsOnly, Category = "Pattern Rules", meta = (Units = "cm", ClampMin = 0))
	float FirstHitApproachDistance = 0.f;

	/** 회전: turning speed toward the player. */
	UPROPERTY(EditDefaultsOnly, Category = "Pattern Rules", meta = (Units = "deg/s", ClampMin = 0))
	float RotationSpeed = 0.f;

	/** 회전: the boss stops turning this long before each attack window starts, until it ends. */
	UPROPERTY(EditDefaultsOnly, Category = "Pattern Rules", meta = (Units = "s", ClampMin = 0))
	float RotationStopLead = 0.f;

	/** 거리 구간: 근거리 upper bound (capsule centers, horizontal). */
	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (Units = "cm", ClampMin = 0))
	float NearDistance = 0.f;

	/** 거리 구간: 중거리 upper bound. Farther is 원거리. */
	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (Units = "cm", ClampMin = 0))
	float FarDistance = 0.f;

	/** 패턴 선택: with no candidate, the boss walks to the player and retries this often. */
	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (Units = "s", ClampMin = 0.01))
	float RetryInterval = 0.5f;

	/** 패턴 후딜레이 대기 시간 after a pattern in phase 1 (random in range). */
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	FFloatInterval PatternRecoveryWaitPhase1 = FFloatInterval(0.f, 0.f);

	/** 패턴 후딜레이 대기 시간 after a pattern in phase 2 (random in range). */
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	FFloatInterval PatternRecoveryWaitPhase2 = FFloatInterval(0.f, 0.f);

	/** 패턴 후딜레이 대기 시간 after the level start, the phase transition, groggy and being executed. */
	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (Units = "s", ClampMin = 0))
	float FixedRecoveryWait = 0.f;

	/** 페이즈: phase 2 starts once HP is at or below this share of MaxHP. */
	UPROPERTY(EditDefaultsOnly, Category = "Phase", meta = (ClampMin = 0, ClampMax = 1))
	float Phase2HPRatio = 0.f;

	/** 2페이즈 변화: 패턴 진행 속도 (attack motions and the movement inside patterns). */
	UPROPERTY(EditDefaultsOnly, Category = "Phase", meta = (ClampMin = 0.01))
	float Phase2PatternSpeed = 1.f;

	/** 2페이즈 변화: MOV multiplier (walking). */
	UPROPERTY(EditDefaultsOnly, Category = "Phase", meta = (ClampMin = 0))
	float Phase2MOVMultiplier = 1.f;

	/** 공격 속성과 예고: the 붉은 섬광 starts this long before an unguardable attack window. */
	UPROPERTY(EditDefaultsOnly, Category = "Pattern Rules", meta = (Units = "s", ClampMin = 0))
	float UnblockableFlashLead = 0.f;

	/** A3 돌진 규칙: 속도 */
	UPROPERTY(EditDefaultsOnly, Category = "A3 Dash", meta = (Units = "cm/s", ClampMin = 0))
	float DashSpeed = 0.f;

	/** A3 돌진 규칙: 목표 지점, this far past the player. */
	UPROPERTY(EditDefaultsOnly, Category = "A3 Dash", meta = (Units = "cm", ClampMin = 0))
	float DashOvershoot = 0.f;

	/** A3 돌진 규칙: 최대 이동 거리 */
	UPROPERTY(EditDefaultsOnly, Category = "A3 Dash", meta = (Units = "cm", ClampMin = 0))
	float DashMaxDistance = 0.f;

	/** A4: 체공 시간 from the leap to the landing. */
	UPROPERTY(EditDefaultsOnly, Category = "A4 Leap", meta = (Units = "s", ClampMin = 0.01))
	float LeapAirTime = 1.f;

	/** A4: 착지 목표, this far short of the player toward the boss. */
	UPROPERTY(EditDefaultsOnly, Category = "A4 Leap", meta = (Units = "cm", ClampMin = 0))
	float LeapTargetOffset = 0.f;

	/** A4: 최대 도약 거리 (horizontal). */
	UPROPERTY(EditDefaultsOnly, Category = "A4 Leap", meta = (Units = "cm", ClampMin = 0))
	float LeapMaxDistance = 0.f;

	/** A4: the landing hits within this radius of the landing point. */
	UPROPERTY(EditDefaultsOnly, Category = "A4 Leap", meta = (Units = "cm", ClampMin = 0))
	float LeapHitRadius = 0.f;

	/** B1 백스텝: the player must be this close (capsule centers, horizontal). */
	UPROPERTY(EditDefaultsOnly, Category = "B1 Backstep", meta = (Units = "cm", ClampMin = 0))
	float BackstepRange = 0.f;

	/** B1 백스텝: chance, checked every time the player starts a basic attack hit. */
	UPROPERTY(EditDefaultsOnly, Category = "B1 Backstep", meta = (ClampMin = 0, ClampMax = 1))
	float BackstepChance = 0.f;

	/** A6 검기: the projectile (BP_SwordWave). */
	UPROPERTY(EditDefaultsOnly, Category = "A6 Sword Wave")
	TSubclassOf<AAgSwordWave> SwordWaveClass;

	/** A6 검기: 속도 */
	UPROPERTY(EditDefaultsOnly, Category = "A6 Sword Wave", meta = (Units = "cm/s", ClampMin = 0))
	float SwordWaveSpeed = 0.f;

	/** A6 검기: 최대 사거리 */
	UPROPERTY(EditDefaultsOnly, Category = "A6 Sword Wave", meta = (Units = "cm", ClampMin = 0))
	float SwordWaveRange = 0.f;

	/** A6 검기: 폭 */
	UPROPERTY(EditDefaultsOnly, Category = "A6 Sword Wave", meta = (Units = "cm", ClampMin = 0))
	float SwordWaveWidth = 0.f;

	/** A6 검기: 높이, from the ground. */
	UPROPERTY(EditDefaultsOnly, Category = "A6 Sword Wave", meta = (Units = "cm", ClampMin = 0))
	float SwordWaveHeight = 0.f;

	/** A6 검기: thickness along its flight. The spec gives no number; an implementation value. */
	UPROPERTY(EditDefaultsOnly, Category = "A6 Sword Wave", meta = (Units = "cm", ClampMin = 0))
	float SwordWaveDepth = 0.f;

	/** 락온 카메라: bone of the 락온 지점 (chest height) the camera looks at. The lock-on marker shows there too. */
	UPROPERTY(EditDefaultsOnly, Category = "Lock-On")
	FName LockOnBone;

	/** 이름, shown in 전투 HUD '보스 상태'. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	FText DisplayName;

	/** 전투 HUD '처형 안내': the prompt shows above this bone (보스 머리 위). */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	FName ExecutionPromptBone;

	/** Pattern montages, by pattern ID. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages", meta = (Categories = "Ability.Boss.Pattern"))
	TMap<FGameplayTag, TObjectPtr<UAnimMontage>> PatternMontages;

	/** 그로기: sections Start, Loop (repeats) and End; End is jumped to so the motion ends with the groggy time. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> GroggyMontage;

	/** 처형 피격: the motion paired with the player's execution; the boss stands up at its end. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> ExecutedMontage;

	/** 처형 사망: same as ExecutedMontage until after the final blow, then stays down. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> ExecutedDeathMontage;

	/** 페이즈 전환: the roar. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> PhaseTransitionMontage;
};
