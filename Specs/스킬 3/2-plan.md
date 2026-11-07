# 스킬 3 장풍 기술 설계 (Plan)

## 1. 영향 범위 및 아키텍처

| 구분 | 대상 | 변경 |
| - | - | - |
| 데이터 | `DesignData/Abilities.json` `Skill3` | 태그·비용·충전·대상 찾기·몽타주·타임라인을 스킬 3 것으로 바꿉니다. |
| 데이터 | `DesignData/Abilities.json` `Skill1`·`Skill2`·`Skill4` | `AbilityTags`만 `Ability.Action.Skill.1`·`.2`·`.4`로 바꿉니다. 다른 필드는 건드리지 않습니다. |
| 데이터 | `DesignData/Abilities.json` `HitReward` | `Rewards`의 스킬 줄(`Ability.Action.Skill`, UP 15)을 스킬마다 한 줄로 나눕니다. 1·2·4는 UP 15, 3은 UP 5입니다. 다른 줄은 그대로입니다. |
| 데이터 | `DesignData/SpawnActors.json` | 장풍 템플릿 `PlayerSwordWave`를 더합니다. |
| C++ | `FAgTransitionParams` (`AgAbilityTask_Transition`) | 선택 필드 `bCommitAbility`를 더합니다. 켜면 섹션을 옮기기 전에 어빌리티를 다시 커밋(비용·쿨다운 충전)하고, 커밋하지 못하면 옮기지 않고 계속 기다립니다. |
| C++ | `FAgSpawnActorParams` (`AgAbilityTask_SpawnActor`) | 선택 필드 `Roll`(도)을 더합니다. 생성 회전이 몸 정면 수평 방향에 이 기울기를 더한 값이 됩니다. |
| C++ | `AAgProjectile` | 예측 조준과 반사로 방향을 바꿀 때 생성 회전의 `Roll`을 유지합니다. |
| C++ | `AgGameplayTags` | `Ability.Action.Skill.1` ~ `Ability.Action.Skill.4`를 네이티브로 선언합니다. `Ability.Action.Skill`은 네 태그의 부모로 남깁니다. |
| C++ | `UAgGameplayAbility` | 남은 충전 수 조회 `GetCharges`를 더합니다. 쿨다운 효과를 세는 곳(`CheckCooldown`, `GetCooldownTimeRemainingAndDuration`)과 같은 쿼리를 씁니다. |
| C++ | `UAgSkillSlot` | 충전 숫자(`ChargeText`)를 그립니다. 최대 충전이 2회 이상일 때만 보이고, 충전이 최대보다 적은 동안 다음 틱 타이머로 갱신합니다. |
| 에셋 | `/Game/Animations/AM_Player_Skill3` (새로 만듦) | ACS2 `Combo01_1`·`Combo01_2`·`Combo01_3`을 섹션 `Attack1`·`Attack2`·`Attack3`에 하나씩 넣습니다. 섹션 링크는 없고, 슬롯·블렌드는 `AM_Player_Skill`과 같게, `RateScale` 1.2. 휘두르는 구간에 `GameplayCue.WeaponTrail` 상태 노티파이를 둡니다. |
| 에셋 | `/Game/Projectiles/BP_PlayerSwordWave` (새로 만듦) | `BP_SwordWave`를 복제합니다. 판정 상자·메시·머티리얼은 그대로 두고, 메시의 커스텀 프리미티브 데이터(1~4번 = 색 RGBA)로 색만 흰색 반투명으로 바꿉니다. 자식 BP는 상속한 컴포넌트를 파이썬으로 고칠 때 부모(보스 검기)의 템플릿을 잘못 건드릴 수 있어 쓰지 않습니다. |
| C++ | `UAgHUDWidget` | 어빌리티 부여와 데이터 다시 읽기 알림에서 쿨다운 표시(`UpdateCooldowns`)도 다시 그립니다. 충전 숫자가 최대 충전(정의 값)에 따라 보이므로, 첫 커밋 전에도 보이게 합니다. |
| 에셋 | `WBP_SkillSlot` | `ChargeText`(Small 외곽선 글자)를 슬롯 중심에서 오른쪽 29px, 위 29px에 둡니다. |

새 클래스와 새 Task는 없습니다. 어빌리티는 공통 `UAgGameplayAbility`와 기존 Task(RootMotion, ApplyEffect, SpawnActor, Transition)로 동작합니다. [캐릭터 기반 구조와 GAS](<../../Docs/Architecture/캐릭터 기반 구조와 GAS.md>)의 '어빌리티 구조'(어빌리티 하나 + 범용 Task, Task 종류나 어빌리티 ID로 분기하지 않음)를 따르고, 새 필드는 둘 이상의 어빌리티가 쓸 수 있는 일반 기능입니다.

### 기술 결정 1: 연속 발동은 기본 공격처럼 한 실행 안의 섹션 전환으로

| 안 | 내용 | 판단 |
| - | - | - |
| A (추천) | 기본 공격 콤보와 같은 방식입니다. 한 번 발동한 스킬 3이 `Attack1` → `Attack2` → `Attack3`을 Transition으로 잇습니다. 후딜에 3 키를 누르면 스킬 3이 이미 실행 중이라 발동하지 않고, 입력 계층이 `Input.Skill3` 이벤트를 보내며([캐릭터 기반 구조와 GAS](<../../Docs/Architecture/캐릭터 기반 구조와 GAS.md>) '입력'), 후딜 구간의 Transition이 받습니다. Transition의 `bCommitAbility`가 그때마다 MP 10과 충전 1회를 치르고, 치를 수 없으면 넘어가지 않습니다. | 연발 순서가 몽타주 섹션과 Transition으로 데이터에 그대로 드러나고, 입력 계층·선입력·대상 다시 찾기(BlendTime 재재생)는 고치지 않습니다. 다시 커밋하면 엔진이 커밋 알림(`AbilityCommittedCallbacks`)을 보내 HUD가 지금처럼 갱신됩니다. |
| B | 스킬 3을 재발동(`bAllowRetrigger`)하게 하고, 직전 실행이 어느 섹션의 후딜에서 끝났는지로 시작 섹션을 고릅니다. | 엔진은 재발동할 때 이전 실행을 먼저 끝낸 뒤 새로 발동하므로, 시작 섹션을 고를 때 이전 실행의 상태가 남지 않습니다. 이를 넘기는 상태나 태그가 새로 필요합니다. |

- 세 번째 동작의 후딜에서 다시 발동하는 것도 같은 방식입니다. `Attack3`의 Transition이 `Attack1`로 돌아갑니다.
- 다른 행동으로 모션 캔슬하거나, 피격으로 끊기거나, 동작이 끝까지 재생되면 실행이 끝납니다. 다음 3 키는 새 발동이라 `Attack1`부터 시작합니다.
- 커밋에 실패한 이벤트는 버리고 Transition은 구간 끝까지 계속 기다립니다. 후딜 동안 충전이 회복되면 그다음 입력으로 이어집니다.
- 선입력은 지금 그대로입니다. 후딜 전에 누른 3 키는 Busy가 끝난 다음 틱에 다시 실행되고, 같은 시각에 시작한 Transition이 그 이벤트를 받습니다(기본 공격과 같은 순서).

### 기술 결정 2: 스킬마다 어빌리티 태그를 두고 적중 보상 줄을 나눔

- 네 스킬의 어빌리티 태그를 `Ability.Action.Skill.1` ~ `Ability.Action.Skill.4`로 구분합니다(사용자 결정). 적중 보상의 스킬 줄을 스킬마다 한 줄로 나누면, 스킬 3만 UP 5로 둘 수 있습니다.
- 적중 보상 줄의 매칭(`HasTag`, 부모 태그 포함)과 '맞는 줄은 모두 충전'은 그대로입니다. 그래서 `Ability.Action.Skill` 한 줄을 남기면 네 스킬 모두에 맞으므로, 그 줄은 지우고 스킬마다 적습니다. HitReward 코드는 고치지 않습니다.
- 부모 태그로 묶는 규칙(`CancelAbilitiesWithTag`의 `Ability.Action` 등)에서는 네 스킬 모두 그대로 행동입니다. 지금 코드에는 `Ability.Action.Skill`을 직접 읽는 곳이 없습니다.
- 스킬 4종 확장 설계의 "스킬별 어빌리티 태그는 그 태그를 읽는 규칙이 생길 때 그 스킬의 작업에서 더한다"에 따라, 스킬별 충전량이 생기는 이 작업에서 넷을 함께 더합니다. 하나만 더하면 남은 스킬이 부모 태그 줄에 남아 스킬 3에도 맞기 때문입니다.

### 기술 결정 3: 장풍 기울기는 SpawnActor의 `Roll`

- 기울기는 같은 장풍을 어느 동작이 날리느냐에 따라 다르므로, 생성 액터 템플릿이 아니라 타임라인의 SpawnActor 항목에 둡니다. 템플릿은 `PlayerSwordWave` 하나입니다.
- `Roll`은 엔진 `FRotator`의 Roll(전진 축 기준, 뒤에서 볼 때 시계 방향이 +)입니다. 장풍의 길이 방향은 `BP_SwordWave`처럼 좌우이므로 +35°는 오른쪽 끝이 내려가 '왼쪽 위 ↔ 오른쪽 아래', -30°는 '왼쪽 아래 ↔ 오른쪽 위'가 됩니다. 부호는 구현 때 화면으로 확인합니다.
- 투사체는 생성을 마칠 때 예측 지점으로 방향을 바꾸며(`SetActorRotation(FlightDirection.Rotation())`), 이때 Roll이 0이 됩니다. 방향을 바꾸는 두 곳(예측 조준, 반사)에서 생성 회전의 Roll을 유지하도록 고칩니다. 보스 검기·탄환은 Roll 0이라 지금과 같습니다.
- 판정 모양은 액터의 자식 컴포넌트라 함께 기울어지므로, 보이는 모양과 판정이 같습니다.

## 2. GAS 및 데이터 설계

### 태그

- `Ability.Action.Skill.1` ~ `Ability.Action.Skill.4` — "스킬 1 ~ 4". 적중 보상 줄이 스킬마다 충전량을 고르는 데 씁니다. 코드는 직접 읽지 않고 `Abilities.json`이 이름으로 가리킵니다.

### `Abilities.json` `Skill3`

시각은 1배속 시퀀스 기준(몽타주 Rate Scale 적용 전)이며 초기값입니다. 장풍 시각은 팩의 공격 판정 시각, 후딜 시작과 Transition은 [ACS2 Combo Skill 애니메이션 조사](<../../Docs/Art/ACS2 Combo Skill 애니메이션 조사.md>) '이어 붙이기'의 연결 시점이고, 세 번째의 후딜은 `Combo01_3` → `Combo01_4` 연결 시점(0.67초)입니다. 몽타주를 만든 뒤 칼끝 궤적과 함께 다시 맞춥니다.

```json
"Skill3": {
  "AbilityTags": ["Ability.Action.Skill.3"],
  "InputTag": "Input.Skill3",
  "Icon": "/Game/UI/Icons/T_Skill_DashSlash.T_Skill_DashSlash",
  "ActivationBlockedTags": ["State.Busy", "State.HitReaction", "State.Groggy", "State.Execution", "State.Dead"],
  "ActivationOwnedTags": ["State.Acting"],
  "CancelAbilitiesWithTag": ["Ability.Action", "Ability.Sprint"],
  "Cost": {"MP": 10},
  "Cooldown": 3,
  "MaxCharges": 3,
  "Targeting": {"Preset": "Enemy", "Range": 2000, "TargetSpacing": 100},
  "Montage": {
    "Asset": "/Game/Animations/AM_Player_Skill3.AM_Player_Skill3",
    "Sections": {
      "Attack1": [
        {"Task": "RootMotion", "Start": 0, "End": 0.35, "bWarpTranslation": false},
        {"Task": "ApplyEffect", "Start": 0, "End": 0.5, "Effect": "Busy"},
        {"Task": "SpawnActor", "Start": 0.39, "Actor": "PlayerSwordWave", "Roll": 35},
        {"Task": "Transition", "Start": 0.5, "Event": "Input.Skill3", "ToSection": "Attack2", "BlendTime": 0.2, "bCommitAbility": true}
      ],
      "Attack2": [
        {"Task": "RootMotion", "Start": 0, "End": 0.13, "bWarpTranslation": false},
        {"Task": "ApplyEffect", "Start": 0, "End": 0.43, "Effect": "Busy"},
        {"Task": "SpawnActor", "Start": 0.17, "Actor": "PlayerSwordWave", "Roll": -30},
        {"Task": "Transition", "Start": 0.43, "Event": "Input.Skill3", "ToSection": "Attack3", "BlendTime": 0.2, "bCommitAbility": true}
      ],
      "Attack3": [
        {"Task": "RootMotion", "Start": 0, "End": 0.38, "bWarpTranslation": false},
        {"Task": "ApplyEffect", "Start": 0, "End": 0.67, "Effect": "Busy"},
        {"Task": "SpawnActor", "Start": 0.43, "Actor": "PlayerSwordWave", "Roll": 40},
        {"Task": "Transition", "Start": 0.67, "Event": "Input.Skill3", "ToSection": "Attack1", "BlendTime": 0.2, "bCommitAbility": true}
      ]
    }
  }
}
```

- `RootMotion`의 `bWarpTranslation: false`는 회전만 보정합니다(보스 SwordWave와 같음). 구간 끝이 '장풍을 날리기 직전에 방향 확정'입니다. 전진은 모션의 루트 이동이고, 최소 간격 1m는 `TargetSpacing`이 지킵니다.
- 대상 찾기 `Range` 2000은 장풍 사거리(20m)와 같습니다. 락온 대상은 공통 규칙대로 범위 밖이어도 고릅니다.
- 동작마다 다시 대상을 고르는 것은 BlendTime Transition의 다시 재생이 하는 일(`ReplaySection` → `RefreshTarget`)이라 따로 적지 않습니다.
- 첫 번째 발동의 비용·충전은 발동할 때(`ActivateAbility`의 `CommitAbility`), 이어지는 발동은 Transition이 치릅니다.

### `Abilities.json` `Skill1`·`Skill2`·`Skill4`

`AbilityTags`만 바꿉니다.

```json
"Skill1": { "AbilityTags": ["Ability.Action.Skill.1"], ... }
"Skill2": { "AbilityTags": ["Ability.Action.Skill.2"], ... }
"Skill4": { "AbilityTags": ["Ability.Action.Skill.4"], ... }
```

### `Abilities.json` `HitReward`

`Rewards`의 `{"AbilityTag": "Ability.Action.Skill", "UP": 15}`를 아래 네 줄로 바꿉니다.

```json
{"AbilityTag": "Ability.Action.Skill.1", "UP": 15},
{"AbilityTag": "Ability.Action.Skill.2", "UP": 15},
{"AbilityTag": "Ability.Action.Skill.3", "UP": 5},
{"AbilityTag": "Ability.Action.Skill.4", "UP": 15}
```

### `SpawnActors.json`

```json
"PlayerSwordWave": {
  "Class": "/Game/Projectiles/BP_PlayerSwordWave.BP_PlayerSwordWave_C",
  "Speed": 2500,
  "Range": 2000,
  "Damage": {
    "HPMultiplier": 0.5,
    "PPMultiplier": 0.5,
    "HitReaction": "Flinch",
    "bGuardable": true
  }
}
```

- `bDestroyOnHit`를 적지 않아(기본 false) 적을 뚫고 지나갑니다. 같은 대상은 한 번만 맞는 것과 벽·사거리 소멸은 지금 투사체 규칙입니다.
- 장풍마다 피해 스펙과 적중 상태를 따로 만드므로(`OnSpawnedByAbility`), 적중 보상은 장풍마다 첫 적중에 한 번입니다. 맞은 대상만 히트스톱이 걸리는 것(공격자가 투사체라 공격자 히트스톱 없음), 백어택의 기준이 투사체 위치인 것, 에어리얼 보너스도 지금 대미지 처리 그대로입니다.
- 생성 위치는 보스 검기와 같은 캐릭터 위치(캡슐 중심)입니다. 1-spec의 "플레이어 앞에서 발사"를 "캡슐 중심에서 발사"로 고칩니다.

## 3. 인터페이스 & 함수 시그니처

```cpp
// AgAbilityTask_Transition.h
USTRUCT()
struct FAgTransitionParams : public FAgTaskParams
{
	// ...
	/** 켜면 섹션을 옮기기 전에 어빌리티를 다시 커밋(비용·쿨다운)하고, 커밋하지 못하면 옮기지 않고 계속 기다립니다. */
	UPROPERTY()
	bool bCommitAbility = false;
};
```

- `UAgAbilityTask_Transition::TryTransition`의 처음에서 `bCommitAbility`이면 `Ability->K2_CommitAbility()`를 부르고, 실패하면 Task를 끝내지 않고 돌아갑니다. `TagRemoved`·시간으로 넘어가는 Transition에도 같은 뜻이지만, 지금 쓰는 곳은 `Event` 하나입니다.

```cpp
// AgAbilityTask_SpawnActor.h
USTRUCT()
struct FAgSpawnActorParams : public FAgTaskParams
{
	// ...
	/** 생성 회전의 Roll(도). 몸 정면 수평 방향을 축으로 뒤에서 볼 때 시계 방향이 +입니다. */
	UPROPERTY()
	float Roll = 0.f;
};
```

```cpp
// AgProjectile.cpp (파일 안 함수)
namespace
{
	/** 방향만 바꾸고 생성할 때 준 Roll은 유지합니다. */
	FRotator MakeFlightRotation(const FVector& Direction, float Roll);
}
```

```cpp
// AgGameplayAbility.h
/** 남은 충전 수. 걸린 쿨다운 효과 하나가 쓴 충전 하나입니다. */
int32 GetCharges(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const;
```

- 최대 충전 수는 HUD가 정의(`FAgAbilityData::MaxCharges`)에서 읽습니다.

```cpp
// AgSkillSlot.h
UPROPERTY(meta = (BindWidget))
TObjectPtr<UTextBlock> ChargeText;

int32 ShownCharges = INDEX_NONE;
```

- `UpdateCooldown`이 충전 숫자도 그립니다. 다음 틱 타이머는 지금 "쿨다운 중"에만 걸지만, "충전이 최대보다 적은 동안"으로 넓힙니다. 최대 충전이 1인 칸은 지금과 같습니다(충전이 최대보다 적은 동안 = 쿨다운 중).

## 4. 함께 고칠 구현 문서 (4단계)

| 문서 | 내용 |
| - | - |
| [몽타주 타임라인과 구간 판정](<../../Docs/Architecture/몽타주 타임라인과 구간 판정.md>) '흐름' | Transition의 `bCommitAbility`(이어지는 발동이 비용·충전을 치름)를 적습니다. |
| [데이터 로딩과 스키마](<../../Docs/Architecture/데이터 로딩과 스키마.md>) '생성 액터 정의' | SpawnActor 항목에 `Roll`을 더하고, 투사체가 방향을 바꿀 때 Roll을 유지한다고 적습니다. |
| [캐릭터 기반 구조와 GAS](<../../Docs/Architecture/캐릭터 기반 구조와 GAS.md>) '쿨다운'·'입력' | HUD가 남은 충전 수를 조회해 그린다고 더합니다. |

## 5. 다른 세션과 겹치는 곳

- 스킬 1·스킬 4 세션이 `AgGameplayTags.h/.cpp`와 `Abilities.json`을 고치고 있습니다. 태그는 선언 한 줄씩, `Abilities.json`은 `Skill3` 항목, `Skill1`·`Skill2`·`Skill4`의 `AbilityTags` 한 줄씩, `HitReward`의 스킬 줄만 고치고, 편집 직전에 다시 읽어 남의 변경을 덮지 않습니다.
- `Skill1`·`Skill4`의 `AbilityTags`는 그 세션들이 맡은 항목이라, 고친 뒤 두 세션에 바뀐 태그를 알려 항목을 통째로 다시 쓰다 되돌리지 않게 합니다.
- 스킬 1·4 명세가 '스킬 적중 UP +15'를 가리키므로, 4단계에서 [플레이어 캐릭터 - 린](<../../Docs/Design/Content/플레이어 캐릭터 - 린.md>) 'UP 충전' 표를 고칠 때 그 세션들의 반영과 맞춥니다.
