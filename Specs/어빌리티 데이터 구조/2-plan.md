# 어빌리티 데이터 구조 기술 설계 (Plan)

[작업 명세](<1-spec.md>)를 구현하는 설계입니다. 공통 규칙은 [기본 방침](<../../Docs/Architecture/기본 방침.md>), [캐릭터 기반 구조와 GAS](<../../Docs/Architecture/캐릭터 기반 구조와 GAS.md>), [데이터 로딩과 스키마](<../../Docs/Architecture/데이터 로딩과 스키마.md>)를 따르며, 이 설계가 바꾸는 부분은 검증 뒤 그 문서들에 반영합니다(9절).

## 1. 기술 결정

| 항목 | 결정 | 이유 |
| - | - | - |
| 몽타주와 타임라인의 자리 | 어빌리티 항목의 `Montage`에 몽타주 에셋(`Asset`)과 섹션별 타임라인(`Sections`)을 둡니다. 어빌리티 하나는 몽타주 하나를 재생합니다. `Characters.json`의 `Montages`와 `MontageTimelines.json`은 없앱니다. | 어빌리티 하나가 항목 하나에서 다 보입니다. 지금 캐릭터마다 다른 에셋을 쓰는 몽타주 ID가 하나도 없어, 몽타주 ID를 거치는 단계가 하는 일이 없습니다. |
| 공유와 복제 | 같은 몽타주를 쓰는 어빌리티는 각자 타임라인을 적습니다. 캐릭터마다 다른 모션이 필요하면 어빌리티를 복제합니다. | 타임라인은 그 어빌리티가 몽타주의 어느 때에 무엇을 하는지입니다. 지금 겹치는 곳은 `AM_KnockUp`을 쓰는 KnockUp과 Juggle뿐입니다. |
| 시작 섹션 | `StartSections`가 없으면 첫 섹션부터 재생합니다. 있으면 조건이 맞는 항목의 섹션부터 재생하고, 맞는 항목이 없으면 재생하지 않습니다. 항목끼리는 조건이 서로 배타적이고, `Section`을 뺀 항목은 첫 섹션입니다. | 공격자 기준 입력 방향으로 섹션을 고르는 PerfectDodge는 어빌리티를 나눠도 발동 조건으로 가를 수 없습니다. 상황에 따라 모션만 다른 행동(회피, 움찔, 넉업)은 어빌리티 하나로 둡니다. |
| Tasks와 타임라인 | `Tasks`에는 어빌리티가 시작할 때 한 번 하거나 어빌리티 내내 이어지는 일을, `Sections`에는 몽타주 시각에 맞춘 일을 둡니다. 발동하면 `Tasks`를 모두 시작한 뒤 몽타주를 재생합니다. | 지금 발동 Task는 대부분 이 기준으로 나뉘어 있어, 몽타주 재생과 아래의 극한 회피 구간·그로기 효과만 `Tasks`에서 빠집니다(4절). 순서가 바뀌는 어빌리티는 5절입니다. |
| 극한 회피 구간 | Dodge 타임라인의 Backstep·Forward 섹션에 0~0.2초 구간으로 걸고, 효과는 다른 구간 효과처럼 `Infinite`로 둡니다. | 극한 회피 구간은 무적 구간처럼 모션 시간을 따릅니다(1-spec 2절). 회피의 구간(극한 회피, 무적, Busy)이 한 타임라인에 모입니다. |
| 그로기 | 그로기 효과는 적 캐릭터가 `Event.Groggy`를 받아 겁니다(효과 ID는 코드 상수). Groggy 어빌리티는 `State.Groggy`가 붙으면 발동하고(`OwnedTagPresent`), 떨어지면 엔진이 끝내며(`End: TriggerTag`), 모션만 맡습니다. 비전투 중에 걸리지 않는 조건은 효과 정의의 `ApplicationIgnoredTags`에 둡니다. | 그로기의 시작(효과)과 끝(PP 회복)을 PP·그로기 규칙의 주인인 적 캐릭터가 맡고, 어빌리티가 끝나는 때를 엔진 트리거가 정해 적 캐릭터의 취소 코드가 없어집니다. Groggy에는 `Tasks`와 `StartSections`가 필요 없습니다. 처형·사망 중에는 효과의 `RemovalTags`가, 이미 그로기·처형 중에는 PP가 줄지 않는 규칙이 지금처럼 그로기를 막습니다. |
| 사망 모션 | `DeathMontage`는 몽타주 ID 대신 에셋 경로입니다. | 어빌리티가 아니라 적 캐릭터가 사망할 때 재생하는 몽타주라 어빌리티를 나누지 않습니다. |
| 코드가 재생하는 몽타주 | 점프(`Jump`)와 가드 움찔(`GuardFlinch`)은 캐릭터 기본값(`JumpMontage`, `GuardFlinchMontage`)으로 옮기고 `BP_Player`에서 정합니다. | AGENTS.md 규칙 2의 '실행 구조를 구성하는 에셋 참조'입니다. 점프 몽타주를 Jump 어빌리티의 `Montage`로 두면 걸어서 떨어질 때도 재생돼 지금과 달라집니다. |
| 필드 이름 | 태그·발동 필드는 엔진 `UGameplayAbility`의 속성 이름으로 항목에 바로 적고, 트리거는 엔진 구조체 `FAbilityTriggerData`를 그대로 읽습니다. | GAS를 아는 사람과 AI가 매핑표 없이 읽습니다. 트리거 변환 코드와 프로젝트 enum이 없어집니다. |
| Transition 필드 이름 | `Tag`를 `TagRemoved`로, 넘어갈 섹션 `Section`을 `ToSection`으로 바꿉니다. 뜻은 그대로입니다(`TagRemoved`는 그 태그가 빠지면 전환하고, 시작할 때 없으면 다음 틱에 전환). | `Tag`만으로는 '태그가 붙으면 전환'으로 읽힙니다. `TagRemoved`는 엔진 `WaitGameplayTagRemoved`와 같은 이름 방식이라 GAS 지식으로 읽힙니다. 시작할 때 태그가 없으면 엔진 Task는 바로 넘어가고, 이 Task는 같은 때 시작한 항목이 태그를 붙일 수 있게 다음 틱에 넘어가는 차이가 있습니다. `ToSection`은 한 항목에 `EndSection`과 함께 있을 때 어느 쪽이 넘어갈 섹션인지 드러냅니다. 지금은 두 값을 바꿔 적어도 둘 다 유효한 키라 오류 없이 틀리게 동작합니다. StartSections 항목의 `Section`과 이름이 다르지만, 잘못 적으면 모르는 키 오류로 잡힙니다. |
| 복사한 어빌리티 태그 | 문서의 `AbilityTags` 설명에 '다른 어빌리티의 태그를 복사하면 그 태그에 걸린 코드 규칙도 따라온다. 새 행동은 `Ability.Action`만 단다'를 적습니다. 코드가 보는 태그의 목록은 적지 않습니다. | 데이터만으로 새 행동을 더할 때 처형 카메라·히트스톱 제외 같은 코드 규칙이 뜻하지 않게 붙지 않게 합니다. 목록은 코드와 같은 사실을 두 곳에 두게 되어, 코드가 새 태그를 보기 시작하면 낡습니다. |
| 입력 | 키로 발동하는 어빌리티는 `InputTag`를 적습니다. 키를 누르면 그 입력의 어빌리티를 모두 발동해 보고, 하나도 발동하지 않으면 그 입력 태그로 게임플레이 이벤트를 보냅니다(기본 공격 콤보). 같은 키의 어빌리티는 발동 조건이 서로 배타적이어야 합니다. 입력 계층이 직접 다루는 키는 6절입니다. | 키 하나로 발동하는 행동은 입력 계층을 고치지 않고 더합니다. 같은 시각의 일에 순서를 두지 않는 원칙에 따라 목록 순서로 고르지 않습니다. |
| HUD | 스킬·궁극기 칸은 `InputTag`가 `Input.Skill`, `Input.Ultimate`인 어빌리티를 찾습니다. | E·Q의 어빌리티를 바꿔도 HUD를 고치지 않습니다. |
| 다시 읽기 | 몽타주와 타임라인은 어빌리티 정의와 함께 발동할 때 복사하고, BlendTime 전환으로 다시 재생해도 그 사본을 씁니다. `InputTag`는 입력 계층이 누를 때마다 지금 데이터에서 읽습니다. | 지금은 재생할 때마다 타임라인을 다시 찾습니다. 정의를 한 번 복사하면 한 발동 안에서 값이 섞이지 않습니다. |
| 오류 검사 | 몽타주가 에셋 경로라 읽을 때 없는 에셋을 잡습니다. 섹션 이름과 몽타주의 대조는 지금처럼 하지 않습니다. | 지금은 검사하지 않는 몽타주 ID 누락이 읽기 오류가 됩니다. |
| 쿡 | 바꾸지 않습니다. | `ModifyCook`의 `FReferenceFinder`가 데이터 객체의 구조체 안 참조까지 모아, 어빌리티 데이터의 몽타주가 그대로 쿡 목록에 들어갑니다. |

## 2. 데이터 형식

### 어빌리티 항목

| 필드 | 뜻 | 지금 |
| - | - | - |
| `AbilityTags` | 어빌리티 태그. 스펙의 `DynamicSpecSourceTags`에 넣고, 다른 어빌리티의 취소·차단과 코드가 이 태그로 찾습니다. 다른 어빌리티의 태그를 복사하면 그 태그에 걸린 코드 규칙도 따라오므로, 새 행동은 `Ability.Action`만 답니다. | `Tags` |
| `InputTag` | 선택. 이 어빌리티를 발동하는 입력 | 없음(`AAgPlayerCharacter::RunInput`) |
| `ActivationRequiredTags`, `ActivationBlockedTags`, `ActivationOwnedTags` | 발동에 필요한 태그, 발동을 막는 태그, 실행 중 붙는 태그 | `Activation`의 `RequiredTags`, `BlockedTags`, `OwnedTags` |
| `BlockAbilitiesWithTag`, `CancelAbilitiesWithTag` | 실행 중 막는 어빌리티, 발동할 때 취소하는 어빌리티 | `Activation`의 `BlockTags`, `CancelTags` |
| `AbilityTriggers` | `{"TriggerTag", "TriggerSource"}`의 배열. `TriggerSource` 기본은 `GameplayEvent` | `Activation.Triggers`(`Tag`, `Source`) |
| `bAllowRetrigger` | 실행 중 다시 발동 | `Activation.bAllowRetrigger` |
| `Cost`, `Cooldown`, `Targeting`, `PatternSelection` | 그대로 | |
| `End` | `Tasks`(기본), `TriggerTag`, `InputReleased`(입력을 떼면 취소) | `InputReleased` 추가 |
| `Tasks` | 발동할 때 시작하는 Task(4절). PlayMontage는 적지 않습니다. | PlayMontage 포함 |
| `Montage` | 선택. 재생할 몽타주 | 몽타주 ID와 `MontageTimelines.json` |
| `DeathMontage` | 선택. 사망 모션 에셋 경로 | 몽타주 ID |

`Montage`의 필드는 다음과 같습니다.

| 필드 | 뜻 |
| - | - |
| `Asset` | 몽타주 에셋 경로 |
| `StartSections` | 선택. `{"Section", "RequiredTags", "BlockedTags", "MoveDirection", "EventTag"}`의 배열. 조건의 뜻은 지금 PlayMontage의 같은 필드와 같습니다. |
| `Sections` | 섹션 이름별 타임라인 항목 배열. 항목 형식은 [몽타주 타임라인과 구간 판정](<../../Docs/Architecture/몽타주 타임라인과 구간 판정.md>) '형식' 그대로이며, 아래 두 필드만 이름이 바뀝니다. |

### 타임라인 항목

| 필드 | 뜻 | 지금 |
| - | - | - |
| Transition의 `TagRemoved` | 그 태그가 빠지면 전환합니다. 시작할 때 태그가 없으면 다음 틱에 전환합니다. | `Tag` |
| Transition의 `ToSection` | 넘어갈 섹션 | `Section` |

### 효과 정의

| 필드 | 뜻 | 엔진 |
| - | - | - |
| `ApplicationIgnoredTags` (추가) | 걸 때 대상에 하나라도 있으면 걸지 않는 태그 | 대상 태그 조건 컴포넌트의 `ApplicationTagRequirements` |

### 예: Dodge

```json
"Dodge": {
  "AbilityTags": ["Ability.Action.Dodge"],
  "InputTag": "Input.Dodge",
  "ActivationBlockedTags": ["State.Busy", "State.HitReaction", "State.Groggy", "State.Execution", "State.Dead"],
  "ActivationOwnedTags": ["State.Acting"],
  "CancelAbilitiesWithTag": ["Ability.Action", "Ability.Sprint"],
  "bAllowRetrigger": true,
  "Cost": {"SP": 20},
  "Cooldown": 0.4,
  "Tasks": [
    {"Task": "RootMotion", "Direction": "MoveInput"}
  ],
  "Montage": {
    "Asset": "/Game/Animations/AM_Player_Dodge.AM_Player_Dodge",
    "StartSections": [
      {"Section": "Forward", "RequiredTags": ["Input.Move"]},
      {"BlockedTags": ["Input.Move"]}
    ],
    "Sections": {
      "Backstep": [
        {"Task": "ApplyEffect", "Start": 0, "End": 0.2, "Effect": "PerfectDodgeWindow"},
        {"Task": "ApplyEffect", "Start": 0, "End": 0.4, "Effect": "Busy"},
        {"Task": "ApplyEffect", "Start": 0, "End": 0.3, "Effect": "Invincible"}
      ],
      "Forward": [
        {"Task": "RootMotion", "Start": 0, "Direction": "MoveInput", "Distance": 400},
        {"Task": "ApplyEffect", "Start": 0, "End": 0.2, "Effect": "PerfectDodgeWindow"},
        {"Task": "ApplyEffect", "Start": 0, "End": 0.6, "Effect": "Busy"},
        {"Task": "ApplyEffect", "Start": 0, "End": 0.3, "Effect": "Invincible"}
      ]
    }
  }
}
```

## 3. 어빌리티별 변경

| 어빌리티 | 변경 |
| - | - |
| Dodge | `InputTag` `Input.Dodge`. `StartSections`: 이동 입력이 있으면 Forward, 없으면 첫 섹션(Backstep). `PerfectDodgeWindow`를 `Tasks`에서 빼고 Backstep·Forward 타임라인의 0~0.2초 구간으로 옮깁니다. |
| PerfectDodge | `StartSections` 9개: Forward → Forward, ForwardRight·Right·BackwardRight → Right, Backward → Backward, BackwardLeft·Left·ForwardLeft → 첫 섹션(Left), 입력 없음 → Backward |
| BasicAttack | `InputTag` `Input.Attack`. `StartSections`: `State.GuardCounterRecovery`면 Attack2, 아니면 첫 섹션(Attack1). `ActivationBlockedTags`에 `State.Airborne`, `State.DodgeCounterChance`, `State.GuardCounterChance` 추가 |
| JumpAttack | `InputTag` `Input.Attack` |
| DodgeCounter | `InputTag` `Input.Attack`. `ActivationBlockedTags`에 `State.Airborne` 추가 |
| GuardCounter | `InputTag` `Input.Attack`. `ActivationBlockedTags`에 `State.Airborne`, `State.DodgeCounterChance` 추가 |
| Guard | `InputTag` `Input.Guard`, `End` `InputReleased` |
| Skill, Ultimate, Execution | `InputTag` `Input.Skill`, `Input.Ultimate`, `Input.Execute` |
| KnockUp | `StartSections`: 지상이면 첫 섹션(Rise), 공중이면 AirHit |
| Juggle | `StartSections`: AirHit. `AM_KnockUp`에서 지나가는 AirHit·AirFall·Land·GetUp의 항목을 KnockUp과 같게 적습니다. |
| Flinch | 지상 움찔만 남기고 `ActivationBlockedTags`에 `State.Airborne` 추가. `StartSections` 5개가 부위 이벤트(`Event.Hit.Stagger.Back` 등)로 섹션을 고릅니다. |
| FlinchAir (신규) | Flinch와 태그·발동 조건이 같고 `State.Airborne`을 요구하며 `AM_FlinchAir`를 재생합니다. 캐릭터 6명의 어빌리티 목록에 더합니다. |
| Groggy | `AbilityTriggers`: `State.Groggy`(`OwnedTagPresent`), `End` `TriggerTag`. `Event.Groggy` 트리거와 `Tasks`(그로기 효과)를 뺍니다. `ActivationBlockedTags`는 `State.HitReaction.KnockDown`·`KnockUp`·`GuardPushback`·`GuardBreak`이고, `CancelAbilitiesWithTag`는 그대로입니다. 움찔 중에는 움찔을 끊고 그로기 모션을 재생하고, 다른 피격 반응 중에는 발동이 막혀 반응이 끝난 뒤 ABP가 그로기 자세를 보여 줍니다(엔진은 막혔던 태그 트리거를 다시 시도하지 않음). |
| Executed | `DeathMontage`가 에셋 경로 |
| SingleSlash, DoubleSlash, HeavyCleave | `Direction`이 Back이고 거리를 적지 않은 타임라인 구간 4개에 `"Distance": 0`을 적어, 제자리에 머물고 돌지 않는 구간임을 드러냅니다. |
| DashThrust, Groggy, Ultimate | 타임라인의 Transition 4곳에서 `Tag`를 `TagRemoved`로 적습니다. |
| Transition이 있는 모든 어빌리티 | 타임라인의 Transition에서 넘어갈 섹션을 `Section` 대신 `ToSection`으로 적습니다. |

공격 키에 묶이는 네 어빌리티에 더하는 막는 태그는, 지금 입력 계층이 코드에서 고르는 순서(공중 → 회피 반격 기회 → 가드 반격 기회 → 기본 공격)를 데이터로 옮긴 것이라 동작은 같습니다.

효과 정의는 다음 둘을 바꿉니다.

| 효과 | 변경 |
| - | - |
| `PerfectDodgeWindow` | `Infinite`로 바꾸고 `Duration`을 지웁니다. 길이는 Dodge 타임라인의 구간이 정합니다. |
| `Groggy` | `ApplicationIgnoredTags`에 `State.NonCombat`. 비전투 중에는 그로기가 시작되지 않는 지금 동작을 유지합니다. |

## 4. Tasks에 남는 발동 Task

| 경우 | 어빌리티 | 타임라인으로 옮기지 않는 이유 |
| - | - | - |
| 몽타주가 없음 | Guard, Sprint, LockOn, EnemyLockOn, Parried | 타임라인이 없습니다. |
| 발동 순간 돌기(구간 없는 `RootMotion`) | Dodge, PerfectDodge, Parry, KnockDown, KnockUp, Juggle, GuardPushback, GuardBreak, Executed | 타임라인의 `RootMotion`은 구간 동안 캐릭터 회전 속도로 돕니다. 추적 거리가 없는 어빌리티에서는 행동 대상 대신 락온 대상 쪽으로 돌고, 락온 대상이 없으면 돌지 않습니다. |
| Launch | KnockUp, Juggle, GroundSlam | 띄운 뒤 착지할 때까지 살아 있어야 합니다. 이 몽타주들은 BlendTime 전환으로 다시 재생하고, 다시 재생하면 켜진 구간이 끝나고 새 위치에서 다시 시작해 공중에서 한 번 더 띄웁니다. |
| 어느 섹션으로 시작하든 한 번 | PerfectDodge(보상, 회피 반격 기회, 슬로우모션) | 타임라인에 두면 시작 섹션마다 같은 항목을 반복해야 하고(4섹션 × 3줄), 하나라도 빠뜨리면 방향에 따라 결과가 달라집니다. |
| 옮길 수 있지만 두는 것 | Parry(보상, SP 리젠 대기), Execution(`Event.Executed`) | 섹션이 하나라 0초 항목으로 옮겨도 같지만, 같은 성격의 다른 항목이 `Tasks`에 남아 자리가 갈리지 않게 둡니다. |

## 5. 순서가 바뀌는 어빌리티

| 어빌리티 | 지금 | 바뀐 뒤 | 결과가 같은 이유 |
| - | - | - | - |
| Execution | 몽타주 재생 → `Event.Executed` | `Event.Executed` → 몽타주 재생 | 이벤트가 그 자리에서 상대의 처형 피격을 발동해, 두 몽타주가 같은 프레임에 시작합니다. |
| KnockUp, Juggle, GroundSlam | 몽타주 재생 → Launch | Launch → 몽타주 재생 | Launch의 속도는 다음 이동 틱에 적용되고, 몽타주 루트 모션 끄기는 캐릭터 단위입니다. 공중 태그도 다음 이동 틱에 붙어 KnockUp의 시작 섹션 판정이 같습니다. |

회귀 테스트에서 이 네 어빌리티를 따로 확인합니다.

## 6. 입력 계층

- 누름: 막힘 검사와 선입력은 지금처럼 입력 태그 단위입니다. 막히지 않으면 그 입력이 `InputTag`인 어빌리티를 모두 발동해 보고, 하나도 발동하지 않으면 그 입력 태그로 자기에게 게임플레이 이벤트를 보냅니다.
- 뗌: 그 입력의 어빌리티 중 실행 중이고 `End`가 `InputReleased`인 것을 취소합니다.
- 누르고 있음: 입력 제한(`State.Busy`)이 풀리거나 행동·피격 반응이 끝나면, 누르고 있는 입력의 `InputReleased` 어빌리티를 다시 발동해 봅니다. 선입력한 그런 입력은 아직 누르고 있을 때만 실행합니다(지금의 가드).
- 입력 계층에 남는 규칙:
  - 점프: 엔진 `Jump`를 거쳐 공중 태그 트리거로 발동하므로 Jump에는 `InputTag`를 적지 않습니다. 점프 행동의 Busy는 점프 공격 입력을 막지 않습니다.
  - 락온: 누를 때마다 켜고 끕니다.
  - 달리기: 회피 키를 누른 채 회피 후딜에 이동 입력이 있으면 시작하고, 키를 떼거나 SP가 없으면 끝납니다.
  - 가드 중 패리: 누르고 있는 가드가 실행 중이면, 입력 제한이 풀릴 때 후딜에 든 패리 모션을 끝내 가드 자세로 돌아갑니다.
  - 처형 입력과 공중의 점프 입력은 선입력하지 않습니다.
- 반격 기회를 지우는 규칙과 가드 중 다른 행동이 끝났는지 보는 규칙은 지금처럼 `Ability.Action`·`Ability.Reaction` 부모 태그로 봅니다. 새 행동도 `Ability.Action` 아래 태그를 가지면 이 규칙을 따릅니다.

## 7. 클래스·구조체

| 대상 | 변경 |
| - | - |
| `FAgAbilityData` | 필드 이름을 바꿔 펼치고 `InputTag`, `Montage`(`TOptional<FAgAbilityMontage>`), `DeathMontage`(`TObjectPtr<UAnimMontage>`)를 둡니다. `HasSameGrantSettings`는 펼친 발동 필드를 비교합니다. |
| `FAgAbilityMontage` (신규) | `Asset`, `StartSections`, `Sections`. `MakeTask`로 재생 Task를 만듭니다. |
| `FAgMontageStart` (신규) | 시작 섹션 하나와 그 조건 |
| `FAgAbilityActivation`, `FAgAbilityTrigger`, `EAgTriggerSource` | 삭제 |
| `FAgPlayMontageParams` | 삭제. JSON의 Task 이름 `PlayMontage`도 없어집니다. |
| `UAgAbilityTask_PlayMontage` | 받은 `FAgAbilityMontage` 사본으로 시작 섹션을 고르고 재생하며, 타임라인을 그 사본에서 읽습니다. |
| `FAgTransitionParams` | `Tag`를 `TagRemoved`로, `Section`을 `ToSection`으로 바꿉니다. |
| `FAgMontageTimeline`, `UAgMontageTimelineData` | 삭제. 섹션 하나의 항목 배열(`FAgTimelineSection`)은 남습니다. |
| `UAgAssetManager` | `MontageTimelines.json` 등록, `MontageTimelines`, `GetMontageTimelines` 삭제 |
| `UAgCharacterData` | `Montages`, `FindMontage` 삭제 |
| `UAgGameplayAbility` | 발동 Task를 모두 시작한 뒤 `Montage`가 있으면 재생 Task를 시작합니다. |
| `AAgCharacterBase` | 스펙 트리거를 정의에서 그대로 복사합니다. `GuardFlinchMontage` 기본값 |
| `AAgPlayerCharacter` | 입력 계층(6절), `JumpMontage` 기본값 |
| `AAgEnemyCharacter` | 사망 모션을 정의의 에셋으로 재생합니다. `Event.Groggy`를 받아 그로기 효과를 걸고, 그로기가 끝날 때의 어빌리티 취소를 지웁니다(PP 회복은 그대로). |
| `FAgEffectDefinition`, `AgEffectData` | `ApplicationIgnoredTags`를 읽어 대상 태그 조건 컴포넌트의 `ApplicationTagRequirements`에 넣습니다. |
| `UAgDamageResultGameplayEffectComponent` | 가드 움찔을 캐릭터의 `GuardFlinchMontage`로 재생합니다. |
| `UAgHUDWidget` | 스킬·궁극기를 `InputTag`로 찾습니다. |

## 8. 에셋

| 에셋 | 변경 |
| - | - |
| `BP_Player` | `JumpMontage` = `AM_Player_Jump`, `GuardFlinchMontage` = `AM_GuardFlinch` |

## 9. 문서 반영

- [데이터 로딩과 스키마](<../../Docs/Architecture/데이터 로딩과 스키마.md>): 기술 결정의 '읽는 방법'·'데이터를 찾는 방법'·'어빌리티의 데이터', '파일 구성'과 흐름도, '어빌리티'(`AbilityTags`의 복사 주의 포함), '몽타주 관리', '재로딩 반영 시점', '클래스 구조', 'JSON 외부 데이터 위치'(점프·가드 움찔 몽타주), '효과' 형식(`ApplicationIgnoredTags`)
- [몽타주 타임라인과 구간 판정](<../../Docs/Architecture/몽타주 타임라인과 구간 판정.md>): 타임라인의 자리, 시작 섹션, PlayMontage 항목, Transition의 `TagRemoved`·`ToSection`
- [캐릭터 기반 구조와 GAS](<../../Docs/Architecture/캐릭터 기반 구조와 GAS.md>): '어빌리티 구조'의 입력 계층 문장, '상태'의 그로기 문장, '피격 상태와 AI 조건' 표(Flinch, FlinchAir), 캐릭터 클래스 표, '어빌리티 실행' 시퀀스, 'GameplayEffect'에서 효과를 거는 객체(그로기 효과는 적 캐릭터)와 '코드가 효과를 쓰는 방법'의 코드가 거는 JSON 효과
- [시간 규칙](<../../Docs/Design/System/시간 규칙.md>) '시간 연출의 영향': 극한 회피 구간을 '모션을 따르는' 줄로 옮깁니다(1-spec 2절).
- 에디터 작업 스킬의 `애니메이션.md`, `에디터 파이썬 이름.md`에 있는 `MontageTimelines.json` 언급

## 10. 이관

- JSON은 스크래치패드의 일회성 스크립트로 바꿉니다. 어빌리티마다 몽타주 ID를 에셋 경로로 풀고 그 몽타주의 타임라인을 옮기며 필드 이름을 바꿉니다. 3절의 특수 케이스는 손으로 고칩니다. 스크립트는 저장소에 두지 않습니다.
- 형식이 바뀌면 기획자가 작업 중인 JSON과 충돌합니다. 전환 직전에 기획자의 변경을 반영하고, 전환 뒤에는 새 형식으로 고치도록 사용자가 알립니다.
- 구현 순서는 (1) 데이터 형식과 몽타주 재생, (2) 입력과 HUD이며, 단계마다 회귀를 확인합니다.
