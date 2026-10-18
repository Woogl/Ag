# 캐릭터와 GAS

캐릭터와 GAS를 어떻게 구현할지 정합니다.

## 기술 결정

| 항목 | 결정 | 이유 |
| - | - | - |
| ASC 위치 | 캐릭터에 둡니다. | 싱글플레이라 PlayerState에 둘 이유가 없습니다. |
| 어트리뷰트 | 세트 하나에 기획서 스탯을 기획서 이름 그대로 둡니다. HP, MaxHP, SP, MaxSP, MP, MaxMP, UP, MaxUP, PP, MaxPP, ATK, DEF, MOV. 보스가 쓰지 않는 SP·MP·UP는 0입니다. 기본값이 바뀌기 전에 범위(0 ~ 최대값)로 고정합니다. | 기획서, 데이터 위치표, 코드의 이름이 같아 대조하기 쉽습니다. 최대값에서 리젠이 더해져도 기본값이 넘치지 않아야 다음 감소가 바로 보입니다. |
| 어빌리티 목록 | 캐릭터 데이터(`Characters.json`)의 `Abilities`에 적힌 어빌리티 ID마다 단일 `UAgGameplayAbility`를 독립 스펙으로 부여합니다. 정의는 `Abilities.json`에 있습니다. `GiveAbility` 전에 핸들과 ID를 연결하고 스펙의 동적 태그·트리거를 설정합니다. `InstancedPerActor` 인스턴스는 자기 ID의 정의와 실행 사본을 사용합니다. | 같은 클래스라도 조건·이벤트·재발동·쿨다운·GE 출처가 행동마다 구분됩니다. |
| 어빌리티 구조 | 행동 전체를 나타내는 자식 Ability나 흐름 Task는 두지 않습니다. Runner(`UAgGameplayAbility`)가 엔진 어빌리티 기능과 시간표를 맡고, 범용 Task가 기능 하나씩을 수행하며, 행동이 아닌 규칙은 주인 객체(캐릭터, 입력 계층, 피격 처리, 보스 AI)가 맡습니다([어빌리티와 AbilityTask](<어빌리티와 AbilityTask.md>)). EndPlay에서는 메시 해제 전에 모든 Ability를 취소합니다. | 기능별 획득과 복원이 함께 있고, 한 기능의 완료가 다른 기능이나 전체 Ability를 임의로 종료하지 않습니다. |
| 상태 | 4순위 상태(넉업, 넉백, 가드 밀림, 가드 브레이크), 그로기, 처형 피격, 페이즈 전환은 이벤트로 발동하는 행동(GameplayEvent 트리거)으로 만들고, 상태 우선순위는 태그로 나타냅니다. 높은 상태는 낮은 상태를 취소하고, 낮은 상태는 높은 상태 중에 시작하지 못합니다. 4순위 상태의 어빌리티 태그는 한 부모 태그(`Ability.Reaction.HitReaction`) 아래에 두어, 높은 상태가 부모 태그 하나로 취소합니다. 사망은 행동이 아니라 캐릭터가 사망 이벤트를 받아 처리합니다([사망 처리](<사망 처리.md>)). 행동이 붙이는 상태 태그는, 행동 내내 붙으면 ActivationOwnedTags에 두고, 일부 구간에만 붙으면(회피의 무적 구간, 후딜 전의 Busy 등) 시간표의 HoldTag로 붙입니다. | 행동과 같은 방식이라 규칙이 한곳에 모입니다. 두 방식 모두 행동이 끝나면 태그가 떨어지므로, 행동마다 태그를 떼는 코드를 두지 않습니다. |
| 쿨다운 | 어빌리티(플레이어 행동, 보스 패턴)의 쿨다운은 쿨다운 효과(`Cooldown`) 하나로 셉니다. 어빌리티마다 태그를 두지 않고, 효과의 에셋 태그(`Effect.Cooldown`)와 걸 때 넣은 출처(그 스펙의 어빌리티 인스턴스)로 어빌리티를 구분합니다. 시간은 어빌리티 항목의 `Cooldown`이며 0이면 쿨다운이 없습니다. 플레이어와 보스가 이 길 하나를 씁니다. 패리 구간, 회피 반격 기회, 그로기 시간처럼 계속 흐르는 시간과 태그는 그 효과(`ParryWindow`, `DodgeCounterChance`, `GroggyTime`)가 정합니다. 비용(SP·MP·UP)은 '자원 변화'를 따릅니다. | 엔진의 쿨다운 GE는 클래스(`CooldownGameplayEffectClass`)로만 받아, JSON에서 만든 효과로 쿨다운을 확인·적용하는 함수를 재정의합니다. 시간은 어빌리티 정의에 두고 효과 정의에는 모든 어빌리티가 같은 지속 방식과 태그만 둡니다. 어빌리티마다 태그를 두지 않아 어빌리티를 더해도 태그가 늘지 않습니다. 쿨다운 유무를 시간 하나로 정해, 쿨다운 효과가 있는지와 시간이 어긋나지 않습니다. |
| 자원 변화 | 자원이 바뀌는 일마다 그 규칙을 가진 효과를 적용합니다. 행동 비용은 엔진이 커밋할 때 부르는 `ApplyCost`에서, 공통 Ability가 실행 사본의 Resource·Amount를 자원 효과(`Resource`)의 SetByCaller에 음수로 넣어 치릅니다. 치를 수 있는지(`CheckCost`)는 같은 고정 비용으로 확인합니다. 적중의 MP·UP 충전과 극한 회피의 SP 환급은 같은 자원 효과에 얻는 양을 넣어, 달리기의 지속 소모는 주기 효과(`SprintDrain`)로, PP 회복(그로기가 끝날 때, 페이즈 전환)은 PP를 최대로 덮어쓰는 효과(`PPRefill`)로, 패리로 공격자가 잃는 PP는 계산 클래스가 양을 정하는 효과(`ParryPPLoss`)로 처리합니다. 극한 회피와 패리의 보상은 피격 처리가 보상 효과(`PerfectDodgeReward`, `ParryReward`)로 줍니다. | 자원 변경은 GameplayEffect로 적용하고 고정 기획 값은 JSON을 사용합니다. 비용은 GAS의 CheckCost·ApplyCost 흐름을 따릅니다. 엔진의 비용 GE는 클래스(`CostGameplayEffectClass`)로만 받아 쓰지 않고, 비용과 얻기가 자원 효과 하나를 함께 씁니다. 결과를 정하는 피격 처리가 보상도 주면, 구간 닫기와 보상이 한 곳에서 전투 시스템의 순서대로 일어납니다. |
| 리젠 | SP·PP 리젠은 효과로 처리합니다. 리젠 대기는 태그를 주는 지속 효과(`SPRegenDelay`, `PPRegenDelay`)로, 자원이 줄 때마다 다시 걸어 처음부터 다시 셉니다. SP가 0이 될 때는 더 긴 대기 효과(`SPRegenDelayEmpty`)를 겁니다. 회복은 주기 효과로 하고, 대기 태그가 있는 동안에는 멈춥니다. SP는 평소용(`SPRegen`)과 가드 중용(`SPRegenGuarding`) 효과를 가드 태그 조건으로 바꿔 씁니다. 회복량은 효과 정의의 초당 값(`PerSecond`)이고, 로더가 주기(`Period`)를 곱해 한 주기 양으로 넣습니다. 회복 효과는 캐릭터 데이터의 시작 효과 목록(`Effects`)에 두어 캐릭터가 시작할 때 겁니다. | 자원 변화를 모두 GameplayEffect로 처리하는 GAS 표준 방식입니다. 초당 회복량과 대기 시간이 그 효과 정의에 있어 한곳에서 고칩니다. GameplayEffect 타이머는 게임 시간으로 흘러, 히트스톱 중에도 리젠이 계속되는 기획서 규칙과 맞습니다. |
| 그로기 면역 | 그로기 대상이 아닌 캐릭터(플레이어)는 시작 효과 목록(`Effects`)에 그로기 면역 효과(`GroggyImmune`)를 둡니다. 이 효과는 무한 지속으로 `State.GroggyImmune` 태그를 주고, PP가 줄어드는 곳(대미지 계산, 패리로 잃는 PP, 그로기 판정)은 캐릭터가 PP를 잃을 수 있는지(`CanLosePP`: 이 태그나 그로기 태그가 없음)를 봅니다. 그로기 어빌리티의 `ActivationBlockedTags`에도 이 태그를 넣어 직접 이벤트나 활성화 요청을 차단합니다. 플레이어의 PP·MaxPP는 미사용 값으로 0을 둡니다. | 상태를 태그로 나타내는 GAS 방식이라, 그로기 중에 PP를 잃지 않는 규칙과 같은 곳에서 확인합니다. 스탯에는 어트리뷰트 초기값만 둡니다. |

피격 반응은 Runner가 몽타주를 재생하고 그 재생이 끝날 때 끝납니다. 넉업의 상승·낙하·착지·기상은 넉업 몽타주의 섹션과 Transition이 잇습니다. 넉백·넉업의 무적은 시간표의 HoldTag 지점(쓰러짐 시각)부터 행동이 끝날 때까지입니다.

## 클래스 구조

### 캐릭터

플레이어·보스 캐릭터와 컴포넌트, 보스를 움직이는 AI 컨트롤러입니다.

| 클래스 | 부모 | 역할 |
| - | - | - |
| `AAgCharacterBase` | `ACharacter`, `IAbilitySystemInterface`, `IGenericTeamAgentInterface` | 플레이어·보스 공통. ASC, 어트리뷰트, 팀(적대 판정), 어빌리티 부여와 스펙의 어빌리티 항목 찾기(`FindAbility`, `FindAbilitySpec`), 무기 리소스, 공중 상태(`State.Airborne`)와 이동 사건(`Event.Landed`, `Event.Apex`), 행동하지 않을 때의 자동 회전, 재생 중인 Runner의 대상과 간격으로 루트 모션 자르기(최소 간격), 상태 태그 처리(캐릭터 통과, 시간 정지 요청), 추적 구간의 대체 대상(`GetFallbackFacingTarget`), 이동 입력 방향(`GetMoveInputDirection`), 움찔, 히트스톱 중의 시간 배율, 사망 처리와 사망 모션 |
| `AAgPlayerCharacter` | `AAgCharacterBase` | 카메라와 락온 카메라, 입력 매핑, 입력 계층(행동 태그로 스펙 찾기, 트리거 태그, 이동 입력 태그, 선입력, 점프와 점프 모션) |
| `AAgBossCharacter` | `AAgCharacterBase` | 페이즈 같은 보스 상태, 자동 회전 대상(AI 대상), 그로기가 끝날 때의 PP 회복. 보스에 표시하는 월드 위치 HUD(락온 마커, 처형 안내) |
| `UAgAttributeSet` | `UAttributeSet` | 기획서 스탯 13개, 사망·그로기 판정 |

```mermaid
classDiagram
    direction TB
    class IAbilitySystemInterface {
        <<interface>>
    }
    class IGenericTeamAgentInterface {
        <<interface>>
    }
    class AAgCharacterBase {
        UAbilitySystemComponent AbilitySystemComponent
        UMotionWarpingComponent MotionWarping
        USceneComponent WeaponComponent
    }
    class AAgPlayerCharacter {
        <<BP_AgPlayer>>
        USpringArmComponent CameraBoom
        UCameraComponent FollowCamera
        UInputMappingContext InputMapping
        UInputAction MoveAction 등
    }
    class AAgBossCharacter {
        <<BP_AgBoss>>
        UWidgetComponent LockOnMarker
        UWidgetComponent ExecutionPrompt
    }
    class AAgBossAIController {
        UStateTreeAIComponent StateTreeComponent
    }
    ACharacter <|-- AAgCharacterBase
    IAbilitySystemInterface <|.. AAgCharacterBase
    IGenericTeamAgentInterface <|.. AAgCharacterBase
    AAgCharacterBase <|-- AAgPlayerCharacter
    AAgCharacterBase <|-- AAgBossCharacter

    AAgCharacterBase *-- UAgAttributeSet : AttributeSet
    UAttributeSet <|-- UAgAttributeSet
    AAgCharacterBase --> UAgCharacterData : CharacterData
    AAgBossCharacter ..> AAgBossAIController : AIControllerClass
    AAIController <|-- AAgBossAIController
    AAgBossAIController ..> AAgBossCharacter : GetBoss
```

### 어빌리티

Runner와 Task의 상세 규약은 [어빌리티와 AbilityTask](<어빌리티와 AbilityTask.md>)를 따릅니다. 시간표 판정은 [몽타주 시간표](<몽타주 시간표.md>)를 따릅니다.

| 클래스 | 부모 | 역할 |
| - | - | - |
| `UAgGameplayAbility` | `UGameplayAbility` | 모든 행동의 Runner. ID별 정의·실행 사본, 태그 조건·트리거·비용·쿨다운·재발동, 대상(Targeting), 몽타주 재생과 Task가 요청한 전환, 시간표 판정과 추적 구간, 발동·구간·지점 Task의 시작·종료, 효과 스펙(`MakeEffectSpec`) |
| `UAgAbilitySystemComponent` | `UAbilitySystemComponent` | 스펙 동적 태그에 따른 차단·취소 보완, 행동 태그로 스펙 찾기(`FindSpecByTag`), 애니메이팅 어빌리티를 기록하는 몽타주 재생(`PlayMontageWithBlendIn`), 효과 스펙(`MakeEffectSpec`) |
| `UAgAbilityTask_Attack` | `UAbilityTask` | 판정 범위(무기 스윕이나 공격 범위)에 닿는 적을 대상마다 한 번 칩니다. 이전 무기 위치·적중 기록·물리 이후 틱을 직접 소유하고 종료 시 정리 |
| `UAgAbilityTask_HoldTag` | `UAbilityTask` | 자기 태그 횟수 획득·해제 |
| `UAgAbilityTask_ApplyEffect` | `UAbilityTask` | 효과 데이터의 효과를 걸고, 구간 끝이나 행동 끝(bRemoveOnEnd)에 자기 효과만 지움 |
| `UAgAbilityTask_Face` | `UAbilityTask` | 행동 대상이나 이동 입력 방향으로 몸을 바로 돌림 |
| `UAgAbilityTask_Dash` | `UAbilityTask` | 일정 속도 루트 모션 소스 이동(대상 쪽 돌진과 유도, 아래로 떨어짐), 막힘·거리 검사, 멈추면 전환 요청 |
| `UAgAbilityTask_Launch` | `UAbilityTask` | 루트 모션 점프로 띄우기·도약, 착지하면 끝 |
| `UAgAbilityTask_DisableRootMotion` | `UAbilityTask` | 그 재생의 몽타주 루트 모션 끄기(인스턴스 카운터), 같은 재생에서 구간이 끝나면 되돌림 |
| `UAgAbilityTask_SpawnProjectile` | `UAbilityTask_SpawnActor` | 발사 설정 사본과 실행 입력으로 한 발 생성·초기화 |
| `UAgAbilityTask_SendGameplayEvent` | `UAbilityTask` | 주인이나 행동 대상에게 게임플레이 이벤트 보내기 |
| `UAgAbilityTask_Transition` | `UAbilityTask` | 조건(태그, 이벤트, 효과가 끝나기 전)이 맞으면 Runner에 전환 요청 |
| `UAgAbilityTask_PlayCutscene` | `UAbilityTask` | 주인 기준으로 시퀀스 재생, 주인의 시간 배율로 재생 속도, 끝나면 전환 요청 |
| `UAgAbilityTask_CameraShake` | `UAbilityTask` | 로컬 플레이어가 범위 안이면 카메라 셰이크 |
| `UAgTargetingFilterTask_ActionTarget`, `UAgTargetingFilterTask_OnScreen`, `UAgTargetingSortTask_HorizontalDistance`, `UAgTargetingSelectionTask_AITarget` | 엔진 타겟팅 Task | TargetingPreset 안의 행동 대상 조건(적대·생존·범위·태그), 화면 안 조건, 수평 거리 정렬, 보스 AI 대상 선택 |

발사 후 투사체의 수명은 투사체가 맡습니다. 처형은 양쪽에 `State.Execution`을 사용하며 역할은 자기 행동과 전달된 사건으로 구분합니다.

### GameplayEffect

GameplayEffect는 `Effects.json`의 효과 정의이고, 에셋 매니저가 읽을 때 GE 객체로 만듭니다. GE C++ 클래스는 두지 않습니다. 형식, 효과 목록과 거는 곳, 코드가 효과를 거는 방법은 [JSON 데이터](<JSON 데이터.md>) '효과'를 따릅니다. 대미지 효과는 [대미지와 피격 처리](<대미지와 피격 처리.md>)에 있습니다. 행동 비용·쿨다운은 Runner가, 행동이 거는 효과(패리 구간, 극한 회피 구간, 그로기 시간, 달리기 소모)는 ApplyEffect가, 패리·극한 회피의 결과와 보상(패리로 잃는 PP, 보상, 환급, 회피 반격 기회)·히트스톱·적중 충전은 피격 결과 처리 컴포넌트가, PP 회복은 보스 캐릭터(그로기가 끝날 때)와 보스 AI(페이즈 전환이 끝날 때)가, 리젠 대기와 시작 효과는 캐릭터가 겁니다. 로직이 있는 계산만 C++ 클래스로 두고, 효과 정의가 클래스 경로로 가리킵니다.

| 클래스 | 부모 | 역할 |
| - | - | - |
| `UAgParryPPLossExecution` | `UGameplayEffectExecutionCalculation` | `ParryPPLoss` 효과의 계산. 공격한 쪽의 MaxPP × 막은 쪽의 패리 PP 비율(PP를 잃지 않는 대상은 그대로) |

```mermaid
classDiagram
    direction TB
    class UGameplayEffect {
        <<Effects.json 효과>>
    }
    UAgAssetManager ..> UGameplayEffect : GetEffect(EAgEffect)<br>FindEffect(효과 ID)
    UGameplayEffectExecutionCalculation <|-- UAgParryPPLossExecution
    UGameplayEffect --> UAgParryPPLossExecution : Executions(ParryPPLoss)
    UAgAbilitySystemComponent ..> UAgAssetManager : GetEffect
    UAgGameplayAbility ..> UAgAbilitySystemComponent : MakeEffectSpec(비용, 쿨다운, ApplyEffect)
    UAgHitResultGameplayEffectComponent ..> UAgAbilitySystemComponent : MakeEffectSpec(적중 충전, 환급, 패리)
    UAgHitResultGameplayEffectComponent ..> UGameplayEffect : 히트스톱, 보상, 회피 반격 기회
    AAgBossCharacter ..> UGameplayEffect : PPRefill(그로기 끝)
    AAgBossAIController ..> UGameplayEffect : PPRefill(페이즈 전환 끝)
    AAgCharacterBase ..> UGameplayEffect : 시작 효과, 리젠 대기, Kill
```

## 시간표와 Task 실행

몽타주 안의 구간과 지점은 재생을 시작한 Runner가 재생 위치로 판정해 Task를 시작·종료합니다([몽타주 시간표](<몽타주 시간표.md>)). 실행은 재생(몽타주 인스턴스 ID)과 항목으로 구분하므로 이전 재생의 늦은 구간 끝이나 전환 요청이 새 재생에 붙지 않습니다. 행동이 끝나면 엔진이 남은 Task를 끝내고, Runner가 켜진 구간과 워핑을 정리합니다. HoldTag는 자기 횟수만 제거하고, ApplyEffect는 자기 효과만 지웁니다.

Busy는 시간표의 HoldTag 구간(시작부터 후딜 시작까지)으로, 이동 입력 제한은 행동의 OwnedTags `State.Acting`으로 데이터에 적습니다. 그래서 후딜 동안에도 행동이 끝날 때까지 이동 입력이 막힙니다. 가드 시작의 이동 제한은 `State.ParryWindow`를 함께 봅니다([입력 처리](<입력 처리.md>)).

달리기의 SP 소모는 주기 효과(`SprintDrain`)를 ApplyEffect(bRemoveOnEnd)로 걸어 시작 시 즉시 틱하지 않고 주기 적용합니다. 한 주기 양은 초당 소모량 × 효과의 `Period`입니다. 입력 해제·소진·취소·사망에서 달리기가 끝나면 그 효과만 지웁니다.
