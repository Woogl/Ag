# 스킬 1 기술 설계 (Plan)

## 1. 영향 범위 및 아키텍처

### 지금 구조

- 공격 어빌리티의 대상은 어빌리티가 발동할 때 한 번 찾습니다(`FindTargetingTarget` → `AgTargeting::FindTarget`). 프리셋 후보에 락온 대상이 있으면 그 대상, 없으면 가장 가까운 후보입니다. 플레이어 공격은 프리셋의 `UAgTargetingSelectionTask_LockOnTarget`이 락온 대상을 `Range`와 관계없이 후보에 넣습니다(코드 리뷰 뒤 바뀜: 전에는 `Range`를 락온 해제 거리보다 넓은 50m로 두는 약속이었습니다). 처형은 이 '후보 가운데 락온 우선'을 기획서대로 씁니다.
- 추적 구간(RootMotion)은 행동 대상이 구간의 `TargetingRange` 밖이면 다가가지 않고 락온 대상 쪽으로 돌기만 하며, 대상이 없으면 락온 대상 쪽, 그것도 없으면 돌지 않습니다.
- 시야 필터(`UAgTargetingFilterTask_Visible`)는 `Socket`이 있으면 출처 메시의 그 소켓에서 대상까지 `WorldStatic` 가림을 보고, `Socket`을 생략하면 가림을 보지 않습니다. 지금은 경비 사수의 `Shooting`(`Muzzle`)만 씁니다.
- 구간 동안 상태를 붙이는 일은 태그를 주는 효과를 `ApplyEffect` 구간으로 겁니다(`Busy`, `Invincible`). 캐릭터는 몇몇 상태 태그를 받아 스스로 바뀝니다(`State.PassThrough`의 캡슐 충돌 등).
- Transition은 시작하자마자, 또는 `Event`가 오거나 `TagRemoved`가 빠지면 섹션을 옮깁니다. 조건부로 옮기는 방법은 없습니다.

### 모션 측정 (ACS2 Combo Skill 팩 `Attack09`, 1배속, 2.77초)

| 구간 | 측정값 |
| - | - |
| 0 ~ 0.2초 | 제자리에서 몸을 낮춥니다. 루트는 0.3초까지 15cm만 나아갑니다. |
| 0.6 ~ 0.7초 | 몸을 앞으로 던지며 칼을 쥔 오른손을 뒤로 당긴 자세입니다. 루트 전진 0.6초 2.36m, 0.7초 2.94m. |
| 0.7 ~ 0.95초 | 오른손이 앞으로 뻗어 찌릅니다(0.7 ~ 0.8초). 팩의 공격 노티파이는 0.79초입니다. |
| 0.95 ~ 1.5초 | 낮게 파고들며 감속합니다(1.0초 3.98m, 1.1초 4.13m, 1.5초 4.45m). 1.1초 뒤로는 두 발이 바닥에 붙어 있습니다. |
| 1.5 ~ 2.77초 | 일어섭니다(2.1초 4.71m, 끝 5.44m). |

- 0.62초부터 1.5초까지 약 2m를 나아가므로, 대상 등 뒤 3m에 나타나면 모션의 돌진만으로 대상 바로 뒤까지 파고듭니다.

### 바꾸는 곳

| 대상 | 바뀌는 내용 | 공통 규칙 |
| - | - | - |
| `UAgAbilityTask_Teleport` (신규, `UAbilityTask`) · `FAgTeleportParams` | 구간 Task. 시작할 때 행동 대상의 등 뒤로 순간이동하고, 공중에 나타났으면 구간이 끝날 때까지 그 높이에 머뭅니다. 갈 수 없으면 그 자리에 있습니다. 어느 쪽이든 나타나는 자리에서 `GameplayCue.Teleport`를 실행합니다. 아래 '순간이동 규칙'. | 둘 이상의 어빌리티가 쓸 수 있는 기능 하나입니다(적 패턴도 같은 Task로 플레이어 뒤에 나타날 수 있습니다). 나타날 자리를 찾는 함수는 이 Task만 쓰므로 파일 안 익명 네임스페이스에 둡니다. |
| `UAgTargetingFilterTask_LockOnTarget` (신규, `UTargetingFilterTask_BasicFilterTemplate`) | 출처에게 락온 대상이 있으면 그 대상이 아닌 후보를 걸러 냅니다. 락온 중이 아니면 거르지 않습니다. 같은 프리셋의 `Visible`이 가려진 락온 대상을 거르면 후보가 없어 대상이 없습니다. | 명세 '대상'의 "락온 대상이 가려져 있으면 다른 적을 고르지 않음"을 엔진 Targeting System의 필터로 프리셋에 넣습니다. 어빌리티 데이터와 대상 찾기 코드(`FindTarget`의 '후보 가운데 락온 우선')는 고치지 않아, 처형은 그대로입니다. 대상이 없으면 추적 구간이 락온 대상 쪽으로 돌기만 하므로, 제자리 찌르기의 방향도 따로 처리하지 않습니다. |
| `UAgTargetingFilterTask_Visible` · `AgTargeting::IsVisible` | `Socket`을 생략하면 가림을 보지 않던 것을, 출처 액터 위치(캡슐 중심)에서 대상 액터 위치까지 `WorldStatic` 가림을 보도록 바꿉니다. AI 출처의 지각 검사는 그대로입니다. | 생략한 프리셋은 지금 없습니다. 명세의 '지형에 가려짐'(캡슐 중심 사이, 캐릭터는 가리지 않음)과 같고, 경비 사수의 `Muzzle` 가림과 같은 채널입니다. |
| `FAgTransitionParams` · `UAgAbilityTask_Transition` | `RequiredTags`(선택)를 더합니다. 시작할 때 주인에게 이 태그가 모두 없으면 옮기지 않고 끝납니다. | 이름과 뜻은 `StartSections`의 `RequiredTags`와 같습니다. 공중에서 찌르기를 마치면 끝 섹션으로 넘어가 어빌리티가 끝나는 데 씁니다('공중에 나타났을 때'). Task가 어빌리티를 끝내지 않는 원칙대로 끝은 몽타주 완료로 납니다. |
| `AAgCharacterBase` | `State.Hidden`이 붙으면 `SetActorHiddenInGame(true)`, 떨어지면 `false`. `State.PassThrough`처럼 태그 이벤트로 처리합니다. | 시간에 따른 캐릭터 상태는 효과가 주는 태그로 둡니다. 무기는 캐릭터의 컴포넌트라 함께 숨습니다. |
| `AgGameplayTags` | `State.Hidden`, `GameplayCue.Teleport`, `GameplayCue.BlinkRushThrust`를 네이티브로 더합니다. | |
| `DesignData/Effects.json` | `Hidden`: `Infinite`, `State.Hidden`. | 구간에 두는 효과는 `Infinite`입니다. |
| `DesignData/TargetingPresets.json` | `Teleport`: `Sphere` → 락온 대상 후보(`AgTargetingSelectionTask_LockOnTarget`) → `Hostile` → 생존(`State.Dead` 제외) → `LockOnTarget` → `Visible`(소켓 없음) → 거리순. | 프리셋은 쓰임으로 이름 짓습니다(`Shooting`, `LockOn`). 필터끼리는 순서와 관계없이 모두 통과해야 남습니다. |
| `DesignData/Abilities.json` `Skill1` | 아래 2절. | 다른 스킬 항목은 고치지 않습니다. |
| `AM_Player_Skill1` (신규 몽타주) | `Default` 섹션: `Attack09` 0 ~ 0.2초(1배) → 0.62 ~ 0.7초(0.7배, 멈칫) → 0.7 ~ 0.95초(1.3배, 찌르기) → 0.95 ~ 1.5초(1.5배, 파고듦) → 1.5 ~ 2.1초(1.3배, 일어섬). 노티파이: 0.12초 엔진 `AnimNotify_GameplayCue`(`GameplayCue.Teleport`, 사라지는 효과), 0.36초부터 0.16초 `AnimNotify_GameplayCueState`(`GameplayCue.WeaponTrail`, 칼 궤적), 찌르기 순간 `AnimNotify_GameplayCue`(`GameplayCue.BlinkRushThrust`). `AirEnd` 섹션(다음 섹션 없음, `Default`도 이리 잇지 않음): `Attack09` 0.95초 자세(찌른 자세)에 머무는 조각, 블렌드 아웃 시간 길이. 루트 모션 설정은 지금 플레이어 공격 몽타주와 같게 합니다. | 연출(사라지는 효과, 섬광)은 모션 시각에 맞춰 몽타주 노티파이로 둡니다. 섹션 끝 머무는 조각은 '흐름'의 기존 방식입니다. 한 모션 안의 이어진 조각이라 자세가 튀지 않습니다. |
| `GC_Teleport` (신규, 엔진 `UGameplayCueNotify_Burst`의 데이터 전용 BP) · `NS_Teleport` · `M_TeleportRift` · `M_TeleportInk` (신규) | 사라지고 나타나는 자리에서 `NS_Teleport`를 재생합니다(소리 없음). 배치는 `pelvis` 소켓·회전 고정이라, 메시를 넘기는 몽타주 노티파이(사라질 때)는 골반에서, 위치를 넘기는 Teleport(나타날 때)는 캡슐 중심에서 똑바로 섭니다. `NS_Teleport`는 이미터 다섯(빛의 틈 `Rift`, 틈을 가로지르는 가로 섬광 `Flash`, 수평으로 튀는 빛 파편 `Shards`, 먹 결 `Ink`, 몸 크기 원통에서 일어나는 빛가루 `Motes`)이고, 색은 사용자 변수 `User.Color` 하나입니다. 머티리얼 계산은 각각 Custom HLSL 노드 하나에 둡니다. | 효과 클래스는 엔진 클래스를 그대로 씁니다. 사용자 요청으로 전용 효과를 만들었습니다(다른 스킬 효과의 복제가 아닌 고유한 모양). |
| `GC_BlinkRushThrust` (신규, `UGameplayCueNotify_Burst`의 데이터 전용 BP) · `NS_BlinkRushThrust` (신규) | 찌르기 순간의 몽타주 노티파이로 칼끝 앞에서 `NS_BlinkRushThrust`를 재생합니다(소리 없음). 배치는 메시 `root` 소켓이라 캐릭터 정면 기준이고, 칼끝 위치는 이미터의 위치 오프셋입니다. 이미터는 찌른 방향의 빛줄기 `Line`, 칼끝의 여덟 갈래 별빛 섬광 `GlintH`·`GlintV`·`GlintD1`·`GlintD2`, 찌른 방향으로 원뿔처럼 튀는 빛 파편 `Rays`, 먹 결 `Ink`, 빛가루 `Motes`이며, 머티리얼은 `M_TeleportRift`·`M_TeleportInk`를 같이 씁니다. | `GC_Teleport`와 같은 구성입니다. |
| `EAgCameraSituation::BlinkRushHit` · `UAgGameplayCueNotify_Burst` · `DesignData/Camera.json` | 적중 큐의 어빌리티 태그가 `Ability.Action.Skill.1`이면 `BlinkRushHit` 반응을 재생합니다. `Camera.json` `BlinkRushHit`: 보통 셰이크(`CS_Medium`), 시야각 −6°, 0.04 / 0.08 / 0.25초. | 회피 반격·가드 반격 적중 연출과 같은 경로입니다. |
| `Scripts/ScenarioTest` | 키 표 등 공용 도구는 고치지 않습니다. 검증 시나리오는 4절. | |

### 순간이동 규칙 (`UAgAbilityTask_Teleport::Activate`)

1. 대상: 행동 대상입니다. 락온 대상이거나 어빌리티 `Range`(15m) 안의 적이라는 조건은 대상 프리셋이 맡습니다. 행동 대상이 없으면 순간이동하지 않습니다.
2. 높이: 플레이어 캡슐 아래 끝을 대상 캡슐 아래 끝 높이에 맞춥니다. 대상의 공중 여부는 `State.Airborne`으로 봅니다.
3. 후보: 대상 캡슐 중심에서 수평 거리 `Distance`, 대상 캐릭터 정면의 반대 방향부터 15°씩 좌우를 번갈아 180°까지(24곳). 순서대로 보고 처음 맞는 곳에 나타납니다.
4. 나타날 수 있는 자리:
   - 그 자리의 캡슐이 막히는 것과 겹치지 않음: 엔진 `UWorld::EncroachingBlockingGeometry`(지형·다른 캐릭터·보이지 않는 벽).
   - 대상 캡슐 중심에서 그 자리 캡슐 중심까지 `WorldStatic`이 가리지 않음.
   - 캡슐 중심에서 아래로 내린 선(캐릭터 무브먼트와 같은 채널, `IsWalkable`)이 닿는 바닥의 높이가 대상 아래 바닥과 엔진 `MaxStepHeight`(45cm) 안. 대상이 땅에 서 있으면 대상 발 높이가 곧 대상 아래 바닥입니다. 캡슐로 쓸어 내리면 중심이 단 밖이어도 가장자리에 걸쳐 바닥으로 잡혀 선으로 봅니다(검증에서 발견).
5. 이동: 엔진 `TeleportTo`(검사 생략, 위에서 이미 검사)로 옮기고 대상 쪽을 봅니다. 땅이면 찾은 바닥에 발을 맞춥니다.
6. 공중이면: 이동 모드를 `Falling`으로, 속도를 0으로, 중력 배율을 0으로 둡니다. 구간이 끝나면(`OnDestroy`, 취소 포함) 중력 배율을 되돌립니다. 낙하 상태를 유지해 `State.Airborne`·공중 피격 반응·낙하 높이 측정이 그대로 공중을 따르고, 낙하 중 몽타주 루트 모션은 수평 속도만 바꾸므로 높이가 유지됩니다.
7. 나타나는 자리(옮겼든 아니든 지금 위치)에서 `GameplayCue.Teleport`를 실행합니다. 같은 프레임의 몽타주 노티파이는 타임라인 판정보다 먼저 울려 옛 자리에서 재생되므로, 나타나는 효과는 자리를 정하는 이 Task가 냅니다.

### 같은 시각과 순서

- `Teleport`의 시작(0.2초)과 `Hidden`·`Invincible`·`DisableRootMotion`의 끝(0.2초)은 같은 판정 안에서 순서와 관계없이 결과가 같습니다(그리기·피격·이동은 판정 뒤).
- 추적 구간은 시작할 때 지금 위치로 워프 목표를 정하므로, 순간이동 뒤의 RootMotion은 `Teleport`보다 늦게 시작합니다(0.21초). 같은 판정 안에서는 시각 순서로 실행됩니다. 이 규칙을 '형식'에 더합니다.
- `AirEnd`로 가는 Transition은 무기 판정 구간의 끝(0.506초)보다 뒤(0.51초)에 둡니다. 전환과 같은 시각에 끝나는 판정 구간은 끝까지 스윕하지 못합니다.

### 영향 없는 곳

- 스킬 2 ~ 4, 다른 어빌리티의 대상 찾기(새 필터는 `Teleport` 프리셋에만 넣음), `Shooting` 프리셋(소켓 있음).
- 입력 계층, HUD(`UAgSkillSlot`이 정의의 코스트와 쿨다운을 그대로 그림), 적중 보상(`Ability.Action.Skill` UP +15).
- 카메라 위치 추적. 자유 시점의 위치 추적이 순간이동한 자리로 따라갑니다(명세대로 바꾸지 않음).

## 2. GAS 및 데이터 설계

### 태그 (`AgGameplayTags`)

```cpp
UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Hidden);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Teleport);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_BlinkRushThrust);
```

### `DesignData/Effects.json`

```json
"Hidden": {
  "DurationPolicy": "Infinite",
  "GrantedTags": ["State.Hidden"]
}
```

### `DesignData/TargetingPresets.json`

```json
"Teleport": [
  {"Class": "/Script/Ag.AgTargetingSelectionTask_Sphere"},
  {"Class": "/Script/Ag.AgTargetingSelectionTask_LockOnTarget"},
  {"Class": "/Script/Ag.AgTargetingFilterTask_Hostile"},
  {"Class": "/Script/Ag.AgTargetingFilterTask_TagRequirements", "Requirements": {"IgnoreTags": ["State.Dead"]}},
  {"Class": "/Script/Ag.AgTargetingFilterTask_LockOnTarget"},
  {"Class": "/Script/Ag.AgTargetingFilterTask_Visible"},
  {"Class": "/Script/TargetingSystem.TargetingFilterTask_SortByDistance"}
]
```

### `DesignData/Abilities.json` `Skill1`

몽타주 시각: 0 ~ 0.2초 `Attack09` 0 ~ 0.2초(몸을 낮춤), 0.2 ~ 0.314초 0.62 ~ 0.7초를 0.7배(멈칫), 0.314 ~ 0.507초 0.7 ~ 0.95초를 1.3배(찌르기), 0.507 ~ 0.873초 0.95 ~ 1.5초를 1.5배(파고듦), 0.873 ~ 1.335초 1.5 ~ 2.1초를 1.3배(일어섬). 찌르기 판정은 `Attack09` 0.8 ~ 0.95초인 0.391 ~ 0.506초입니다. 누른 뒤 찌르기까지 0.39초라 넉업 체공(약 0.57초) 안에 따라가 찌릅니다.

```json
"Skill1": {
  "AbilityTags": ["Ability.Action.Skill.1"],
  "InputTag": "Input.Skill1",
  "Icon": "/Game/UI/Icons/T_Skill_BlinkRush.T_Skill_BlinkRush",
  "ActivationBlockedTags": ["State.Busy", "State.HitReaction", "State.Groggy", "State.Execution", "State.Dead"],
  "ActivationOwnedTags": ["State.Acting"],
  "CancelAbilitiesWithTag": ["Ability.Action", "Ability.Sprint"],
  "Cost": {"MP": 20},
  "Cooldown": 6,
  "Targeting": {"Preset": "Teleport", "Range": 1500, "TargetSpacing": 100},
  "Montage": {
    "Asset": "/Game/Animations/AM_Player_Skill1.AM_Player_Skill1",
    "Sections": {
      "Default": [
        {"Task": "RootMotion", "Start": 0, "End": 0.12, "bWarpTranslation": false},
        {"Task": "ApplyEffect", "Start": 0, "End": 0.67, "Effect": "Busy"},
        {"Task": "ApplyEffect", "Start": 0, "End": 0.2, "Effect": "Invincible"},
        {"Task": "ApplyEffect", "Start": 0.12, "End": 0.2, "Effect": "Hidden"},
        {"Task": "DisableRootMotion", "Start": 0.12, "End": 0.2},
        {"Task": "Teleport", "Start": 0.2, "End": 0.506, "Distance": 300},
        {"Task": "RootMotion", "Start": 0.21, "End": 0.38, "TargetingRange": 500, "bWarpTranslation": false},
        {"Task": "WeaponHit", "Start": 0.391, "End": 0.506, "Damage": {"HPMultiplier": 1.5, "PPMultiplier": 1.5, "HitReaction": "Flinch", "bGuardable": true}},
        {"Task": "Transition", "Start": 0.51, "RequiredTags": ["State.Airborne"], "ToSection": "AirEnd"}
      ]
    }
  }
}
```

| 항목 | 명세와의 대응 |
| - | - |
| `Targeting` | '대상'. `Range` 15m가 락온하지 않았을 때의 타겟팅 범위이고, 락온 대상은 프리셋이 거리와 관계없이 후보에 넣습니다. 첫 RootMotion은 `TargetingRange`를 생략해 이 15m를 따릅니다. |
| 첫 `RootMotion` (0 ~ 0.12초) | 사라지기 전 웅크리는 동안 대상 쪽으로 돌기만 합니다. 15m 밖의 락온 대상에게도 돕니다(구간 밖이면 락온 대상 쪽으로 돌기). |
| `Invincible` (0 ~ 0.2초) | '무적: 시작부터 나타날 때까지' |
| `Hidden`·`DisableRootMotion` (0.12 ~ 0.2초) | '보이지 않음 0.08초'. 보이지 않는 동안 모션의 이동을 끄므로, 순간이동하지 않으면 사라진 자리에 나타납니다. |
| `Teleport` (0.2 ~ 0.506초) | '나타나는 자리'와 '공중에 나타났을 때'의 머무는 동안. `Distance` 3m = '대상과의 간격'. |
| 둘째 `RootMotion` (0.21 ~ 0.38초) | 나타난 뒤 '공격 중 위치·회전 보정'. 이동은 모션의 돌진 그대로 두고(`bWarpTranslation` 끔) 대상 쪽으로 돌기만 하며, 찌르기 직전에 방향을 확정합니다. |
| `WeaponHit` | HP 1.5, PP 1.5, 움찔 |
| `Busy` 끝 0.67초 | 후딜 시작: 파고든 `Attack09`가 두 발을 바닥에 붙이고 멈춘 1.2초 |
| `Transition` 0.51초 | 공중이면 `AirEnd`로 넘어가 블렌드 아웃하며 몽타주와 어빌리티가 끝나고, 엔진 낙하 자세로 떨어집니다. 이때 `Busy` 구간도 닫힙니다. 땅이면 아무 일도 하지 않습니다. |

## 3. 인터페이스 & 함수 시그니처

```cpp
// AgAbilityTask_Teleport.h
USTRUCT()
struct FAgTeleportParams : public FAgTaskParams
{
	GENERATED_BODY()

	/** 나타날 때 대상과의 캡슐 중심 사이 수평 거리입니다. */
	UPROPERTY()
	float Distance = 0.f;

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

UCLASS()
class UAgAbilityTask_Teleport : public UAbilityTask
{
	GENERATED_BODY()
	friend struct FAgTeleportParams;

public:
	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	UPROPERTY()
	FAgTeleportParams Params;

	/** 공중에 나타나 높이를 유지하는 동안 끝날 때 되돌릴 중력 배율입니다. */
	TOptional<float> SavedGravityScale;
};

// AgTargeting.h
UCLASS()
class UAgTargetingFilterTask_LockOnTarget : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

protected:
	/** 출처에게 락온 대상이 있으면 그 대상이 아닌 후보를 거릅니다. */
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;
};

// AgAbilityTask_Transition.h (FAgTransitionParams)
/** 시작할 때 주인에게 이 태그가 모두 있어야 옮깁니다. 없으면 옮기지 않고 끝납니다. */
UPROPERTY()
FGameplayTagContainer RequiredTags;

// AgCharacterBase.h
void HandleHiddenChanged(const FGameplayTag Tag, int32 NewCount);
```

`AgAbilityTask_Teleport.cpp`의 익명 네임스페이스(파일 안에서만 씀):

```cpp
/** 대상 둘레에서 등 뒤에 가장 가까운, 나타날 수 있는 캡슐 중심 위치입니다. */
TOptional<FVector> FindAppearLocation(const ACharacter& Character, const ACharacter& Target, float Distance);

/** Location 아래 MaxDepth 안의 WorldStatic 바닥 높이입니다. */
TOptional<double> FindFloorZ(const UWorld& World, const FVector& Location, double MaxDepth, const FCollisionQueryParams& QueryParams);
```

## 4. 검증 시나리오 (4-test)

`Scripts/ScenarioTest`의 테스트 창으로 명세의 완료 조건을 확인하고, 이번에 만든 시나리오 중 회귀에 쓸 것만 `Saved/ScenarioTest/Scenarios`에 남깁니다.

| 시나리오 | 확인 |
| - | - |
| 기본 | 15m 안 샌드백 하나: 0.12초에 숨고 0.2초에 등 뒤 3m(±10cm)에 대상 쪽을 보고 나타나 바로 뒤까지 파고듦, 적중 대미지(백어택 포함)·PP·움찔, MP −20·UP +15, 쿨다운 6초 동안 1 키 무시, MP 20 미만이면 발동 안 함 |
| 대상 고르기 | 여럿 중 가장 가까운 적, 락온 대상 25m(더 가까운 적 있음), 가장 가까운 적이 벽 뒤(보이는 적에게 감), 락온 대상이 벽 뒤(제자리, 락온 쪽으로 찌름), 15m 안에 적 없음(제자리, 정면) |
| 자리 | 대상 바로 뒤가 벽·낭떠러지(옆 자리), 높은 단 위의 적(단 위에 섬) |
| 공중 | 스킬 2로 띄운 적: 같은 발 높이·등 뒤에 나타나 0.506초까지 높이 유지, 적중 시 저글·에어리얼 보너스, 0.51초 뒤 어빌리티 끝·낙하, 낙하 중 기본 공격 키 → 점프 공격 |
| 무적 | 적 공격 판정이 0 ~ 0.2초에 닿으면 피해 없음, 나타난 뒤 닿으면 피격 |
| 이어지기 | 후딜에 기본 공격·스킬 2로 캔슬 |
| 연출 | 사라지고 나타나는 효과, 찌르기 섬광, 적중 카메라(테스트 창 캡처로 확인) |
| 회귀 | 경비 사수 사격(`Shooting` 시야 필터), 처형 대상 고르기, 다른 스킬·기본 공격 |

## 5. 문서 반영 (4단계)

| 문서 | 반영 |
| - | - |
| [몽타주 타임라인과 구간 판정](<../../Docs/Architecture/몽타주 타임라인과 구간 판정.md>) '흐름'·'형식' | Transition의 `RequiredTags`. 순간이동 뒤의 추적 구간은 Teleport보다 늦게 시작한다는 규칙. |
| [데이터 로딩과 스키마](<../../Docs/Architecture/데이터 로딩과 스키마.md>) '대상의 시야 필터'·클래스 구조 | `Teleport` 프리셋과 `UAgTargetingFilterTask_LockOnTarget`. `Visible`의 `Socket`을 생략하면 캡슐 중심에서 가림을 본다는 설명. |
| [캐릭터 기반 구조와 GAS](<../../Docs/Architecture/캐릭터 기반 구조와 GAS.md>) '캐릭터'·'GameplayEffect' | `AAgCharacterBase`의 `State.Hidden` 처리. `GameplayCue.Teleport`(Teleport Task가 실행), `GameplayCue.BlinkRushThrust`(몽타주 노티파이). |
| 기획서·아트 문서 | 1-spec의 '기존 기획과의 관계' 표를 따릅니다. `GC_Teleport`의 에셋은 아트 표현 사양에 적습니다. |
