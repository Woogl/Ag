// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/AgAbilityData.h"
#include "Math/Interval.h"
#include "AgCharacterData.generated.h"

class UAnimMontage;
class UStateTree;
class UTargetingPreset;

/** 플레이어 사양·보스 사양의 '스탯' 표: 스탯(어트리뷰트)마다 초기값과 최대값 */
USTRUCT(BlueprintType)
struct FAgAttributes
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Attributes", meta = (ClampMin = 0))
	float HP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attributes", meta = (ClampMin = 0))
	float MaxHP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attributes", meta = (ClampMin = 0))
	float SP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attributes", meta = (ClampMin = 0))
	float MaxSP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attributes", meta = (ClampMin = 0))
	float MP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attributes", meta = (ClampMin = 0))
	float MaxMP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attributes", meta = (ClampMin = 0))
	float UP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attributes", meta = (ClampMin = 0))
	float MaxUP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attributes", meta = (ClampMin = 0))
	float PP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attributes", meta = (ClampMin = 0))
	float MaxPP = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attributes", meta = (ClampMin = 0))
	float ATK = 0.f;

	UPROPERTY(EditAnywhere, Category = "Attributes", meta = (ClampMin = 0))
	float DEF = 0.f;

	/** 기본 이동 속도 */
	UPROPERTY(EditAnywhere, Category = "Attributes", meta = (ClampMin = 0, Units = "cm/s"))
	float MOV = 0.f;
};

/**
 * 플레이어와 보스가 함께 쓰는 데이터(구현 계획서 'JSON 데이터'). Characters.json의 캐릭터 항목 하나입니다.
 * 수치·설정·에셋 참조는 모두 Transient이며, 에셋 매니저가 JSON을 검증하고 한 번에 채웁니다. 객체는 에셋 매니저가 들고 있습니다.
 * 필드마다 기획서의 어느 항목인지는 데이터 위치표(Docs/Program/데이터 위치표.md)를 따릅니다.
 */
UCLASS(Abstract, BlueprintType)
class UAgCharacterData : public UObject
{
	GENERATED_BODY()

public:
	/** 최초 로딩 실패 시 게임 시작을 막습니다. 다시 읽기 실패 시 이전 정상 값은 유지합니다. */
	bool HasValidJsonData() const
	{
		return bHasValidJsonData;
	}

	void MarkJsonDataLoaded()
	{
		bHasValidJsonData = true;
	}

	/** 이 캐릭터의 어빌리티 목록에 있는 ID면 Abilities.json의 정의를 돌려줍니다. 없으면 null입니다. */
	const FAgAbilityData* FindAbility(FName Id) const;

	/** 몽타주 ID로 몽타주를 찾습니다. 없으면 오류 로그를 남기고 null입니다. */
	UAnimMontage* FindMontage(FName Id) const;

	/** 이름. 전투 HUD '보스 상태'에 표시합니다. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "UI")
	FText DisplayName;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Attributes", meta = (ShowOnlyInnerProperties))
	FAgAttributes Attributes;

	/**
	 * 어빌리티 목록: 이 캐릭터가 가진 어빌리티(행동, 보스 패턴, 상태)의 Abilities.json ID. 캐릭터는 시작할 때 이 순서로
	 * 항목마다 단일 UAgGameplayAbility를 ID별 독립 스펙으로 부여합니다.
	 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Abilities")
	TArray<FName> Abilities;

	/** 사망 연출 시간. 끝나면 결과 UI를 띄웁니다. (플레이어 사양·보스 사양 '사망') */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Death", meta = (Units = "s", ClampMin = 0))
	float DeathPresentationTime = 0.f;

	/** 공격 중 이동: 정지 간격 (캡슐 중심 사이 수평 거리) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Attack Movement", meta = (Units = "cm", ClampMin = 0))
	float StopDistance = 0.f;

	/** 시작할 때 거는 효과의 Effects.json ID(SP·PP 리젠, 그로기 면역) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Abilities")
	TArray<FName> Effects;

	/** 몽타주 ID마다 그 몽타주. ID는 구현 계획서 'JSON 데이터'의 '어빌리티의 데이터'를 따릅니다. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Assets")
	TMap<FName, TObjectPtr<UAnimMontage>> Montages;

private:
	bool bHasValidJsonData = false;
};

/** Characters.json의 Player */
UCLASS(BlueprintType)
class UAgPlayerData : public UAgCharacterData
{
	GENERATED_BODY()

public:
	/** 입력에서 사용하는 콤보 모션 순서. 타격 수치는 몽타주 시간표가 소유합니다. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Abilities")
	TArray<FName> BasicAttackMontageIds;

	/** 선입력: 허용되지 않는 상태에서 누른 입력을 기억하는 시간 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Input", meta = (Units = "s", ClampMin = 0))
	float InputBufferTime = 0.f;

	/** SP 소모: 가드로 막은 공격. 가드 경감 배율 적용 전 HP 감소량에 곱하는 비율 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Guard", meta = (ClampMin = 0))
	float GuardSPCostRatio = 0.f;

	/** 이동: 가드 중 이동 속도 (MOV의 배수) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Guard", meta = (ClampMin = 0))
	float GuardMoveSpeedRatio = 1.f;

	/** 패리: 공격자가 잃는 PP (공격자 MaxPP에 대한 비율) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Guard", meta = (ClampMin = 0, ClampMax = 1))
	float ParryPPRatio = 0.f;

	/** 슬로우모션: 궁극기 마무리 적중의 속도 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Slow Motion", meta = (ClampMin = 0.01, ClampMax = 1))
	float UltimateSlowMotionSpeed = 1.f;

	/** 슬로우모션: 궁극기 마무리 적중의 지속 시간 (실제 시간) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Slow Motion", meta = (Units = "s", ClampMin = 0))
	float UltimateSlowMotionDuration = 0.f;

	/**
	 * 처형의 찌르기까지 플레이어가 보스 정면에 붙는 간격(캡슐 중심 사이 수평 거리). 두 처형 모션이 맞물리는 간격으로,
	 * 기획서에 수치가 없어 두 모션의 원래 배치에서 잰 값입니다.
	 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Execution", meta = (Units = "cm", ClampMin = 0))
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
	UPROPERTY(Transient, VisibleAnywhere, Category = "Targeting")
	TObjectPtr<UTargetingPreset> EnemyTargeting;

	/** 이동 '달리기': 속도 (MOV의 배수) */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Sprint", meta = (ClampMin = 0))
	float SprintSpeedRatio = 1.f;
};

/** Characters.json의 Boss */
UCLASS(BlueprintType)
class UAgBossData : public UAgCharacterData
{
	GENERATED_BODY()

public:
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

	/** 패턴 후딜레이 대기 시간: 레벨 시작, 넉업, 넉백, 페이즈 전환, 그로기 유지, 처형 피격 뒤 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "AI", meta = (Units = "s", ClampMin = 0))
	float FixedRecoveryWait = 0.f;

	/** 페이즈: HP가 MaxHP의 이 비율 이하가 되면 2페이즈입니다. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Phase", meta = (ClampMin = 0, ClampMax = 1))
	float Phase2HPRatio = 0.f;

	/** 락온 지점(가슴 높이)의 본. 락온 카메라가 이곳을 보고, 락온 마커도 여기에 표시합니다. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Lock-On")
	FName LockOnBone;

	/** 전투 HUD '처형 안내': 이 본 위에 안내를 표시합니다 (보스 머리 위). */
	UPROPERTY(Transient, VisibleAnywhere, Category = "UI")
	FName ExecutionPromptBone;

	/** 보스 AI의 흐름(ST_Boss). 보스 AI 컨트롤러가 빙의할 때 실행합니다. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "AI")
	TObjectPtr<UStateTree> AIStateTree;
};
