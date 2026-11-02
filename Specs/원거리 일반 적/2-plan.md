# 원거리 일반 적 — 경비 사수 기술 설계 (Plan)

## 1. 영향 범위 및 아키텍처

`BP_Gunner`는 절단자처럼 `AAgEnemyCharacter`를 바로 상속합니다. 사격 규칙(사격 조건, 조준점 고정, 조준점으로 발사)은 어빌리티 하나에만 묶지 않고 다른 원거리 공격도 쓸 수 있는 범용 Task·타겟팅·StateTree 조건으로 만듭니다. Task는 어빌리티를 끝내거나 취소하지 않고, 조준 중 조건이 깨지면 패턴을 실행한 StateTree가 취소합니다.

| 대상 | 변경 |
| - | - |
| `AgTargeting::IsVisible` (신규) | 출처가 대상을 볼 수 있는지 판정합니다. 출처가 AI면 엔진 AI 지각의 시야가 대상을 지금 감지하고 있어야 하고(`HasActiveStimulus`, 적 시스템의 시야 거리·정면 각도·지형 가림 그대로), 소켓을 주면 출처 메시의 그 소켓에서 대상 캡슐 중심까지 WorldStatic이 가리지 않아야 합니다. 아래 필터와 StateTree 조건이 함께 씁니다(`IsInRange`처럼 찾기와 유지가 같은 판정). |
| `UAgTargetingFilterTask_Visible` (신규) | `IsVisible`을 통과한 대상만 남기는 타겟팅 필터. 소켓은 필터 속성(`Socket`)입니다. |
| `State.Aiming` (신규 태그), `Aiming` (JSON 효과) | 조준 중임을 나타냅니다. 시간표의 ApplyEffect 구간(조준 시작 ~ 발사)이 효과로 붙입니다. |
| `FAgEnemyAICondition_AimLost` (신규, '적: 조준 대상 놓침') | `State.Aiming`이 있는데 `IsVisible`(소켓은 조건 속성 `MuzzleSocket`)이 거짓이거나, 어빌리티에 조준점이 있고 캡슐 중심 → 총구 → 조준점 선이 WorldStatic에 막히면 참입니다. |
| `FAgEnemyAITask_RunPattern` | 상태를 나갈 때 아직 도는 패턴을 취소합니다(`ExitState`). 다른 전환(피격 반응·플레이어 사망·사망)은 이미 다른 쪽에서 패턴을 끊으므로 동작이 같습니다. |
| `AAgEnemyAIController` | `ConfigurePerception`이 감각 값을 바꾼 뒤 엔진 `RequestStimuliListenerUpdate`로 청취자도 다시 읽게 합니다. 게임 도중 생긴 적은 빙의 전에 청취자로 등록돼 팀이 비어 있어 플레이어를 보지 못했습니다(검증 중 발견, 레벨에 배치한 적은 영향 없음). |
| `ST_Enemy` | '패턴 실행'에 틱 전환(조건 '조준 대상 놓침', `MuzzleSocket` `Muzzle`) → '패턴 후딜레이'를 더합니다. 보스·근거리 적은 `State.Aiming`이 없어 이 전환이 걸리지 않습니다. |
| `UAgAbilityTask_Aim` (신규, 구간 항목) | 구간 동안 행동 대상의 캡슐 중심을 어빌리티의 조준점으로 매 틱 갱신합니다. 구간이 끝나면 마지막 값이 남아 조준점이 고정됩니다. |
| `UAgGameplayAbility` | 조준점(`AimLocation`)과 조회·설정 함수. 발동할 때 비웁니다. 행동 대상처럼 한 실행에 속한 상태라 어빌리티에 둡니다. |
| `AAgProjectile` | 판정 모양을 C++의 상자(`HitBox`)에서 BP에 둔 모양 컴포넌트(상자 또는 캡슐) 하나로 바꿉니다. 시작할 때 찾아 자체 충돌을 끄고, 매 틱 그 컴포넌트의 자리·방향·모양으로 쓸며 액터는 판정이 간 만큼 옮깁니다. C++이 만들던 `Mesh`도 BP 컴포넌트가 됩니다. |
| `BP_SwordWave` | C++ 컴포넌트가 사라지므로 같은 값으로 BP 컴포넌트를 다시 둡니다: 루트 상자(반 크기 20·100·50), 그 아래 메시(`SM_Cube`, 위치 −20·−100·−50, 배율 0.4·2·1, `MI_SwordWave`, 사용자 지정 프리미티브 데이터 1·0.5·0.75·1·0.5, 그림자 끔, Nanite 끔, 충돌 없음). |
| `UAgAbilityTask_SpawnProjectile` | 어빌리티에 조준점이 있으면 발사 위치에서 조준점으로(상하 포함) 쏩니다. 조준점이 없으면 지금처럼 몸 정면 수평으로 쏩니다(`SwordWave` 변경 없음). |

- 그대로 맞는 기존 동작(변경 없음): 투사체의 캐릭터 관통·같은 대상 한 번·벽 소멸·사거리·패리 반사(발사자 쪽으로, 이동 거리 다시 셈)·발사자 사망 뒤에도 비행(`AAgProjectile`), 같은 진영 무시(`CanBeHitBy`), 시간 정지·슬로우모션(게임 전체 시간 배율), 강인도 소형(움찔이 패턴을 끊음), 패턴이 취소되거나 끝나면 패턴 후딜레이(`ST_Enemy`의 패턴 실행), 쿨다운은 발동 때 시작해 취소돼도 남음, 일반 적의 대미지 숫자 숨김([근거리 Plan](<../근거리 일반 적/2-plan.md>)).
- 사격 조건과 Spec의 대응:
  - 조준 시작: `PistolShot`의 타겟팅이 `bRequired`이고 프리셋에 `Visible`(소켓 `Muzzle`)이 있어, 보이지 않거나 총구가 가리면 발동할 수 없습니다. 그래서 패턴 후보에서 빠지고 공통 규칙대로 다가가며 다시 고릅니다. 쿨다운도 쓰지 않습니다.
  - 조준 중과 발사 직전: `Aiming` 구간(조준 시작 ~ 발사) 동안 '조준 대상 놓침'이 참이 되면 StateTree가 '패턴 후딜레이'로 가며 패턴을 취소합니다. 몽타주와 예고가 함께 멈춥니다. 조준점이 고정된 뒤에는 고정 조준점까지의 가림도 봅니다.
  - 조준점 고정: Aim 구간이 발사 0.35초 전에 끝납니다.
- 회전: 패턴 중에는 몽타주 재생 중이라 돌지 않으므로, 조준 추적(조준점 고정까지)과 재장전 중 다시 바라보기는 RootMotion 회전 구간으로 둡니다. 권총 모션은 제자리 모션이라 몽타주의 시퀀스에서 루트 모션을 켜야 회전 워핑이 걸립니다(이동량은 0).
- [기본 방침](<../../Docs/Architecture/기본 방침.md>): 새 Task는 Aim 하나이고 한 가지 일만 합니다. 새 태그는 `State.Aiming` 하나입니다.

## 2. GAS 및 데이터 설계

### JSON

| 파일 | 항목 |
| - | - |
| `Characters.json` | `Gunner`: 표시 이름 경비 사수, 소형, HP 300, PP 200, ATK 90, DEF 0, MOV 200, 어빌리티(`PistolShot`, `Flinch`·`KnockDown`·`KnockUp`·`Juggle`·`Groggy`·`Executed`·`Parried`·`EnemyLockOn`), `PostPatternDelayTime` 2 ~ 4초, `SightDistance` 2000, `SightAngle` 120, 시작 효과 `GunnerPPRegen`, 몽타주 |
| `Abilities.json` | `PistolShot`: `Ability.Enemy.Pattern`, 쿨다운 6초, 발동 차단 태그는 보스 패턴과 같음, 타겟팅 `Shooting` 2000cm `bRequired`, 거리 0 ~ 2000cm·가중치 1 |
| `TargetingPresets.json` | `Shooting`: 구 선택, 적대, 사망 제외, `Visible`(소켓 `Muzzle`), 거리순 |
| `Projectiles.json` | `PistolBullet`: `BP_PistolBullet`, 초당 2000cm, 사거리 3000cm, HP 1.0·움찔·가드 가능(PP 배율 0) |
| `Effects.json` | `GunnerPPRegen`: `PPRegen`과 같고 초당 15. `Aiming`: `Infinite`, `State.Aiming` 부여 |
| `MontageTimelines.json` | `AM_Gunner_PistolShot` 아래 표 |

| 섹션 | 항목 |
| - | - |
| `Aim` (1.2초) | ApplyEffect `Aiming` 0 ~ 1.2, RootMotion 0 ~ 0.85 (회전만), Aim 0 ~ 0.85, 1.2에 BlendTime 0.1 Transition → `Fire` |
| `Fire` (0.8초) | SpawnProjectile 0 (`PistolBullet`, `Muzzle`), 0.8에 BlendTime 0.2 Transition → `Reload` |
| `Reload` (2.0초) | RootMotion 0 ~ 2.0 (회전만, 다시 바라보기) |

- 시간 값은 Spec 2.3의 구간 길이입니다. 몽타주에 맞출 때 바뀌면 구현 전에 Spec을 먼저 고칩니다.
- 패리 예고는 몽타주의 노티파이로 발사 0.3초(패리 구간 길이) 전부터 발사까지 무기에 둡니다.

### 에셋

| 에셋 | 내용 |
| - | - |
| 스켈레톤 소켓 `Muzzle` | `SK_Mannequin`의 `hand_r` 아래, 권총 대용 메시의 앞끝. 발사 위치와 가림 검사의 기준입니다. 권총 메시가 정해지면 위치만 옮깁니다. |
| 권총 대용 | 엔진 기본 상자를 작게 줄인 것(아래 '결정한 항목' 1) |
| `BP_PistolBullet` | `AAgProjectile`의 자식. 판정은 반지름 5cm 캡슐을 진행 방향으로 눕혀 둡니다(길이는 탄체에 맞춤). 메시는 엔진 기본 구를 진행 방향으로 늘린 발광 탄체로 궤적을 대신합니다. 머티리얼은 `BP_SwordWave`처럼 기존 머티리얼에 사용자 지정 프리미티브 데이터를 줍니다(새 외형 에셋 없음). |
| `AM_Gunner_PistolShot` | `Aim`은 `MF_Pistol_Idle_ADS`, `Fire`는 `MM_Pistol_Fire`를 0.8초에 맞춘 재생 속도, `Reload`는 `MM_Pistol_Reload`(2.0초). `MM_Pistol_Fire`는 애디티브라 몽타주 슬롯에서 그대로 쓸 수 없으므로 애디티브 설정만 끈 복제본을 씁니다. 원본 데이터가 전신 자세가 아니면 `Fire`에도 조준 자세를 쓰고 알려 드립니다. 세 시퀀스의 루트 모션을 켭니다. 대기(총 내림)에서 겨누는 연결은 몽타주 블렌드 인으로 보여 줍니다. |
| 공용 재사용 | 피격 `AM_Flinch`·`AM_FlinchAir`·`AM_KnockDown`·`AM_KnockUp`, 그로기 `AM_Boss_Groggy`, 처형 피격·사망 `AM_Boss_Executed`·`AM_Boss_ExecutedDeath` |
| `BS_GunnerMove` | 맨손 대기 `MM_Idle`과 맨손 걷기 `MF_Unarmed_Walk_*` 8방향. 대기와 걷기에서 총을 내린 자세가 됩니다. |
| `ABP_Gunner` | `ABP_Character`의 자식. 이동 `BS_GunnerMove`, 그로기 자세는 보스와 같음. 맨손 제자리 회전 모션이 없어 제자리 회전 모션은 비웁니다(발을 붙인 채 몸만 돎). 그래프는 바꾸지 않습니다. |
| `BP_Gunner` | 부모 `AAgEnemyCharacter`. `SKM_Manny_Simple`, 린과 같은 직립 신장, 권총 대용을 `hand_r`에 붙이고 `Weapon` 태그(예고 VFX가 붙는 곳). `ABP_Gunner`, 캐릭터 데이터 ID `Gunner`, `ST_Enemy`, 회전 속도 360°, 월드 위젯은 `BP_Cutter`와 같습니다. |

### 결정한 항목

1. **권총 대용**: 엔진 기본 상자(`/Engine/BasicShapes/Cube`)를 작게 줄여 손에 붙입니다. 새 에셋 없이 총을 겨누는 방향과 예고 VFX 위치가 보입니다.
2. **탄환 판정 모양**: 투사체가 상자와 캡슐을 모두 판정으로 쓸 수 있게 하고(위 `AAgProjectile`), 탄환은 반지름 5cm 캡슐을 씁니다. Spec의 '반경 5cm 구' 문구를 캡슐로 맞춥니다.
3. **상하 조준 자세(에임 오프셋)**: 이번에는 넣지 않습니다. 탄환은 상하 방향까지 조준점으로 가고 팔은 수평으로 겨눕니다. 플레이로 어색함을 본 뒤 정합니다(공용 `ABP_Character` 그래프에 노드를 더하는 일이라).

## 3. 인터페이스 & 함수 시그니처

```cpp
// AgTargeting.h
namespace AgTargeting
{
	/** 출처가 AI면 지각의 시야가 대상을 지금 감지하고, Socket이 있으면 그 소켓에서 대상 캡슐 중심까지 WorldStatic이 가리지 않습니다. */
	bool IsVisible(const AActor& Source, const AActor& Target, FName Socket);
}

UCLASS()
class UAgTargetingFilterTask_Visible : public UTargetingFilterTask_BasicFilterTemplate
{
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;

	UPROPERTY(EditAnywhere, Category = "Target Filter")
	FName Socket;
};

// AgEnemyAIConditions.h
USTRUCT(meta = (DisplayName = "적: 조준 대상 놓침", Category = "Ag|Enemy AI"))
struct FAgEnemyAICondition_AimLost : public FStateTreeAIConditionBase
{
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

	UPROPERTY(EditAnywhere, Category = "Condition")
	FName MuzzleSocket;
};

// AgEnemyAITasks.h (FAgEnemyAITask_RunPattern)
virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

// AgAbilityTask_Aim.h
USTRUCT()
struct FAgAimParams : public FAgTaskParams
{
	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

UCLASS()
class UAgAbilityTask_Aim : public UAbilityTask
{
	friend struct FAgAimParams;

public:
	UAgAbilityTask_Aim();
	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;

private:
	void UpdateAimLocation();

	UPROPERTY()
	FAgAimParams Params;
};

// AgGameplayAbility.h
const TOptional<FVector>& GetAimLocation() const;
void SetAimLocation(const FVector& Location);
```
