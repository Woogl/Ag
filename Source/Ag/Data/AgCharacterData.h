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

/** 플레이어 사양·보스 사양의 '스탯' 표: 스탯마다 초기값과 최대값 */
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

	/** bInfinitePP면 쓰지 않습니다. */
	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = 0, EditCondition = "!bInfinitePP"))
	float PP = 0.f;

	/** bInfinitePP면 쓰지 않습니다. */
	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = 0, EditCondition = "!bInfinitePP"))
	float MaxPP = 0.f;

	/** PP 무한: PP가 줄지 않아 그로기에 빠지지 않습니다. */
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
 * 플레이어와 보스가 함께 쓰는 데이터(구현 계획서 'JSON과 DataAsset').
 * 데이터 에셋(DA_Player, DA_Boss)에는 에셋 참조와 기획서에 없는 값(구현 값, 어빌리티, 본 이름)을 저장합니다.
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

	/** Transient 필드를 채우는 프로젝트 Data 폴더의 JSON 파일 (Player.json, Boss.json) */
	UPROPERTY(EditDefaultsOnly, Category = "Data")
	FString ValuesFile;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Stats", meta = (ShowOnlyInnerProperties))
	FAgStats Stats;

	/** PP 리젠: 리젠 대기 시간이 지난 뒤의 초당 회복량 (전투 시스템 'PP 리젠'). 0이면 회복하지 않습니다. */
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

	/** 시작할 때 주는 어빌리티 (행동, 피격 반응, 사망 등) */
	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TArray<TSubclassOf<UGameplayAbility>> Abilities;

	/** 시작할 때 거는 리젠 효과(SP·PP 리젠, 달리기의 SP 소모). 회복량은 각 효과가 이 데이터에서 읽습니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TArray<TSubclassOf<UAgEffect_Regen>> RegenEffects;

	/** 움찔: 하던 동작 위에 덧입혀 재생하는 몽타주 */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> FlinchMontage;

	/** 가드 움찔: 가드 자세 위에 덧입혀 재생하는 몽타주. 가드하지 않는 캐릭터는 비워 둡니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> GuardFlinchMontage;

	/** 넉백: 밀려나 쓰러졌다가 일어남 */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> KnockBackMontage;

	/** 가드 중이 아닐 때의 이동. 가로축은 캐릭터 정면 기준 이동 방향 (deg), 세로축은 수평 속도 (cm/s)입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Animation")
	TObjectPtr<UBlendSpace> MoveBlendSpace;

	/** 가드 중의 이동. 축은 MoveBlendSpace와 같습니다. 가드하지 않는 캐릭터는 비워 둡니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Animation")
	TObjectPtr<UBlendSpace> GuardMoveBlendSpace;

	/** 공중에 있는 동안 반복 재생하는 모션 */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Animation")
	TObjectPtr<UAnimSequenceBase> AirborneAnimation;
};

/** DA_Player, Player.json */
UCLASS(BlueprintType)
class UAgPlayerData : public UAgCharacterData
{
	GENERATED_BODY()

public:
	/** 선입력: 허용되지 않는 상태에서 누른 입력을 기억하는 시간 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Input", meta = (Units = "s", ClampMin = 0))
	float InputBufferTime = 0.f;

	/** 평타: 타마다 하나씩, 1타부터 */
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

	/** 자원 회복: 가드 중이 아닐 때 리젠 대기 시간이 지난 뒤의 초당 SP 회복량 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Resources", meta = (ClampMin = 0))
	float SPRegenRate = 0.f;

	/** 자원 회복: 가드 중의 초당 SP 회복량 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Resources", meta = (ClampMin = 0))
	float SPRegenRateGuarding = 0.f;

	/** 자원 회복: 마지막으로 SP를 소모한 뒤의 리젠 대기 시간 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Resources", meta = (Units = "s", ClampMin = 0))
	float SPRegenDelay = 0.f;

	/** 자원 회복: SP가 0이 됐을 때의 리젠 대기 시간 */
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

	/** SP 소모: 회피. SP가 0보다 크면 소모량보다 적어도 쓸 수 있습니다. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Dodge", meta = (ClampMin = 0))
	float DodgeSPCost = 0.f;

	/** 회피: 쿨다운 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Dodge", meta = (Units = "s", ClampMin = 0))
	float DodgeCooldown = 0.f;

	/** 회피: 무적 구간. 시작부터 이 시간까지 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Dodge", meta = (Units = "s", ClampMin = 0))
	float DodgeInvincibleTime = 0.f;

	/** 회피: 극한 회피 구간. 시작부터 이 시간까지 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Dodge", meta = (Units = "s", ClampMin = 0))
	float PerfectDodgeTime = 0.f;

	/** 회피: 구를 때의 후딜 시작 (시작부터) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Dodge", meta = (Units = "s", ClampMin = 0))
	float RollRecoveryStart = 0.f;

	/** 회피: 뒤로 물러날 때의 후딜 시작 (시작부터) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Dodge", meta = (Units = "s", ClampMin = 0))
	float BackstepRecoveryStart = 0.f;

	/** SP 소모: 가드로 막은 공격. 가드 경감 배율 적용 전 HP 감소량에 곱하는 비율 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Guard", meta = (ClampMin = 0))
	float GuardSPCostRatio = 0.f;

	/** 이동: 가드 중 이동 속도 (MOV의 배수) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Guard", meta = (ClampMin = 0))
	float GuardMoveSpeedRatio = 1.f;

	/** 패리: 패리 구간. 가드를 새로 시작한 순간부터 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Guard", meta = (Units = "s", ClampMin = 0))
	float ParryWindowTime = 0.f;

	/** 상태별 허용 입력: 패리 구간이 생기는 가드 시작 후 이동할 수 없는 시간 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Guard", meta = (Units = "s", ClampMin = 0))
	float GuardMoveLockTime = 0.f;

	/** 패리: 공격자가 잃는 PP (공격자 MaxPP에 대한 비율) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Guard", meta = (ClampMin = 0, ClampMax = 1))
	float ParryPPRatio = 0.f;

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

	/** 베기 선: 위치. 카메라가 보는 쪽으로 플레이어 앞 이 거리 안 (cm) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Ultimate")
	FFloatInterval UltimateSlashDistance = FFloatInterval(0.f, 0.f);

	/** 베기 선: 위치. 바닥에서 이 높이 안 (cm) */
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

	/** 처형. TargetingRange가 처형 타겟팅 범위입니다. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Execution")
	FAgPlayerAttack Execution;

	/**
	 * 처형의 첫 베기까지 플레이어가 보스 정면에 붙는 간격(캡슐 중심 사이 수평 거리). 두 처형 모션이 맞물리는 간격으로,
	 * 기획서에 수치가 없어 두 모션의 원래 배치에서 잰 값입니다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Execution", meta = (Units = "cm", ClampMin = 0))
	float ExecutionDistance = 0.f;

	/** 락온 타겟팅 범위 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Lock-On", meta = (Units = "cm", ClampMin = 0))
	float LockOnRange = 0.f;

	/** 락온 해제 조건: 대상이 이 거리보다 멀어지면 락온을 풉니다. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Lock-On", meta = (Units = "cm", ClampMin = 0))
	float LockOnReleaseRange = 0.f;

	/**
	 * TP_Enemy: 공격 중 이동, 락온, 처형, 궁극기 마무리의 대상 후보(플레이어 주변의 캐릭터)를 모읍니다. 범위와 조건은
	 * 쓰는 곳마다 따로 확인합니다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	TObjectPtr<UTargetingPreset> EnemyTargeting;

	/** 이동 '달리기': 속도 (MOV의 배수) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Sprint", meta = (ClampMin = 0))
	float SprintSpeedRatio = 1.f;

	/** SP 소모: 달리기의 초당 소모량. SP가 0보다 크면 달릴 수 있습니다. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Sprint", meta = (ClampMin = 0))
	float SprintSPCost = 0.f;

	/** 회피 반격 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Dodge Counter")
	FAgPlayerAttack DodgeCounter;

	/** 회피 반격 기회: 유효 시간. 극한 회피가 발동한 순간부터 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Dodge Counter", meta = (Units = "s", ClampMin = 0))
	float DodgeCounterChanceTime = 0.f;

	/** 슬로우모션: 극한 회피 성공의 속도 (게임 전체 시간 배율) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Slow Motion", meta = (ClampMin = 0.01, ClampMax = 1))
	float SlowMotionSpeed = 1.f;

	/** 슬로우모션: 극한 회피 성공의 지속 시간 (실제 시간) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Slow Motion", meta = (Units = "s", ClampMin = 0))
	float SlowMotionDuration = 0.f;

	/** 평타 몽타주. BasicAttack과 같은 순서 */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TArray<TObjectPtr<UAnimMontage>> BasicAttackMontages;

	/** 회피: 구르기 (이동 입력 방향) */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> RollMontage;

	/** 회피: 뒤로 물러나기 (이동 입력이 없을 때) */
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

	/** 점프: 착지 모션. 이동 입력 없이 착지할 때 재생합니다. */
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

	/** 넉업: 상승 자세. 정점 무렵 KnockUpFallMontage가 블렌드되어 들어올 때까지 반복합니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> KnockUpMontage;

	/**
	 * 넉업: 섹션 Fall(낙하 자세, 반복) → Land(바닥에 떨어짐). 착지하면 Land로 넘깁니다.
	 * 블렌드 인 시간의 가운데가 정점에 옵니다.
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

	/** 회전 정지 시간(초)을 패턴 몽타주의 시간으로 바꾼 값 */
	virtual float GetRotationStopLeadInMontage() const override { return RotationStopLead * PatternPlayRate; }

	/** 패턴 목록 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Patterns")
	TArray<FAgBossPattern> Patterns;

	/** 모션 재생 속도: 공격 패턴 모션의 재생 속도 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Pattern Rules", meta = (ClampMin = 0.01))
	float PatternPlayRate = 1.f;

	/** 패턴 선택: 근거리의 상한 (캡슐 중심 사이 수평 거리) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "AI", meta = (Units = "cm", ClampMin = 0))
	float NearDistance = 0.f;

	/** 패턴 선택: 중거리의 상한. 이보다 멀면 원거리입니다. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "AI", meta = (Units = "cm", ClampMin = 0))
	float FarDistance = 0.f;

	/** 패턴 선택: 후보가 없을 때 플레이어에게 걸어가며 다시 고르는 간격 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "AI", meta = (Units = "s", ClampMin = 0.01))
	float RetryInterval = 0.5f;

	/** 패턴 후딜레이 대기 시간: 1페이즈에서 공격 패턴을 실행한 뒤 (범위 안에서 무작위, 초) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "AI")
	FFloatInterval PatternRecoveryWaitPhase1 = FFloatInterval(0.f, 0.f);

	/** 패턴 후딜레이 대기 시간: 2페이즈에서 공격 패턴을 실행한 뒤 (범위 안에서 무작위, 초) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "AI")
	FFloatInterval PatternRecoveryWaitPhase2 = FFloatInterval(0.f, 0.f);

	/** 패턴 후딜레이 대기 시간: 레벨 시작, 넉백, 페이즈 전환, 그로기 유지, 처형 피격 뒤 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "AI", meta = (Units = "s", ClampMin = 0))
	float FixedRecoveryWait = 0.f;

	/** 페이즈: HP가 MaxHP의 이 비율 이하가 되면 2페이즈입니다. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Phase", meta = (ClampMin = 0, ClampMax = 1))
	float Phase2HPRatio = 0.f;

	/** 돌진 찌르기 돌진 규칙: 속도 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Dash Thrust", meta = (Units = "cm/s", ClampMin = 0))
	float DashSpeed = 0.f;

	/** 돌진 찌르기 돌진 규칙: 목표 지점. 플레이어 위치보다 이만큼 더 나아간 지점 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Dash Thrust", meta = (Units = "cm", ClampMin = 0))
	float DashOvershoot = 0.f;

	/** 돌진 찌르기 돌진 규칙: 최대 이동 거리 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Dash Thrust", meta = (Units = "cm", ClampMin = 0))
	float DashMaxDistance = 0.f;

	/** 도약 내려찍기 체공 시간: 도약부터 착지까지. 엔진 중력과 함께 최고 높이를 정합니다. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Leap Slam", meta = (Units = "s", ClampMin = 0.01))
	float LeapAirTime = 1.f;

	/** 도약 내려찍기: 착지 목표. 플레이어 위치에서 보스 쪽으로 이만큼 떨어진 지점 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Leap Slam", meta = (Units = "cm", ClampMin = 0))
	float LeapTargetOffset = 0.f;

	/** 도약 내려찍기: 최대 도약 거리 (수평) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Leap Slam", meta = (Units = "cm", ClampMin = 0))
	float LeapMaxDistance = 0.f;

	/** 도약 내려찍기: 착지 지점 반경. 착지가 이 안의 대상을 맞힙니다. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Leap Slam", meta = (Units = "cm", ClampMin = 0))
	float LeapHitRadius = 0.f;

	/** 백스텝: 플레이어와의 거리. 플레이어가 이 안에 있어야 발동합니다 (캡슐 중심 사이 수평 거리). */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Backstep", meta = (Units = "cm", ClampMin = 0))
	float BackstepRange = 0.f;

	/** 백스텝: 발동 확률. 플레이어가 평타의 각 타나 점프 공격을 시작할 때마다 검사합니다. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Backstep", meta = (ClampMin = 0, ClampMax = 1))
	float BackstepChance = 0.f;

	/** 백스텝: 뒤로 물러나는 거리. 모션의 루트 모션 이동을 이 거리에 맞춥니다. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Backstep", meta = (Units = "cm", ClampMin = 0))
	float BackstepDistance = 0.f;

	/** SwordWave: 투사체 (BP_SwordWave). 판정 상자(폭, 높이)와 모습은 블루프린트에서 정합니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Sword Wave")
	TSubclassOf<AAgSwordWave> SwordWaveClass;

	/** 검기 발사: 속도 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Sword Wave", meta = (Units = "cm/s", ClampMin = 0))
	float SwordWaveSpeed = 0.f;

	/** 검기 발사: 최대 사거리 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Sword Wave", meta = (Units = "cm", ClampMin = 0))
	float SwordWaveRange = 0.f;

	/** 락온 지점(가슴 높이)의 본. 락온 카메라가 이곳을 보고, 락온 마커도 여기에 표시합니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Lock-On")
	FName LockOnBone;

	/** 이름. 전투 HUD '보스 상태'에 표시합니다. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "UI")
	FText DisplayName;

	/** 전투 HUD '처형 안내': 이 본 위에 안내를 표시합니다 (보스 머리 위). */
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

	/** 그로기: 섹션 Start → Loop(반복) → End. 그로기 시간이 끝날 때 모션도 끝나도록 End로 넘깁니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> GroggyMontage;

	/**
	 * 처형 피격: 플레이어의 처형과 짝을 이루는 모션으로, 끝에서 보스가 일어섭니다. 처형의 첫 베기까지 첫 프레임에
	 * 머물러 있어 처형 몽타주와 같은 프레임에 시작해 박자를 맞춥니다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> ExecutedMontage;

	/** 처형 사망: 마지막 일격 뒤까지는 ExecutedMontage와 같고, 그 뒤 쓰러진 채로 끝납니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> ExecutedDeathMontage;

	/** 페이즈 전환: 포효 */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Montages")
	TObjectPtr<UAnimMontage> PhaseTransitionMontage;
};
