// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/AgAttackData.h"
#include "Engine/DataAsset.h"
#include "Math/Interval.h"
#include "AgCharacterData.generated.h"

class AAgProjectile;
class FJsonObject;
class UAgGameplayAbility;
class UAnimationAsset;
class UAnimMontage;
class UGameplayEffect;
class ULevelSequence;
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
 * 플레이어와 보스가 함께 쓰는 데이터(구현 계획서 'JSON과 DataAsset').
 * 데이터 에셋(DA_Player, DA_Boss)에는 에셋 참조와 기획서에 없는 값(구현 값, 어빌리티 클래스, 본 이름)을 저장합니다.
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

	/**
	 * 에셋 매니저가 ValuesFile의 JSON을 검사하기 전에 부릅니다. 어빌리티 항목마다 그 어빌리티가 정한 전용 값의
	 * 구조체(GetAbilityParamsType)로 Params를 읽게 하고(UAgAssetManager::SetJsonStructType), 정할 수 없으면 Errors에
	 * 더합니다. 파일은 바꾸지 않습니다.
	 */
	void ResolveJsonStructTypes(FJsonObject& Json, TArray<FString>& Errors) const;

	/** 어빌리티 목록에서 ID로 항목을 찾습니다. 없으면 null입니다. */
	const FAgAbilityData* FindAbility(FName Id) const;

	/** 어빌리티 ID마다의 어빌리티 클래스. 없으면 null입니다. */
	TSubclassOf<UAgGameplayAbility> FindAbilityClass(FName Id) const;

	/** 몽타주 ID로 몽타주를 찾습니다. 없으면 오류 로그를 남기고 null입니다. */
	UAnimMontage* FindMontage(FName Id) const;

	/** 애니메이션 ID로 찾습니다. 없거나 T가 아니면 null입니다(가드하지 않는 캐릭터의 가드 이동처럼 없어도 됩니다). */
	template <typename T>
	T* FindAnimation(FName Id) const
	{
		return Cast<T>(Animations.FindRef(Id));
	}

	/** Transient 필드를 채우는 프로젝트 Data 폴더의 JSON 파일 (Player.json, Boss.json) */
	UPROPERTY(EditDefaultsOnly, Category = "Data")
	FString ValuesFile;

	/** 이름. 전투 HUD '보스 상태'에 표시합니다. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "UI")
	FText DisplayName;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Attributes", meta = (ShowOnlyInnerProperties))
	FAgAttributes Attributes;

	/**
	 * 어빌리티 목록: 이 캐릭터가 가진 어빌리티(행동, 보스 패턴, 상태)마다 항목 하나. 캐릭터는 시작할 때 항목마다
	 * AbilityClasses에서 ID로 찾은 클래스를 부여합니다.
	 */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Abilities")
	TArray<FAgAbilityData> Abilities;

	/** PP 리젠: 리젠 대기 시간이 지난 뒤의 초당 회복량 (전투 시스템 'PP 리젠'). 0이면 회복하지 않습니다. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "Resources", meta = (ClampMin = 0))
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

	/** 어빌리티 ID마다 그 어빌리티 클래스. 어빌리티 목록의 항목마다 하나씩 둡니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TMap<FName, TSubclassOf<UAgGameplayAbility>> AbilityClasses;

	/** 시작할 때 거는 GameplayEffect(SP·PP 리젠, 달리기의 SP 소모, PP 무한). 회복량은 각 효과가 이 데이터에서 읽습니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TArray<TSubclassOf<UGameplayEffect>> Effects;

	/** 몽타주 ID마다 그 몽타주. ID는 구현 계획서 'JSON과 DataAsset'의 '어빌리티의 데이터'를 따릅니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets")
	TMap<FName, TObjectPtr<UAnimMontage>> Montages;

	/** 애니메이션 ID마다 그 애니메이션(이동·가드 이동 블렌드 스페이스, 공중 반복 모션). ID는 UAgAnimInstance가 정합니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets")
	TMap<FName, TObjectPtr<UAnimationAsset>> Animations;
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

	/** 궁극기 컷신(LS_UltimateCutscene). 카메라만 담고, 플레이어 위치·방향을 기준으로 재생합니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Ultimate")
	TObjectPtr<ULevelSequence> UltimateCutscene;

	/**
	 * 처형의 찌르기까지 플레이어가 보스 정면에 붙는 간격(캡슐 중심 사이 수평 거리). 두 처형 모션이 맞물리는 간격으로,
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
};

/** DA_Boss, Boss.json */
UCLASS(BlueprintType)
class UAgBossData : public UAgCharacterData
{
	GENERATED_BODY()

public:
	/** 회전 정지 시간(초)을 패턴 몽타주의 시간으로 바꾼 값 */
	virtual float GetRotationStopLeadInMontage() const override { return RotationStopLead * PatternPlayRate; }

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

	/** SwordWave: 투사체 (BP_SwordWave). 판정 상자(폭, 높이)와 모습은 블루프린트에서 정합니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Assets|Sword Wave")
	TSubclassOf<AAgProjectile> SwordWaveClass;

	/** 락온 지점(가슴 높이)의 본. 락온 카메라가 이곳을 보고, 락온 마커도 여기에 표시합니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Lock-On")
	FName LockOnBone;

	/** 전투 HUD '처형 안내': 이 본 위에 안내를 표시합니다 (보스 머리 위). */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	FName ExecutionPromptBone;

	/** 보스 AI의 흐름(ST_Boss). 보스 AI 컨트롤러가 빙의할 때 실행합니다. */
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TObjectPtr<UStateTree> AIStateTree;
};
