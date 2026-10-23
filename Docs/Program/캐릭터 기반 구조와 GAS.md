# 캐릭터 기반 구조와 GAS

캐릭터 기반 구조와 GAS, 그리고 캐릭터가 레벨에 나타나 싸울 준비를 마치기까지(데이터, 빙의, 어빌리티 시스템, AI, 입력, HUD 연결)를 어떻게 구현할지 정합니다.

## 기술 결정

| 항목 | 결정 | 이유 |
| - | - | - |
| ASC 위치 | 캐릭터에 둡니다. | 싱글플레이라 PlayerState에 둘 이유가 없습니다. |
| 어트리뷰트 | 세트 하나에 기획서 스탯을 기획서 이름 그대로 둡니다. HP, MaxHP, SP, MaxSP, MP, MaxMP, UP, MaxUP, PP, MaxPP, ATK, DEF, MOV. 보스가 쓰지 않는 SP·MP·UP는 0입니다. 기본값이 바뀌기 전에 범위(0 ~ 최대값)로 고정합니다. PP를 더하기 모디파이어로 줄일 때는 `PreGameplayEffectExecute`에서 감소량의 절댓값을 반올림한 뒤 음수로 적용합니다. 반올림 규칙은 [전투 시스템](<../Design/전투 시스템.md>) '대미지'를 따릅니다. | 기획서, 데이터 위치표, 코드의 이름이 같아 대조하기 쉽습니다. 최대값에서 리젠이 더해져도 기본값이 넘치지 않아야 다음 감소가 바로 보입니다. PP 감소량을 적용 직전에 반올림하면 패리도 같은 규칙을 따르고, 리젠으로 생긴 남은 PP의 소수부는 보존됩니다. |
| 어빌리티 목록 | 캐릭터 데이터(`Characters.json`)의 `Abilities`에 적힌 어빌리티 ID마다 단일 `UAgGameplayAbility`를 독립 스펙으로 부여합니다. 정의는 `Abilities.json`에 있고, 행동 태그와 트리거는 스펙에 넣습니다. | 같은 클래스라도 조건·이벤트·재발동·쿨다운·GE 출처가 행동마다 구분됩니다. |
| 어빌리티 구조 | 모든 행동을 어빌리티 클래스 하나(`UAgGameplayAbility`)와 범용 Task로 실행하고, 행동이 아닌 규칙은 주인 객체가 맡습니다. | 구조와 이유는 [어빌리티와 AbilityTask](<어빌리티와 AbilityTask.md>) '결정과 목표'를 따릅니다. |
| 상태 | 4순위 상태(넉업과 저글, 넉다운, 가드 밀림, 가드 브레이크), 그로기, 처형 피격, 페이즈 전환은 이벤트로 발동하는 행동(GameplayEvent 트리거)으로 만들고, 상태 우선순위([전투 시스템](<../Design/전투 시스템.md>) '상태 우선순위')는 태그로 나타냅니다. 4순위 상태의 어빌리티 태그는 한 부모 태그(`Ability.Reaction.HitReaction`) 아래에 두어, 높은 상태가 부모 태그 하나로 취소합니다. 사망은 행동이 아니라 캐릭터가 사망 이벤트를 받아 처리합니다([사망 처리](<사망 처리.md>)). 그로기는 지속 효과가 상태 태그를 부여하며 효과 만료가 행동도 끝냅니다([보스 AI와 패턴](<보스 AI와 패턴.md>) '그로기와 처형 피격'). 그 밖에 행동이 붙이는 상태 태그는, 행동 내내 붙으면 ActivationOwnedTags에 두고, 일부 구간에만 붙으면(회피의 무적 구간, 후딜 전의 Busy 등) 시간표의 ApplyEffect 구간으로 그 태그를 주는 효과를 겁니다. | 행동과 같은 방식이라 규칙이 한곳에 모입니다. 두 방식 모두 행동이 끝나면 태그가 떨어지므로, 행동마다 태그를 떼는 코드를 두지 않습니다. |
| 쿨다운 | 어빌리티(플레이어 행동, 보스 패턴)의 쿨다운은 쿨다운 효과(`Cooldown`) 하나로 셉니다. 어빌리티마다 태그를 두지 않고, 효과의 에셋 태그(`Effect.Cooldown`)와 걸 때 넣은 출처(그 스펙의 어빌리티 인스턴스)로 어빌리티를 구분합니다. 시간은 어빌리티 항목의 `Cooldown`이며 0이면 쿨다운이 없습니다. 플레이어와 보스가 이 길 하나를 씁니다. | 엔진의 쿨다운 GE는 클래스(`CooldownGameplayEffectClass`)로만 받아, 코드에서 만든 GE 객체로 쿨다운을 확인·적용하는 함수를 재정의합니다. 시간은 어빌리티 정의에 두고, 공통 효과의 구성은 아래 '코드가 효과를 쓰는 방법'을 따릅니다. 어빌리티마다 태그를 두지 않아 어빌리티를 더해도 태그가 늘지 않습니다. 쿨다운 유무를 시간 하나로 정해, 쿨다운 효과가 있는지와 시간이 어긋나지 않습니다. |
| 자원 변화 | 자원이 바뀌는 일마다 그 규칙을 가진 효과를 적용합니다. 행동 비용은 GAS의 비용 흐름(`CheckCost`·`ApplyCost`)에서 자원 효과(`Resource`)를 음수로 걸어 치르고, 적중의 MP·UP 충전과 극한 회피의 SP 환급도 같은 자원 효과에 얻는 양을 넣습니다. 적중 보상의 중복 지급 방지는 [대미지와 피격 처리](<대미지와 피격 처리.md>)의 '적중 충전'을 따릅니다. 달리기의 지속 소모는 주기 효과(`SprintDrain`)로, PP 회복(그로기·처형 피격이 끝날 때, 페이즈 전환)은 PP를 최대로 덮어쓰는 효과(`PPRefill`)로, 패리로 공격자가 잃는 PP는 공격자의 MaxPP에 비율을 곱하는 효과(`ParryPPLoss`)로 처리합니다. | 자원 변경은 GameplayEffect로 적용하고 고정 기획 값은 JSON을 사용합니다. 엔진의 비용 GE는 클래스(`CostGameplayEffectClass`)로만 받아 쓰지 않고, 비용과 얻기가 자원 효과 하나를 함께 씁니다. |
| 리젠 | SP·PP 리젠은 자원마다 회복 효과 하나(`SPRegen`, `PPRegen`)로 처리합니다. 회복은 주기 효과입니다([데이터 로딩과 스키마](<데이터 로딩과 스키마.md>) '효과'). 리젠 대기는 태그를 주는 지속 효과(`SPRegenDelay`, `SPRegenDelayAtZero`, `PPRegenDelay`)이고, 회복 효과는 대기 태그가 있는 동안 멈춥니다. 회복 효과의 `RegenDelay`가 자원이 줄 때마다 대기 효과를 다시 걸어 처음부터 다시 세고, SP가 0이 되면 더 긴 대기 효과를 겁니다. 회복 효과는 캐릭터 데이터의 시작 효과 목록(`Effects`)에 두어 캐릭터가 시작할 때 겁니다. | 자원 변화를 모두 GameplayEffect로 처리하는 GAS 표준 방식입니다. 회복량, 대기 시간, 대기를 다시 거는 규칙이 효과 정의에 있어 한곳에서 고칩니다. GameplayEffect 타이머는 게임 시간으로 흘러, 히트스톱 중에도 리젠이 계속되는 기획서 규칙과 맞습니다. |
| 그로기 면역 | 그로기 대상이 아닌 캐릭터(플레이어)는 시작 효과 목록(`Effects`)에 그로기 면역 효과(`GroggyImmune`)를 둡니다. 이 효과는 무한 지속으로 `State.GroggyImmune` 태그를 줍니다. PP를 잃을 수 없는 캐릭터(`CanLosePP`가 거짓: `State.GroggyImmune`, `State.Groggy`, `State.Execution`, `State.Invincible` 중 하나가 있음)의 PP 감소는 어트리뷰트 세트가 기본값이 바뀌기 전에 막고, 그로기 판정도 같은 조건을 봅니다. 그로기 어빌리티의 `ActivationBlockedTags`에도 그로기 면역 태그를 넣어 직접 이벤트나 활성화 요청을 차단합니다. | 상태를 태그로 나타내는 GAS 방식이라, 그로기·처형·무적 중에 PP를 잃지 않는 규칙을 같은 곳에서 확인합니다. PP를 줄이는 효과(대미지, 패리로 잃는 PP)마다 조건을 두지 않아, 새 효과도 이 규칙을 따릅니다. 스탯에는 어트리뷰트 초기값만 둡니다. |

## 클래스 구조

### 캐릭터

플레이어·보스 캐릭터와 컴포넌트, 보스를 움직이는 AI 컨트롤러입니다.

| 클래스 | 부모 | 역할 |
| - | - | - |
| `AAgCharacterBase` | `ACharacter`, `IAbilitySystemInterface`, `IGenericTeamAgentInterface` | 플레이어·보스 공통. ASC, 어트리뷰트, 팀(적대 판정), 어빌리티 부여와 스펙의 어빌리티 항목 찾기(`FindAbility`, `FindAbilitySpec`), 무기 리소스, 공중 상태(`State.Airborne`)와 이동 사건(`Event.Landed`, `Event.Apex`), 재생 중인 어빌리티의 대상과 간격으로 루트 모션 자르기(최소 간격), 상태 태그 처리(캐릭터 통과, 시간 정지 요청, 극한 회피 구간의 회피를 시작한 자리 캡슐 `PerfectDodgeCapsule`), 락온 대상 조회(`GetLockOnTarget`), 이동 입력 방향(`GetMoveInputDirection`), 피격 흔들림 컴포넌트(`UAgHitPhysicsComponent`, 래그돌 전에 멈춤), 히트스톱 중의 시간 배율, 사망 처리와 사망 모션 |
| `UAgCharacterMovementComponent` | `UCharacterMovementComponent` | 플레이어와 보스의 기본 회전. 제자리 회전 중에는 애님 인스턴스가 계산한 회전량을 액터에 적용하며, 그 밖에는 엔진 회전을 사용합니다. 회전 제한과 방향 제공 방식은 [캐릭터 이동과 모션 제어](<캐릭터 이동과 모션 제어.md>)의 캐릭터 회전을 따릅니다. |
| `AAgPlayerCharacter` | `AAgCharacterBase` | 카메라와 락온 카메라, 입력 매핑, 입력 계층(행동 태그로 스펙 찾기, 트리거 태그, 이동 입력 태그, 선입력, 점프와 점프 모션) |
| `AAgBossCharacter` | `AAgCharacterBase` | 페이즈 같은 보스 상태, 피격 반응 뒤로 미룬 그로기의 시작, 그로기·처형 피격·페이즈 전환이 끝날 때의 PP 회복과 2페이즈 진입. 보스에 표시하는 월드 위치 HUD(락온 마커, 처형 안내) |
| `UAgAttributeSet` | `UAttributeSet` | 기획서 스탯 13개, PP 감소량 반올림과 감소 허용 조건 적용, 사망·그로기 판정 |

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

### 레벨 시작에서 BeginPlay까지

```mermaid
sequenceDiagram
    participant E as 엔진
    participant B as 보스
    participant AI as 보스 AI 컨트롤러
    participant GM as 보스전 게임 모드
    participant P as 플레이어
    participant H as HUD
    E->>B: 데이터 연결,<br>컴포넌트 초기화<br>(레벨에 놓인 보스)
    B->>AI: SpawnDefaultController<br>(AutoPossessAI)
    AI->>B: 빙의, PossessedBy:<br>ASC 액터 정보 갱신
    AI->>AI: OnPossess:<br>보스 캐릭터 기본값의<br>AIStateTree 시작
    B->>B: 무기 컴포넌트 찾기
    E->>GM: 플레이어 참가(PostLogin)
    GM->>P: 스폰(데이터 연결),<br>빙의
    P->>P: 무기 컴포넌트 찾기<br>PossessedBy: 입력 매핑,<br>ASC 액터 정보 갱신<br>SetupPlayerInputComponent
    GM->>H: ShowHUD
    H->>P: SP 변화 구독
    E->>B: BeginPlay
    E->>P: BeginPlay
    Note over AI: 첫 틱: 대상(플레이어)의<br>기본 공격 시작 이벤트 구독
```

### 캐릭터 BeginPlay

```mermaid
sequenceDiagram
    participant C as AAgCharacterBase
    participant ASC as 캐릭터 ASC
    participant D as 캐릭터 데이터
    participant GM as 보스전 게임 모드
    C->>ASC: InitAbilityActorInfo
    C->>ASC: MOV 변화, 히트스톱·캐릭터 통과·극한 회피 구간·시간 정지 태그, 사망 이벤트 구독
    C->>D: 스탯 초기값
    C->>ASC: 스탯 기본값(최대값 먼저), MOV 반영
    C->>D: 어빌리티 ID 목록<br>(정의는 Abilities.json)
    C->>ASC: 어빌리티 부여
    C->>D: 시작 효과 ID 목록<br>(정의는 Effects.json)
    C->>ASC: GameplayEffect 적용<br>(리젠, 그로기 면역)
    C->>C: 루트 모션 자르기 연결
    C->>GM: RegisterCombatant
    Note over C,GM: 플레이어: 어빌리티 시작·끝, 상태 태그 구독, 락온 회전 방식<br>보스: 그로기 태그, 어빌리티 끝 구독
```

### GameplayEffect

효과 정의의 JSON 형식은 [데이터 로딩과 스키마](<데이터 로딩과 스키마.md>) '효과'를 따릅니다. 이 절에서는 효과를 거는 객체의 역할과, 코드가 효과를 만들고 쓰는 방법(아래 '코드가 효과를 쓰는 방법')을 정리합니다. 대미지 효과는 [대미지와 피격 처리](<대미지와 피격 처리.md>)에 있습니다. 행동 비용·쿨다운은 어빌리티가, 행동이 거는 효과(패리 구간, 극한 회피 구간, 그로기 시간, 달리기 소모)는 ApplyEffect가, 패리·극한 회피의 결과와 보상(패리로 잃는 PP, 보상, 환급, 회피 반격 기회)·히트스톱·적중 충전은 피격 결과 처리 컴포넌트가, PP 회복(그로기·처형 피격·페이즈 전환이 끝날 때)은 보스 캐릭터가, 시작 효과와 Kill은 캐릭터가, 리젠 대기는 회복 효과의 리젠 대기 컴포넌트가, 슬로우모션은 슬로우모션 효과의 컴포넌트가 겁니다.

| 클래스 | 부모 | 역할 |
| - | - | - |
| `UAgRegenDelayGameplayEffectComponent` | `UGameplayEffectComponent` | 회복 효과의 `RegenDelay`([데이터 로딩과 스키마](<데이터 로딩과 스키마.md>) '효과'). 효과가 걸릴 때 회복하는 어트리뷰트의 변경을 구독하고 지워질 때 풉니다(엔진의 대상 태그 조건 컴포넌트가 태그 변경을 구독하는 방식). |
| `UAgSlowMotionGameplayEffectComponent` | `UGameplayEffectComponent` | 효과의 `SlowMotionSpeed`·`SlowMotionDuration`. 효과가 실행될 때 시간 서브시스템에 슬로우모션을 겁니다([전투 연출과 시간 제어](<전투 연출과 시간 제어.md>) '슬로우모션'). |

```mermaid
classDiagram
    direction TB
    class UGameplayEffect {
        <<코드 전용 효과와 JSON 효과>>
    }
    UAgAssetManager ..> UGameplayEffect : GetEffect(EAgEffect)<br>FindEffect(효과 ID)
    UGameplayEffectComponent <|-- UAgRegenDelayGameplayEffectComponent
    UGameplayEffect *-- UAgRegenDelayGameplayEffectComponent : GEComponents(RegenDelay)
    UAgRegenDelayGameplayEffectComponent ..> UGameplayEffect : 대기 효과
    UGameplayEffectComponent <|-- UAgSlowMotionGameplayEffectComponent
    UGameplayEffect *-- UAgSlowMotionGameplayEffectComponent : GEComponents(SlowMotionDuration)
    UAgAbilitySystemComponent ..> UAgAssetManager : GetEffect
    UAgGameplayAbility ..> UAgAbilitySystemComponent : MakeEffectSpec(비용, 쿨다운, ApplyEffect)
    UAgDamageResultGameplayEffectComponent ..> UAgAbilitySystemComponent : MakeEffectSpec(적중 충전, 환급, 패리)
    UAgDamageResultGameplayEffectComponent ..> UGameplayEffect : 히트스톱, 보상, 회피 반격 기회
    AAgBossCharacter ..> UGameplayEffect : PPRefill(그로기·처형 피격 끝, 페이즈 전환 끝)
    AAgCharacterBase ..> UGameplayEffect : 시작 효과, Kill
```

#### 코드가 효과를 쓰는 방법

- `EAgEffect`는 코드가 지정해 사용하는 효과(`Damage`, `Resource`, `Cooldown`, `Kill`, `Hitstop`, `PPRefill`)입니다. 에셋 매니저가 `StartInitialLoading`에서 값마다 `AgEffectData::MakeNativeEffect`로 한 번 만들어 `NativeEffects`에 UPROPERTY로 보관하며, JSON 재로딩 대상에서 제외합니다. 효과별 GE 클래스는 만들지 않습니다.
- `GetEffect(EAgEffect)`는 `NativeEffects`에서 찾습니다.
- `FindEffect(FName)`와 `DataId = "Effects"` 검증은 JSON 효과만 대상으로 합니다. 두 저장소는 서로 덮어쓰지 않습니다.
- 코드 전용 효과의 구성은 아래 표를 따릅니다. 구성 뒤 `OnGameplayEffectChanged`로 컴포넌트의 태그를 모읍니다. 적용할 때마다 새 스펙을 만들고, 스펙에만 공격 값·비용·쿨다운 시간·히트스톱 시간을 넣습니다.

| 효과 | 코드에서 고정하는 구성 |
| - | - |
| `Damage` | 즉시 효과. `Effect.Damage`, `State.Dead` 대상 제외, `UAgDamageExecution`과 `UAgDamageResultGameplayEffectComponent` |
| `Resource` | 즉시 효과. SP·MP·UP 각각에 대응하는 SetByCaller 값을 더함. 바꾸지 않는 자원도 스펙에 0을 넣음 |
| `Cooldown` | 지속 효과와 `Effect.Cooldown`. 지속 시간과 행동별 출처는 적용할 스펙에 지정 |
| `Kill` | 즉시 효과. HP를 0으로 덮어씀 |
| `Hitstop` | 지속 효과와 `State.Hitstop` 부여. 지속 시간은 `CombatRules.json`의 `HitstopDuration`을 적용할 스펙에 지정 |
| `PPRefill` | 즉시 효과. 대상의 MaxPP를 스냅샷 없이 읽어 PP에 덮어씀 |

- 걸 효과의 수와 종류를 데이터가 정하는 자리는 데이터의 효과 ID 목록으로 둡니다. 거는 때만 코드가 정합니다(플레이어 데이터의 `ParryAttackerEffects`, `PerfectDodgeEffects`). 그 때 발동하는 행동이 있으면 그 행동의 `ApplyEffect`로 겁니다(패리 보상은 Parry).
- 걸 효과를 GE 클래스로만 받는 엔진 기능(추가 효과 컴포넌트, 실행의 조건부 효과)은 쓰지 않고 효과 ID 필드로 대신합니다. 엔진의 다른 효과 지우기 컴포넌트는 자기 자신을 GE 클래스의 기본 객체와 비교해 가려 내 실행 중에 만든 GE에서는 자기 자신을 가려 내지 못하고, 스택 방식은 정하는 함수가 에디터 전용이라, 리젠 대기는 걸린 대기 효과를 태그로 지우고 다시 겁니다.
- 데이터가 거는 효과(행동 정의와 시간표의 ApplyEffect)는 효과 ID로 적습니다. 구간에 두는 효과는 `Infinite`입니다. 구간의 길이는 시간표가 정하기 때문입니다.
- 스펙은 GE 객체로 만듭니다(ASC와 어빌리티의 `MakeEffectSpec`). 넘길 값이 없는 효과는 ASC의 `ApplyGameplayEffectToSelf`에 GE 객체를 넘깁니다. 어빌리티의 `ApplyGameplayEffectToOwner`는 받은 GE의 클래스 기본 객체로 스펙을 만들어 쓰지 않습니다.
- 스펙에 값을 넣는 일은 그 값을 가진 구조체가 맡습니다(`AddToSpec`).
- 실행 중에 만든 GE는 모두 같은 클래스라, 걸린 효과는 엔진의 GE 클래스 기준 조회 대신 효과의 `AssetTags`나 부여 태그로 찾습니다.
