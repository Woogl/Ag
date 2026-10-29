# 움찔 부위별 반응 기술 설계 (Plan)

## 1. 영향 범위 및 아키텍처

| 대상 | 변경 |
| - | - |
| `UAgDamageResultGameplayEffectComponent` | 움찔(`Event.Hit.Stagger`)을 보낼 때 맞은 곳을 골라 그 하위 이벤트를 보냅니다. 강인도에 따른 반응 선택(이번 작업 전에 반영)은 그대로입니다. |
| `UAgGameplayAbility` | 발동시킨 이벤트 태그를 기억합니다. |
| `FAgPlayMontageParams` | 발동 이벤트로 재생 여부를 고르는 조건 `EventTag`를 더합니다. |
| `AgGameplayTags` | `Event.Hit.Stagger` 아래 맞은 곳 태그 5개를 네이티브로 선언합니다. |
| `Abilities.json` `Flinch` | 섹션별 PlayMontage 6개(공중 1, 지상 5)로 바꿉니다. |
| `Characters.json` | 린과 커스터의 `Flinch`·`KnockDown`을 공통 몽타주로 바꾸고, 커스터에 `Flinch` 어빌리티를 더합니다. |
| 에셋 | 쌍검 `Hit2` 좌우 반전 시퀀스, 공통 `AM_Flinch`·`AM_FlinchAir`·`AM_KnockDown`을 만들고 캐릭터별 움찔·넉다운 몽타주를 지웁니다. `AM_KnockUp`의 기상 끝부분을 쌍검 모션으로 바꿉니다. |

[캐릭터 기반 구조와 GAS](<../../Docs/Architecture/캐릭터 기반 구조와 GAS.md>)의 어빌리티 구조를 따릅니다.
- 어빌리티와 재생 Task는 맞은 곳을 모르고, 어떤 섹션을 고를지는 데이터에 적은 조건(`EventTag`)으로만 정합니다. 회피가 `MoveDirection`으로 섹션을 고르는 것과 같은 방식입니다.
- `EventTag`는 이벤트로 발동하는 어떤 어빌리티든 쓸 수 있는 범용 조건이라, 새 Task나 어빌리티 ID 분기를 만들지 않습니다.
- 맞은 곳을 정하는 규칙은 반응 선택의 주인인 피격 결과 처리 컴포넌트가 맡습니다.

## 2. GAS 및 데이터 설계

### 2.1 맞은 곳 판정 (피격 결과 처리)

`ResolveHitReactionEvent`가 `Event.Hit.Stagger`를 고른 경우에만, 아래 순서로 하위 태그를 정해 그 태그로 보냅니다. 저글(`Event.Hit.Flinch`)과 넉업·넉다운 이벤트는 바꾸지 않습니다.

1. **등**: 피격 방향(공격자 또는 투사체 위치 → 피격자, 수평)의 반대, 즉 피격자에서 공격자 쪽 방향이 피격자 등 방향과 45° 이내면 `Event.Hit.Stagger.Back`.
2. **맞은 뼈**: 이펙트 컨텍스트의 HitResult `BoneName`(WeaponHit만 넣음)에서 피격자 메시의 부모 뼈를 따라 올라가, 처음 만나는 기준 뼈로 정합니다.

   | 기준 뼈 | 태그 |
   | - | - |
   | `neck_01`, `spine_04` | `Event.Hit.Stagger.Head` (머리·가슴) |
   | `clavicle_r` | `Event.Hit.Stagger.ArmRight` |
   | `clavicle_l` | `Event.Hit.Stagger.ArmLeft` |
   | `pelvis` | `Event.Hit.Stagger.Body` (배·골반·다리) |

3. **뼈가 없거나 기준 뼈를 못 찾음**(투사체, 범위 공격): `Event.Hit.Stagger.Body`.

- 모든 사람형 캐릭터가 `SK_Mannequin` 스켈레톤을 같이 쓰므로, 기준 뼈 이름은 컴포넌트의 익명 네임스페이스 상수로 둡니다. 스켈레톤이 다른 캐릭터가 생기면 그때 캐릭터 기본값으로 옮깁니다.
- 공중 여부는 여기서 보지 않습니다. 공중이어도 위 태그를 그대로 보내고, 몽타주가 `State.Airborne`으로 공중 섹션을 고릅니다.

### 2.2 발동 이벤트 조건

- `UAgGameplayAbility`는 `ActivateAbility`에서 `IsEventTrigger(TriggerEventData)`일 때 `TriggerEventData->EventTag`를 `TriggerEventTag`로 기억하고, 게터를 둡니다. 이벤트로 발동하지 않았으면 비웁니다.
- `FAgPlayMontageParams`에 `FGameplayTag EventTag`를 더합니다. 값이 있으면 어빌리티의 `TriggerEventTag`가 이 태그와 맞을(`MatchesTag`) 때만 재생하고, 아니면 기존 `RequiredTags`·`MoveDirection`처럼 Task를 끝냅니다.
- 엔진 `HandleGameplayEvent`는 이벤트 태그의 부모를 따라 올라가며 트리거를 찾습니다. 그래서 `Flinch`의 트리거는 지금처럼 `Event.Hit.Stagger` 하나로 두면 하위 태그 이벤트로도 발동합니다. `IsEventTrigger`도 `MatchesTag`라 하위 태그를 받아들입니다.

### 2.3 태그 (`AgGameplayTags`)

`Event.Hit.Stagger.Back`, `.Head`, `.ArmRight`, `.ArmLeft`, `.Body`를 네이티브로 선언합니다. 상위 `Event.Hit.Stagger`는 그대로 둡니다.

### 2.4 `Abilities.json`

```json
"Tasks": [
  {"Task": "PlayMontage", "Montage": "FlinchAir", "RequiredTags": ["State.Airborne"]},
  {"Task": "PlayMontage", "Montage": "Flinch", "Section": "Back", "BlockedTags": ["State.Airborne"], "EventTag": "Event.Hit.Stagger.Back"},
  {"Task": "PlayMontage", "Montage": "Flinch", "Section": "Head", "BlockedTags": ["State.Airborne"], "EventTag": "Event.Hit.Stagger.Head"},
  {"Task": "PlayMontage", "Montage": "Flinch", "Section": "ArmRight", "BlockedTags": ["State.Airborne"], "EventTag": "Event.Hit.Stagger.ArmRight"},
  {"Task": "PlayMontage", "Montage": "Flinch", "Section": "ArmLeft", "BlockedTags": ["State.Airborne"], "EventTag": "Event.Hit.Stagger.ArmLeft"},
  {"Task": "PlayMontage", "Montage": "Flinch", "Section": "Body", "BlockedTags": ["State.Airborne"], "EventTag": "Event.Hit.Stagger.Body"}
]
```

- 발동 조건·취소 태그·`bAllowRetrigger`는 그대로입니다. 다시 맞으면 어빌리티가 다시 발동하며 새 태그의 섹션을 처음부터 재생합니다.
- 몸을 돌리는 RootMotion Task는 넣지 않습니다. 이동은 몽타주 루트 모션만 씁니다.

### 2.5 `Characters.json`

| 캐릭터 | 변경 |
| - | - |
| 린 | `Montages`의 `Flinch` → `AM_Flinch`, `FlinchAir` → `AM_FlinchAir` 추가, `KnockDown` → `AM_KnockDown` |
| 커스터 | `Abilities`에 `Flinch` 추가. `Montages`에 `Flinch` → `AM_Flinch`, `FlinchAir` → `AM_FlinchAir` 추가, `KnockDown` → `AM_KnockDown` |
| `MontageTimelines.json` | 두 넉다운 몽타주의 무적 구간(0.1 ~ 0.7초)을 `AM_KnockDown` 키 하나로 합칩니다. |

### 2.6 에셋

| 에셋 | 구성 |
| - | - |
| 쌍검 `Hit2` 좌우 반전 시퀀스(`A_Flinch_ArmLeft`) | `/Game/Animations/` 아래에 원본을 미러 데이터 테이블로 반전해 구운 새 시퀀스를 둡니다. 5.8에서 굽는 방법(애니메이션 편집기의 미러 베이크 또는 에디터 파이썬으로 프레임마다 반전 포즈 기록)을 구현 첫 단계에서 확인합니다. 무기 소켓(`hand_r`)은 반전하지 않으므로 무기는 오른손에 남습니다. |
| `AM_Flinch` | `DefaultSlot`. 섹션 `Back`(`Hit5`), `Head`(`Hit1`), `ArmRight`(`Hit2`), `ArmLeft`(반전 `Hit2`), `Body`(`Hit4`). 섹션끼리 잇지 않습니다. 다섯 시퀀스 모두 루트 모션을 켭니다. |
| `AM_FlinchAir` | `DefaultSlot`, 쌍검 `Hit_Airborne`, 루트 모션 끔. 캐릭터의 몽타주 ID `FlinchAir`. |
| `AM_KnockDown` | `DefaultSlot`, 쌍검 `Hit_knockdown`, 루트 모션 켬. |
| `AM_KnockUp` | `GetUp` 조각을 사무라이 `Getup2_Idle1`의 0 ~ 0.47초로 줄이고, 그 뒤에 섹션 `Stand`(`A_KnockUp_Stand`의 1.0833초 ~ 끝)를 더합니다. 몽타주 타임라인의 `GetUp` 0.42초에 `Stand`로 0.15초 블렌드하는 Transition을 둡니다. 두 자세의 평균 차이는 13cm입니다. |
| `A_KnockUp_Stand` | 쌍검 `Hit_knockdown`의 복제본으로 루트 모션을 끄고 루트를 고정합니다. 원본은 넉다운에서 루트 모션을 켜고 쓰는데, 한 몽타주에 루트 모션 시퀀스가 섞이면 넉업 전체가 루트 모션 몽타주가 되어 Launch의 이동을 덮어쓰기 때문입니다. |
| `AM_Flinch` 재생 배율 | 1.4849(0.8167 / 0.55). 움찔 경직 0.55초. |
| 삭제 | `AM_Player_Flinch`, `AM_Player_KnockDown`, `AM_Boss_KnockDown`. 지우기 전에 참조(JSON, 몽타주 타임라인 키, 다른 에셋)를 확인합니다. |

- 공중 움찔을 `AM_Flinch`의 섹션이 아닌 별도 몽타주로 두는 이유: 엔진은 몽타주 안에 루트 모션 시퀀스가 하나라도 있으면 몽타주 전체를 루트 모션으로 다룹니다. 캐릭터 무브먼트는 낙하 중에도 그 수평 속도를 쓰므로, 같은 몽타주의 공중 섹션은 0.4초 동안 수평으로 멈춥니다. '한 어빌리티의 모션은 한 몽타주의 섹션'([데이터 로딩과 스키마](<../../Docs/Architecture/데이터 로딩과 스키마.md>))의 예외입니다.
- 좌우 반전 시퀀스 `A_Flinch_ArmLeft`는 엔진 미러링과 같은 계산(X축 반전, 좌우 뼈 짝 `_l`·`_r` 교환, 컴포넌트 공간 기준 자세 보정)을 에디터 파이썬으로 프레임마다 구워 만들었습니다. 미러 데이터 테이블 팩토리는 스켈레톤을 스크립트로 넘길 수 없어 쓰지 않았습니다.
- 시퀀스의 루트 모션 설정은 쌍검 팩 원본 시퀀스에 저장합니다. 지금 `Hit14`도 같은 방식입니다.

### 2.7 바꾸지 않는 것

- 적의 움찔 종료 후 패턴 후딜레이 생략은 이미 강인도와 무관하게 `State.HitReaction.Flinch`로 판단하므로 코드를 바꾸지 않습니다. StateTree에서 이 조건을 확인만 합니다.
- 넉업(`AM_KnockUp`), 저글, 가드 움찔, 피격 흔들림은 그대로입니다.

## 3. 인터페이스 & 함수 시그니처

```cpp
// AgGameplayAbility.h
const FGameplayTag& GetTriggerEventTag() const { return TriggerEventTag; }
FGameplayTag TriggerEventTag;   // private

// AgAbilityTask_PlayMontage.h (FAgPlayMontageParams)
UPROPERTY()
FGameplayTag EventTag;

// AgDamageResultGameplayEffectComponent.cpp (익명 네임스페이스)
FGameplayTag ResolveStaggerEvent(const AActor& Causer, const AAgCharacterBase& Target, const FGameplayEffectContextHandle& Context);
```

- `ResolveHitReactionEvent`가 `Event_Hit_Stagger`를 돌려준 경우, `StartHitReaction`이 `ResolveStaggerEvent`로 바꿔 보냅니다.

## 4. 문서 동기화 (4단계)

| 문서 | 반영 |
| - | - |
| [전투 시스템](<../../Docs/Design/System/전투 시스템.md>) | '피격 모션'의 움찔 행과 공중 움찔, 맞은 곳 규칙 |
| [적 시스템](<../../Docs/Design/System/적 시스템.md>) | '소형 적의 움찔' → '적의 움찔' |
| [플레이어 캐릭터 - 린](<../../Docs/Design/Content/플레이어 캐릭터 - 린.md>), [보스 캐릭터 - 커스터](<../../Docs/Design/Content/보스 캐릭터 - 커스터.md>) | 피격 모션 참조와 그림 |
| [아트 표현 사양](<../../Docs/Art/아트 표현 사양.md>) | '플레이어 움찔 모션' → 공통 피격 모션(움찔·공중 움찔·넉다운) |
| [ABP와 로코모션](<../../Docs/Art/ABP와 로코모션.md>), Samurai·Halberd·Dual Wield 조사의 선택 표 | 움찔·넉업·넉다운 선택 |
| [캐릭터 기반 구조와 GAS](<../../Docs/Architecture/캐릭터 기반 구조와 GAS.md>) | `Event.Hit.Stagger` 하위 태그와 맞은 곳 판정, `Flinch`의 섹션 선택 |
| [몽타주 타임라인과 구간 판정](<../../Docs/Architecture/몽타주 타임라인과 구간 판정.md>) 또는 [데이터 로딩과 스키마](<../../Docs/Architecture/데이터 로딩과 스키마.md>) | PlayMontage `EventTag` 조건 |
