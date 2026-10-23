# 보스 AI와 패턴

보스를 어떻게 구현할지 정합니다.

행동의 실행 구조와 Task는 [어빌리티와 AbilityTask](<어빌리티와 AbilityTask.md>)를 따릅니다.

## 기술 결정

| 항목 | 결정 | 이유 |
| - | - | - |
| 패턴 | 모든 패턴은 어빌리티(`UAgGameplayAbility`)의 독립 스펙으로 실행합니다. 보스 데이터(`Characters.json`의 `Boss`)의 `Abilities`에 적힌 패턴 ID마다 `Abilities.json`의 정의(`Tags`, `Activation`, `Cooldown`, `Targeting`, `End`, `Tasks`, `PatternSelection`)를 사용합니다. AI는 `PatternSelection`이 있는 항목의 페이즈·허용 거리·가중치와 쿨다운으로 후보를 고릅니다. 패턴은 모션과 몽타주 시간표로 실행하고, 돌진(DashThrust)·도약(GroundSlam)은 범용 Task(Dash, Launch)가, Backstep의 물러나는 거리는 범용 Task(RootMotion)가 맡습니다. 공격 패턴의 재생 속도([보스 사양](<../Design/보스 사양.md>) '모션 재생 속도')는 공격 패턴 몽타주의 Rate Scale에 둡니다. | 패턴별 자식 클래스나 패턴 전용 Task가 필요 없습니다. 부여·실행 사본·쿨다운은 [캐릭터 기반 구조와 GAS](<캐릭터 기반 구조와 GAS.md>), 모션과 이동은 [캐릭터 이동과 모션 제어](<캐릭터 이동과 모션 제어.md>)를 따릅니다. |
| AI | 보스 AI의 흐름은 StateTree 에셋 `ST_Boss`의 상태와 전환으로 짜고, 보스 AI 컨트롤러의 StateTree AI 컴포넌트가 실행합니다. `AAgBossCharacter::AIStateTree`를 `BP_Boss`의 기본값에서 지정하고, C++ 컨트롤러가 빙의할 때 보스 캐릭터에서 가져와 적용합니다. 상태는 패턴 후딜레이, 패턴 실행, 다가가기, Backstep, 페이즈 전환, 피격 반응, 비전투, 사망입니다. 보스 사양 '전투 시작과 상태 전환' 표의 전환은 상위 상태에 우선순위가 높은 전환으로 두어, 같은 프레임에 행동이 끝나도 표의 전환이 먼저 적용됩니다. 넉업, 넉다운, 그로기, 처형 피격, 페이즈 전환은 상태 어빌리티가, 사망은 캐릭터가 처리합니다. 넉업, 넉다운, 그로기나 처형 피격이 시작되면 피격 반응 상태에서 기다리고, 모두 끝나면 패턴 후딜레이로 넘어갑니다. 보스가 사망하면 사망 상태로 넘어가 엔진 Delay 태스크(계속 실행)로 머물며 더 전환하지 않습니다. 사망 전환은 루트가 아니라 전투, 피격 반응, 비전투에 둡니다. 루트에 두면 사망 상태에서도 매 틱 같은 상태로 다시 전환합니다. | 엔진이 권장하는 AI 구조라 상태와 전환을 에디터에서 한눈에 보고, 실행 중에는 StateTree 디버거로 따라갈 수 있습니다. |
| AI 대상 | 보스의 락온 행동(`EnemyLockOn`)의 대상(`GetLockOnTarget`)이 AI 대상이고, 그 발동과 유지는 [대상 찾기와 락온](<대상 찾기와 락온.md>) '락온'을 따릅니다. AI 태스크와 조건, 보스 회전은 이 대상을 쓰고, 공격 패턴도 같은 프리셋(`Targeting.Preset`이 `Enemy`)과 각 행동의 `Targeting.Range`로 행동 대상을 정합니다(대상과의 최소 간격, 추적 구간, DashThrust의 돌진 방향, GroundSlam의 착지 목표). 락온 마커·처형 안내의 표시와 GroundSlam의 착지 셰이크처럼 플레이어 화면에 관한 것은 로컬 플레이어를 기준으로 합니다. | 플레이어와 같은 락온 행동과 대상 규칙을 씁니다. 락온은 죽거나 범위 밖으로 나간 대상을 놓으므로, 비전투 조건(플레이어 사망)은 대상이 아니라 플레이어 캐릭터를 봅니다. |
| 패턴 선택 | 패턴 실행 상태의 C++ 태스크 하나가 후보를 모으고 가중치로 고른 뒤 그 패턴을 실행합니다. 후보가 없거나 시작하지 못하면 다가가기로 넘어가 재시도 간격 뒤 다시 고릅니다. 허용 거리와 페이즈 판단은 `FAgPatternSelection::IsAvailable` 하나로 하며, Backstep 발동 조건도 이를 씁니다. | 패턴마다 상태를 만들지 않아 패턴 목록이 보스 데이터의 `Abilities` 한 곳에만 있습니다. StateTree의 가중치 선택 기능은 패턴마다 상태가 필요합니다. |
| Backstep | 플레이어의 기본 공격 각 타와 점프 공격이 시작할 때 그 공격의 행동 대상이 자기 보스이면 패턴 후딜레이 상태에서 Backstep 발동을 검사합니다. 발동 조건과 Backstep 상태의 패턴 실행은 다른 패턴처럼 패턴 ID(`Backstep`)로 스펙을 찾으며, ID는 `ST_Boss`에서 파라미터로 넣습니다. 물러나는 거리는 몽타주 전체를 덮는 RootMotion 구간(`Direction` `Back`)이 시작할 때 바라보던 방향 그대로 뒤로 보스 사양 'Backstep'의 거리만큼 가도록 루트 모션을 맞춥니다([어빌리티와 AbilityTask](<어빌리티와 AbilityTask.md>) 'Task 목록'). 공격 패턴이 아니므로 Rate Scale을 바꾸지 않고, 물러나는 동안 돌지 않습니다(행동 중 자동 회전 정지). 끝나면 바로 다음 패턴을 고르되, 페이즈 전환 HP 구간이면 페이즈 전환이 먼저입니다. | 다른 적을 공격할 때는 검사하지 않습니다. Backstep의 허용 거리(보스 사양 '패턴 목록')가 기본 공격 타겟팅 범위보다 길어, 타겟팅 범위로 대상을 제한하면 범위 밖에서 Backstep을 쓰지 않게 됩니다. 보스 사양 '모션 재생 속도'는 공격 패턴에만 적용됩니다. 곧게 물러난 뒤에는 바로 이어지는 다음 패턴의 추적 구간이 다시 플레이어 쪽으로 돌립니다. |
| 패턴 전조 동작 (DoubleStrike, Sweep, JumpCleave, QuintupleStrike) | 몽타주에서 패턴 전조 동작 부분을 느린 재생 속도의 별도 구간으로 나누고, 그 뒤의 시간표 시각과 노티파이를 늘어난 만큼 미룹니다. QuintupleStrike는 같은 방법으로 느리거나 빠른 구간을 나눠 리듬을 맞춥니다. | 몽타주 시간 자체가 늘어나므로 판정 구간과 추적 구간이 모두 같은 기준으로 맞습니다. |
| 넉업, 넉다운 | 보스도 플레이어와 같은 넉업·저글·넉다운 정의를 받고, 몽타주는 각자의 캐릭터 데이터에서 읽습니다. 피격 반응 어빌리티는 진행 중인 보스 패턴도 취소하고, 패턴은 피격 반응 중에 시작하지 못하며, 플레이어와 보스 모두 피격 반응 중에는 회전하지 않습니다. 두 캐릭터의 기본 회전 제한은 [캐릭터 이동과 모션 제어](<캐릭터 이동과 모션 제어.md>)의 캐릭터 회전을 따릅니다. | 넉다운 규칙이 캐릭터마다 같아 정의 하나로 처리합니다. |
| 비전투 | 플레이어가 사망하면 AI가 비전투 상태로 넘어가 보스에게 비전투 태그를 붙이고 진행 중인 패턴을 취소합니다. 이 태그는 패턴, 그로기, 페이즈 전환을 막고 보스 회전을 멈춥니다. | 비전투 뒤에는 사망만 적용합니다. |
| 이동 | 다가가기와 좌우 걷기는 AI 태스크가 매 프레임 이동 입력을 직접 줍니다. NavMesh는 쓰지 않습니다. | 아레나가 장애물 없는 평지라 길찾기가 필요 없습니다. |

## 클래스 구조

### 보스 AI

보스 AI의 흐름(상태와 전환)은 StateTree 에셋 `ST_Boss`에 있어 에디터에서 그림으로 봅니다. 아래는 그 에셋을 실행하는 코드입니다. 태스크와 조건은 모두 `Controller`(Context 속성)로 `AAgBossAIController`를 받아 보스, 보스 데이터, 현재 대상(`GetTarget`)을 읽으며, 그 선은 그리지 않습니다.

| 클래스 | 부모 | 역할 |
| - | - | - |
| `AAgBossAIController` | `AAIController` | 보스 AI. 락온 행동을 발동해 현재 대상(보스의 락온 대상)을 잡아 AI 포커스로 지정하고, StateTree AI 컴포넌트로 `ST_Boss`를 실행하고, 이 보스가 대상인 플레이어의 기본 공격 시작을 StateTree 이벤트로 넘깁니다. |
| 보스 AI 태스크 | `FStateTreeAITaskBase` | 패턴 후딜레이 대기, 다가가기, 패턴 실행(선택 포함), 페이즈 전환, 피격 반응 대기, 비전투 |
| 보스 AI 조건 | `FStateTreeAIConditionBase` | 보스 사망, 플레이어 사망, 피격 반응 중, 페이즈 전환 시작 조건, Backstep 발동 조건 |

```mermaid
classDiagram
    direction LR
    class UAgBossData {
        <<Characters.json의 Boss>>
    }
    class UStateTree {
        <<ST_Boss>>
    }
    class FStateTreeAITaskBase {
        <<struct>>
    }
    class FStateTreeAIConditionBase {
        <<struct>>
    }
    class FAgBossAITask_Wait {
        <<패턴 후딜레이>>
    }
    class FAgBossAITask_Approach {
        <<다가가기>>
    }
    class FAgBossAITask_RunPattern {
        <<패턴 실행, Backstep>>
    }
    class FAgBossAITask_PhaseTransition {
        <<페이즈 전환>>
    }
    class FAgBossAITask_WaitReaction {
        <<피격 반응>>
    }
    class FAgBossAITask_NonCombat {
        <<비전투>>
    }
    AAgBossCharacter --> UStateTree : AIStateTree
    AAgBossAIController ..> UStateTree : SetStateTree<br>StartLogic
    FStateTreeAITaskBase <|-- FAgBossAITask_Wait
    FStateTreeAITaskBase <|-- FAgBossAITask_Approach
    FStateTreeAITaskBase <|-- FAgBossAITask_RunPattern
    FStateTreeAITaskBase <|-- FAgBossAITask_PhaseTransition
    FStateTreeAITaskBase <|-- FAgBossAITask_WaitReaction
    FStateTreeAITaskBase <|-- FAgBossAITask_NonCombat
    FStateTreeAIConditionBase <|-- FAgBossAICondition_BossDead
    FStateTreeAIConditionBase <|-- FAgBossAICondition_PlayerDead
    FStateTreeAIConditionBase <|-- FAgBossAICondition_Reacting
    FStateTreeAIConditionBase <|-- FAgBossAICondition_PhaseTransitionReady
    FStateTreeAIConditionBase <|-- FAgBossAICondition_BackstepReady
```

### 보스 패턴

```mermaid
classDiagram
    UGameplayAbility <|-- UAgGameplayAbility
    UAgGameplayAbility --> FAgAbilityData : ID별 실행 사본
    FAgAbilityData *-- FAgPatternSelection : AI 선택 조건
    FAgAbilityData *-- FAgAbilityTargeting : Enemy
    UAgGameplayAbility ..> UAgAbilityTask_PlayMontage : 발동
    UAgGameplayAbility ..> UAgAbilityTask_Launch : 도약
    UAgAbilityTask_PlayMontage ..> FAgMontageTimeline : 재생마다 복사해 판정
    UAgAbilityTask_PlayMontage ..> UAgAbilityTask_WeaponHit : 판정
    UAgAbilityTask_PlayMontage ..> UAgAbilityTask_Dash : 돌진
    UAgAbilityTask_PlayMontage ..> UAgAbilityTask_ApplyEffect : 캐릭터 통과
    UAgAbilityTask_PlayMontage ..> UAgAbilityTask_SpawnProjectile : 발사
    UAgAbilityTask_SpawnProjectile ..> AAgProjectile : Launch
```

### 보스 패턴 실행

```mermaid
sequenceDiagram
    participant T as 패턴 실행 태스크<br>FAgBossAITask_RunPattern
    participant B as AAgBossCharacter
    participant ASC as 보스 ASC
    participant GA as 패턴 어빌리티
    T->>T: GatherCandidates, 가중치로 선택(Algo::SelectRandomWeightedBy)
    T->>B: FindAbilitySpec (패턴 ID)
    T->>ASC: TryActivateAbility
    ASC->>GA: 시작
    GA->>GA: 정의 사본, Targeting으로 대상
    GA->>GA: PlayMontage로 몽타주 재생, 시간표 판정
    GA-->>ASC: 재생이 끝나면 끝남
    T->>ASC: 매 프레임 스펙이 실행 중인지 확인
    Note over T: 끝났으면 성공 →<br>ST_Boss가 패턴 후딜레이로 넘어감
```

Backstep은 플레이어의 기본 공격 각 타와 점프 공격의 시간표가 섹션 시작에 플레이어 자신에게 보내는 시작 이벤트(`Event.BasicAttackStarted`, SendGameplayEvent)로 시작합니다. 이벤트의 Target은 그 재생의 행동 대상(기본 공격·점프 공격 Targeting의 후보에 포함된 락온 대상 우선, 없으면 가장 가까운 첫 후보)입니다. 컨트롤러가 대상(플레이어) ASC의 이 이벤트 가운데 Target이 자기 보스인 것만 StateTree 이벤트로 넘기면, 패턴 후딜레이 상태의 전환이 발동 조건과 확률을 검사해 Backstep 상태로 보냅니다. 페이즈 전환은 페이즈 전환 태스크가 보스에게 `Event.PhaseTransition`을 보내 시작합니다. 페이즈 전환이 정상 종료로 끝나면(취소가 아니면) 보스 캐릭터가 자기 ASC의 `OnAbilityEnded`로 받아 PP를 채우고(`PPRefill`) 현재 HP로 들어선 마지막 페이즈(`GetReachedPhase`, 보스 데이터 `PhaseTransitionHPRatios`의 순서)로 넘어갑니다. 컨트롤러는 시작한 페이즈 전환이 향하는 페이즈를 남겨, 전환이 취소되어도 같은 페이즈로는 다시 전환하지 않습니다. AI는 페이즈 전환을 시작만 하고, 페이즈와 PP 회복은 그로기·처형 피격 끝과 함께 보스 캐릭터가 맡습니다(아래 '그로기와 처형 피격').

SwordWave는 PlayMontage가 몽타주를 재생하고, 세 발은 몽타주 시간표의 `SpawnProjectile` 항목이 실행합니다. 투사체 정의와 발사 처리는 [데이터 로딩과 스키마](<데이터 로딩과 스키마.md>)와 [전투 연출과 시간 제어](<전투 연출과 시간 제어.md>)를 따릅니다.

### 그로기와 처형 피격

그로기는 이벤트로 발동하며, 발동 Task의 첫 항목인 ApplyEffect가 `Groggy` 효과를 겁니다. 피격 반응 중(`State.HitReaction`)에는 시작하지 못하고(BlockedTags), 보스 캐릭터가 그 태그가 사라질 때(다음 틱) PP가 0 이하면 그로기 이벤트를 다시 보내 일어난 뒤 시작합니다(전투 시스템 '그로기'). 이때 보스 캐릭터는 이벤트를 보내는 동안만 `State.GroggyDelayed`를 붙여, 그로기 행동이 서로 배타적인 조건의 PlayMontage 둘 가운데 Loop부터 재생하는 쪽을 고르게 합니다. Loop의 종료 전환(Tag)은 함께 발동한 ApplyEffect보다 먼저 볼 수 있으나, Transition이 다음 틱에 다시 보므로 순서에 기대지 않습니다. 그로기 효과는 그로기 행동만 걸므로, 처형 대상 조건(`State.Groggy`)은 그로기 행동 중에만 맞습니다. 지속 시간은 `Effects.json`에 있고 효과가 `State.Groggy`를 부여합니다. OwnedTags에는 같은 태그를 넣지 않습니다. ApplyEffect는 행동 끝까지 남아 자기 효과를 소유하므로, 몽타주가 먼저 끝나도 효과의 지속 시간까지 행동이 유지됩니다.

보스 캐릭터는 `State.Groggy`가 사라지면 살아 있고 `State.Execution`이 없을 때 PP를 회복하며, 그로기 어빌리티를 취소합니다. 효과 만료가 모션과 시간표 Task도 끝내므로 히트스톱이나 공중 상태가 종료를 늦추지 않습니다. 그로기 행동이 처형이나 사망으로 먼저 취소되면 ApplyEffect가 효과를 제거합니다. 사망 시에는 PP를 회복하지 않습니다.

처형 피격은 `State.Execution`을 소유하고 그로기 행동을 취소합니다. 엔진은 소유 태그를 먼저 붙인 뒤 취소를 실행하므로 보스 캐릭터는 그로기 종료 시 이 태그를 확인해 PP 회복을 미룹니다. 그로기 상태를 이어 받지 않습니다. 보스 캐릭터는 `OnAbilityEnded`에서 처형 피격(`Ability.Reaction.Executed`)의 종료를 받아 취소 여부와 관계없이 살아 있으면 `PPRefill`을 적용합니다. 처형 중 PP 감소 방지는 CanLosePP가, AI 대기는 IsBossReacting이 `State.Execution`을 함께 검사합니다. 복귀 모션은 Loop의 Transition이 `State.Groggy` 효과의 종료 예정 시점에 맞춰 먼저 시작하지만, 모션 완료가 상태 종료를 결정하지 않습니다.
