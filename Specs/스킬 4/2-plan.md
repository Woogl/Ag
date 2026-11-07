# 스킬 4 기술 설계 (Plan)

## 1. 영향 범위 및 아키텍처

| 구분 | 대상 | 변경 |
| - | - | - |
| 데이터 | `DesignData/Abilities.json` `Skill4` | 비용·쿨다운·몽타주·타임라인을 스킬 4 것으로 바꿉니다. 다른 항목은 건드리지 않습니다. |
| C++ | `FAgAreaHitParams` (`AgAbilityTask_AreaHit`) | 선택 필드 `Cue`를 더합니다. 비우지 않으면 범위 판정을 하는 순간 그 큐를 판정 반경과 함께 실행합니다. |
| C++ | `UAgGameplayCueNotify_Burst` | 선택 필드 `MagnitudeNiagaraParameter`를 더합니다. 비우지 않으면 큐의 `RawMagnitude`를 생성한 나이아가라의 그 실수 파라미터에 넣습니다. |
| C++ | `AgGameplayTags` | `GameplayCue.Skill4Slam`(스킬 4 내려찍기 VFX)을 네이티브로 선언합니다. |
| 에셋 | `/Game/Animations/AM_Player_Skill4` (새로 만듦) | ACS2 `Attack12`(`/Game/Fab/ACS2ComboSkill/Anim_Seq_UE5/Attack12`) 한 구간. 슬롯·블렌드는 `AM_Player_Skill`과 같게 두고 `RateScale` 1.2, 휩쓸기와 내려찍기에 `GameplayCue.WeaponTrail` 상태 노티파이를 둡니다. |
| 에셋 | `/Game/VFX/NS_Skill4Slam`, 전용 머티리얼 (새로 만듦) | Spec '내려찍기 VFX'의 섬광·충격파·참격 선·흙먼지와 파편. 사용자 파라미터 `Radius`(cm)까지 퍼집니다. 텍스처는 새로 만들지 않고 머티리얼 계산(`M_Telegraph`처럼)과 나이아가라 기본 에셋으로 그립니다. |
| 에셋 | `/Game/GameplayCues/GC_Skill4Slam` (새로 만듦) | 부모 `UAgGameplayCueNotify_Burst`. `BurstEffects`에 `NS_Skill4Slam`을 붙이지 않고 큐 위치에 재생, `MagnitudeNiagaraParameter` = `Radius`. |

새 클래스는 없습니다. 어빌리티는 공통 `UAgGameplayAbility`와 기존 Task(RootMotion, ApplyEffect, AreaHit)로 동작하고, 큐는 기존 C++ 부모를 씁니다. [기본 방침](<../../Docs/Architecture/기본 방침.md>)의 'C++ 로직, BP는 기본값만'과 [캐릭터 기반 구조와 GAS](<../../Docs/Architecture/캐릭터 기반 구조와 GAS.md>)의 '어빌리티 하나 + 범용 Task'를 따릅니다.

### 기술 결정: VFX를 AreaHit가 실행하는 이유

| 안 | 내용 | 판단 |
| - | - | - |
| A (추천) | AreaHit의 `Cue` 필드로 판정과 같은 순간에 큐를 실행하고, `RawMagnitude`에 `Area.Range`를 실어 보냅니다. | VFX가 퍼지는 거리의 원본이 판정 반경 하나(`Abilities.json`)뿐입니다. 판정과 VFX가 같은 프레임에 나옵니다. 대신 '연출 큐는 몽타주 노티파이에 둔다'는 지금 규칙에 예외가 하나 생기므로, [몽타주 타임라인과 구간 판정](<../../Docs/Architecture/몽타주 타임라인과 구간 판정.md>) '몽타주에 남는 노티파이'에 "판정 값이 필요한 연출은 그 판정 Task가 실행한다"를 적습니다. |
| B | 몽타주 `AnimNotify_GameplayCue`로 큐를 실행하고, 반경 4m를 `GC_Skill4Slam` 기본값에 따로 적습니다. | 지금 규칙 그대로지만 같은 값이 JSON과 에셋 두 곳에 생깁니다(AGENTS.md 2). 반경을 바꾸면 VFX가 판정과 어긋납니다. |

엔진 `UGameplayCueNotify_Burst`는 생성한 컴포넌트를 C++에 넘기지 않습니다(`OnBurst`가 BP 전용 이벤트). 그래서 `MagnitudeNiagaraParameter`를 쓸 때는 엔진의 실행 순서(생성 조건 → `BurstEffects.ExecuteEffects` → `OnBurst`)를 그대로 부르고 그 사이에 파라미터를 넣습니다. 비어 있으면 지금처럼 부모를 부릅니다.

## 2. GAS 및 데이터 설계

### 태그

- `GameplayCue.Skill4Slam` — "스킬 4 내려찍기 VFX". 코드는 이 태그를 직접 읽지 않고 `Abilities.json`이 이름으로 가리킵니다.

### `Abilities.json` `Skill4`

시각은 1배속 `Attack12` 기준(몽타주 Rate Scale 적용 전)이며, 몽타주를 만든 뒤 노티파이 위치와 함께 다시 맞춥니다.

```json
"Skill4": {
  "AbilityTags": ["Ability.Action.Skill"],
  "InputTag": "Input.Skill4",
  "Icon": "/Game/UI/Icons/T_Skill_DashSlash.T_Skill_DashSlash",
  "ActivationBlockedTags": ["State.Busy", "State.HitReaction", "State.Groggy", "State.Execution", "State.Dead"],
  "ActivationOwnedTags": ["State.Acting"],
  "CancelAbilitiesWithTag": ["Ability.Action", "Ability.Sprint"],
  "Cost": {"MP": 60},
  "Cooldown": 8,
  "Targeting": {"Preset": "Enemy", "Range": 5000, "TargetSpacing": 100},
  "Montage": {
    "Asset": "/Game/Animations/AM_Player_Skill4.AM_Player_Skill4",
    "Sections": {
      "Default": [
        {"Task": "RootMotion", "Start": 0, "End": 0.87, "TargetingRange": 500, "bWarpTranslation": true},
        {"Task": "ApplyEffect", "Start": 0, "End": 1.2, "Effect": "Busy"},
        {"Task": "AreaHit", "Start": 1.03, "Damage": {"HPMultiplier": 2.5, "PPMultiplier": 2.5, "HitReaction": "KnockDown", "bGuardable": true}, "Area": {"Preset": "Hit", "Range": 400}, "Cue": "GameplayCue.Skill4Slam"}
      ]
    }
  }
}
```

| 항목 | 값의 근거 |
| - | - |
| RootMotion 0 ~ 0.87초 | 추적·위치 보정 구간. 뛰어올라 정점을 지나 내려찍기 시작 직전(0.85초)까지 방향을 따라가고, 착지까지 앞으로 가는 3.0m 중 2.4m가 이 안에 있습니다. 나머지 0.6m는 내려찍는 동작의 이동이며, 대상과의 최소 간격(1m)이 막습니다. |
| `TargetingRange` 500 | 모션의 착지 거리 3.0m + 최소 간격 1m에 여유를 둔 값. 이보다 먼 적에게는 회전만 보정합니다. |
| Busy 0 ~ 1.2초 | 후딜 시작 = 무릎을 꿇은 자세가 자리 잡는 1.15초 직후. |
| AreaHit 1.03초 | 칼이 바닥에 닿는 시각(오른손 높이 1.00초 126cm → 1.05초 10cm, ACS2 원래 판정 1.03초). |
| `Area` | 프리셋 `Hit`(궁극기와 같음), 반경 400cm. 구의 중심은 캐릭터 캡슐 중심입니다. |
| `RateScale` 1.2 | ACS2가 플레이어 몽타주에 쓰던 배속. 내려찍기까지 0.86초, 후딜 시작 1.0초, 모션 끝 2.64초(실제 시간). 플레이 확인 후 조정합니다. |

### AreaHit의 큐 실행

- 판정 대상을 찾기 전에 큐를 실행합니다. 맞은 적이 없어도 재생하고, 첫 적중의 피해가 행동을 끝내도(사망 등) VFX는 이미 나옵니다.
- 큐 위치는 캐릭터 발밑(캡슐 중심에서 반높이만큼 아래), `Normal`은 캐릭터 정면의 수평 방향이라 VFX의 X축이 캐릭터 정면, Z축이 위입니다. `RawMagnitude`는 `Area.Range`, Instigator·EffectCauser는 캐릭터입니다.
- 큐는 자기 ASC의 `ExecuteGameplayCue`로 실행합니다. 대상은 플레이어이고, 기존 피격 큐처럼 피격자에게 보내지 않습니다.
- 나이아가라는 큐 위치에 붙이지 않고 생성하므로 캐릭터가 움직여도 그 자리에 남습니다. 월드 시간으로 진행하므로 히트스톱 중에도 퍼집니다.

### `NS_Skill4Slam`

| 이미터 | 만드는 방법 |
| - | - |
| 섬광 (`Flash`) | 칼끝이 닿는 앞쪽(로컬 X +110cm, 높이 25cm)에 지름 2.8m 스프라이트 하나를 0.14초 동안 띄우며 흐립니다(`M_Skill4Glow`). |
| 충격파 (`Shockwave`) | 위를 향해 눕힌 스프라이트 하나(`Particles.SpriteFacing` (0,0,1)). 크기는 SetParameters에서 `User.Radius × 2 × (1 − (1 − 나이×3)³)`로 수명 0.45초의 1/3(0.15초)에 판정 반경까지 커지고, 0.35 뒤로 흐려집니다. `M_Skill4Shockwave`가 바깥 가장자리 띠를 밝게, 안쪽을 옅게 그립니다. |
| 참격 선 (`Slashes`) | 발밑 40cm 고리에서 18개가 바깥으로 날아가는 바닥에 눕힌 속도 방향 스프라이트(14×150 → 22×170cm). 속도 `(User.Radius − 115) × 5`, 수명 0.2초라 앞끝이 판정 반경에서 사라집니다(`M_Skill4Glow`). |
| 흙먼지 (`Dust`) | 판정 반경 95% 원판 안 40개, 위·바깥으로 퍼지며 커지고 0.4~0.65초에 사라집니다(`M_Skill4Dust`, 회청색 반투명). |
| 파편 (`Debris`) | 판정 반경 90% 원판 안 26개 작은 조각이 튀어 올랐다 떨어집니다(`M_Skill4Dust`, 어두운 회색). |

- 모든 이미터는 로컬 공간이라 위치·속도가 큐의 방향(캐릭터 정면)을 따릅니다.
- 빛의 색은 사용자 파라미터 `User.Color` 하나(푸르스름한 흰빛)를 섬광·충격파·참격 선이 함께 씁니다.
- 반경이 바뀌어도 시간은 그대로이고 크기·속도만 `Radius`에 비례합니다.
- 만든 뒤 에디터 미리보기와 PIE 화면으로 모양을 확인하고, 판정 반경 끝과 충격파 가장자리가 맞는지 4m 지점의 샌드백으로 봅니다.

## 3. 인터페이스 & 함수 시그니처

```cpp
// AgAbilityTask_AreaHit.h
USTRUCT()
struct FAgAreaHitParams : public FAgTaskParams
{
	UPROPERTY() FAgDamage Damage;
	UPROPERTY() FAgTargetingRequest Area;
	/** 비우지 않으면 판정하는 순간 이 큐를 발밑에서 실행합니다. RawMagnitude는 판정 반경입니다. */
	UPROPERTY() FGameplayTag Cue;
};

// AgGameplayCueNotify_Burst.h
protected:
	/** 비우지 않으면 큐의 RawMagnitude를 생성한 나이아가라의 이 실수 파라미터에 넣습니다. */
	UPROPERTY(EditDefaultsOnly, Category = "GCN Effects")
	FName MagnitudeNiagaraParameter;
```

- `UAgAbilityTask_AreaHit::Activate`에 큐 실행만 더하고, 판정·피해 처리는 그대로입니다.
- `UAgGameplayCueNotify_Burst::OnExecute_Implementation`은 `MagnitudeNiagaraParameter`가 있을 때만 엔진 실행 순서를 직접 부릅니다. 전투 카메라 연출 요청은 지금처럼 끝에 합니다(이 큐 태그는 대상이 아니라 아무 일도 하지 않음).

## 4. 문서 반영 (4단계)

| 문서 | 반영 |
| - | - |
| 기획서 | Spec '기존 기획과의 관계' 표대로 플레이어 캐릭터 - 린, 아트 표현 사양, VFX·사운드 리소스 조사를 고칩니다. |
| [몽타주 타임라인과 구간 판정](<../../Docs/Architecture/몽타주 타임라인과 구간 판정.md>) | '몽타주에 남는 노티파이'에 판정 값이 필요한 연출(스킬 4 내려찍기 VFX)은 그 판정 Task의 `Cue`로 실행한다는 예외와 이유를 적습니다. |
| [데이터 로딩과 스키마](<../../Docs/Architecture/데이터 로딩과 스키마.md>) | 연출 값 표에 '스킬 4 내려찍기 VFX' 행(모양·시간은 `NS_Skill4Slam`·`GC_Skill4Slam`, 퍼지는 거리는 `Skill4` AreaHit `Area.Range`)을 더합니다. |
| [캐릭터 기반 구조와 GAS](<../../Docs/Architecture/캐릭터 기반 구조와 GAS.md>) | `UAgGameplayCueNotify_Burst` 설명에 `RawMagnitude`를 나이아가라 파라미터로 넘기는 기능을 더합니다. 클래스 관계는 바뀌지 않습니다. |
