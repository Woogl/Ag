# 캐릭터와 GAS

캐릭터와 GAS를 어떻게 구현할지 정합니다.

## 기술 결정

| 항목 | 결정 | 이유 |
| - | - | - |
| ASC 위치 | 캐릭터에 둡니다. | 싱글플레이라 PlayerState에 둘 이유가 없습니다. |
| 어트리뷰트 | 세트 하나에 기획서 스탯을 기획서 이름 그대로 둡니다. HP, MaxHP, SP, MaxSP, MP, MaxMP, UP, MaxUP, PP, MaxPP, ATK, DEF, MOV. 보스가 쓰지 않는 SP·MP·UP는 0입니다. 기본값이 바뀌기 전에 범위(0 ~ 최대값)로 고정합니다. | 기획서, 데이터 위치표, 코드의 이름이 같아 대조하기 쉽습니다. 최대값에서 리젠이 더해져도 기본값이 넘치지 않아야 다음 감소가 바로 보입니다. |
| 어빌리티 목록 | 캐릭터 JSON의 `Abilities` 항목마다 단일 `UAgGameplayAbility`를 독립 스펙으로 부여합니다. `GiveAbility` 전에 핸들과 ID를 연결하고 스펙의 동적 태그·이벤트를 설정합니다. `InstancedPerActor` 인스턴스는 자기 ID의 정의와 실행 사본을 사용합니다. | 같은 클래스라도 조건·이벤트·재발동·쿨다운·GE 출처가 행동마다 구분됩니다. |
| 어빌리티 구조 | 행동 전체를 나타내는 자식 Ability나 Task는 두지 않습니다. 공통 Ability는 모션·입력·착지·컷신·돌진 완료 등 구체 사건을 연결하고, 독립 기능은 Task·엔진 기능·일반 함수가 수행합니다. 기능별 책임과 종료 규약은 [AbilityTask 기반 어빌리티 설계안](<AbilityTask 기반 어빌리티 설계안.md>)을 따릅니다. EndPlay에서는 메시 해제 전에 모든 Ability를 취소합니다. | 기능별 획득과 복원이 함께 있고, 한 기능의 완료가 다른 기능이나 전체 Ability를 임의로 종료하지 않습니다. |
| 상태 | 4순위 상태(넉업, 넉백, 가드 밀림, 가드 브레이크), 그로기, 처형 피격, 페이즈 전환, 사망은 이벤트로 발동하는 어빌리티로 만들고, 상태 우선순위는 태그로 나타냅니다. 높은 상태는 낮은 상태를 취소하고, 낮은 상태는 높은 상태 중에 시작하지 못합니다. 4순위 상태의 어빌리티 태그는 한 부모 태그(`Ability.Reaction.HitReaction`) 아래에 두어, 높은 상태가 부모 태그 하나로 취소합니다. 어빌리티가 붙이는 상태 태그는, 어빌리티가 도는 내내 붙으면 ActivationOwnedTags에 두고, 일부 구간에만 붙으면(회피의 무적 구간, 가드 중 등) 태그 유지 태스크(`UAgAbilityTask_HoldTag`)로 붙입니다. | 행동과 같은 방식이라 규칙이 한곳에 모입니다. 두 방식 모두 어빌리티가 끝나면 엔진이 태그를 떼므로, 어빌리티마다 태그를 떼는 코드를 두지 않습니다. |
| 쿨다운 | 어빌리티(플레이어 행동, 보스 패턴)의 쿨다운은 쿨다운 GameplayEffect(`UAgEffect_Cooldown`) 하나로 셉니다. 태그 없이, 걸 때 넣은 출처(그 스펙의 어빌리티 인스턴스)로 어빌리티를 구분하고, 시간은 어빌리티 항목의 `Cooldown`이며 0이면 쿨다운이 없습니다. 플레이어와 보스가 이 길 하나를 씁니다. 리젠 대기, 패리 구간(그동안 이동 제한)은 시간과 태그를 적용할 때 정하는 GameplayEffect 하나를 같이 씁니다. 비용(SP·MP·UP)은 '자원 변화'를 따릅니다. | GAS 기본 방식입니다. 값이 캐릭터·공통 데이터에 있어 GameplayEffect 에셋에 따로 둘 데이터가 없습니다. 어빌리티마다 태그를 두지 않아 어빌리티를 더해도 태그가 늘지 않습니다. 쿨다운 유무를 시간 하나로 정해, 쿨다운 GameplayEffect가 있는지와 시간이 어긋나지 않습니다. |
| 자원 변화 | 자원이 바뀌는 일마다 그 규칙을 가진 GameplayEffect를 적용합니다. 행동 비용은 어빌리티의 비용 GameplayEffect(`CostGameplayEffectClass`)로 엔진이 커밋할 때 치르고, 공통 Ability가 실행 사본의 Resource·Amount를 비용 Spec의 SetByCaller로 전달합니다. 치를 수 있는지는 같은 고정 비용과 지속 소모 시작 조건으로 확인합니다. MP·UP 충전과 극한 회피의 SP 반환은 얻는 양을 SetByCaller로 받는 자원 GameplayEffect로, PP 회복(그로기가 끝날 때, 페이즈 전환)은 PP를 최대로 덮어쓰는 GameplayEffect로, 패리로 공격자가 잃는 PP는 계산 클래스가 양을 정하는 GameplayEffect로 처리합니다. 극한 회피와 패리의 보상은 그 구간을 연 회피·가드 어빌리티가 그 이벤트를 받아 줍니다. | 자원 변경은 GameplayEffect로 적용하고 고정 기획 값은 JSON을 사용합니다. 비용은 GAS의 CheckCost·ApplyCost와 비용 GameplayEffect로 처리합니다. 보상을 구간을 연 어빌리티가 주면 극한 회피와 패리가 같은 방식이 됩니다. |
| 리젠 | SP·PP 리젠은 GameplayEffect로 처리합니다. 리젠 대기는 태그를 주는 지속 GameplayEffect로, 자원이 줄 때마다 다시 적용해 처음부터 다시 셉니다. SP가 0이 될 때는 더 긴 대기용 GameplayEffect를 적용합니다. 회복은 주기 GameplayEffect로 하고, 대기 태그가 있는 동안에는 멈춥니다. SP는 평소용과 가드 중용 주기 GameplayEffect를 가드 태그 조건으로 바꿔 씁니다. 회복 GameplayEffect의 크기 계산 클래스(`UAgRegenCalculation`)가 주기마다 주인의 캐릭터 데이터에서 초당 회복량을 읽어 주기를 곱하고, 주기는 회복 GameplayEffect의 `Period`입니다. 캐릭터마다 거는 회복 GameplayEffect는 캐릭터 JSON의 시작할 때 거는 GameplayEffect 목록(`Effects`)에 두고, 캐릭터가 시작할 때 겁니다. 대기 시간은 캐릭터·공통 데이터에서 읽어 적용합니다. | 자원 변화를 모두 GameplayEffect로 처리하는 GAS 표준 방식입니다. 회복량을 GameplayEffect가 주기마다 읽으므로, 데이터 파일을 다시 읽어도 효과를 다시 걸 필요가 없습니다. GameplayEffect 타이머는 게임 시간으로 흘러, 히트스톱 중에도 리젠이 계속되는 기획서 규칙과 맞습니다. |
| 그로기 면역 | 그로기 대상이 아닌 캐릭터(플레이어)는 시작할 때 거는 GameplayEffect 목록(`Effects`)에 그로기 면역 GameplayEffect(`UAgEffect_GroggyImmune`)를 둡니다. 이 GameplayEffect는 무한 지속으로 `State.GroggyImmune` 태그를 주고, PP가 줄어드는 곳(대미지 계산, 패리로 잃는 PP, 그로기 판정)은 캐릭터가 PP를 잃을 수 있는지(`CanLosePP`: 이 태그나 그로기 태그가 없음)를 봅니다. 그로기 어빌리티의 `ActivationBlockedTags`에도 이 태그를 넣어 직접 이벤트나 활성화 요청을 차단합니다. 플레이어의 PP·MaxPP는 미사용 값으로 0을 둡니다. | 상태를 태그로 나타내는 GAS 방식이라, 그로기 중에 PP를 잃지 않는 규칙과 같은 곳에서 확인합니다. 스탯에는 어트리뷰트 초기값만 둡니다. |

피격 반응 어빌리티는 엔진의 `PlayMontageAndWait`로 모션을 재생하고 완료·중단 시 종료합니다. 넉업의 상승·낙하·착지·기상 전환은 넉업 어빌리티에서 처리합니다. 넉백·넉업은 실제 소유 몽타주의 쓰러짐 알림을 받은 뒤 `HoldTag`로 어빌리티 종료까지 무적을 유지합니다.

## 클래스 구조

### 캐릭터

플레이어·보스 캐릭터와 컴포넌트, 보스를 움직이는 AI 컨트롤러입니다.

| 클래스 | 부모 | 역할 |
| - | - | - |
| `AAgCharacterBase` | `ACharacter`, `IAbilitySystemInterface`, `IGenericTeamAgentInterface` | 플레이어·보스 공통. ASC, 어트리뷰트, 팀(적대 판정), 어빌리티 부여와 스펙의 어빌리티 항목 찾기(`FindAbility`, `FindAbilitySpec`), 무기 리소스, 실행 소유자가 전달한 최종 이동 제약 적용(루트 모션 자르기·물리 이동·일반 회전 제한), 일반 회전, 공격 중 위치·회전 보정의 대상(`FindAttackTarget`)과 범위 밖 타의 회전 대상(`GetFallbackFacingTarget`), 움찔, 히트스톱 중의 시간 배율, 사망 처리 |
| `AAgPlayerCharacter` | `AAgCharacterBase` | 카메라, 락온, 대상 찾기, 입력 매핑, 입력을 어빌리티로 연결 |
| `AAgBossCharacter` | `AAgCharacterBase` | 페이즈 같은 보스 상태, 회전 대상(AI 대상)과 회전하지 않는 상태. 보스에 표시하는 월드 위치 HUD(락온 마커, 처형 안내) |
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

단일 Ability와 기능별 고정 Params의 상세 규약은 [AbilityTask 기반 어빌리티 설계안](<AbilityTask 기반 어빌리티 설계안.md>)을 따릅니다.

| 클래스 | 부모 | 역할 |
| - | - | - |
| `UAgGameplayAbility` | `UGameplayAbility` | ID별 정의·실행 사본, 비용·쿨다운, 모션 소유권과 사건 연결, 재생별 대상과 이동 제약, 처형 워핑 입력 |
| `UAgAbilitySystemComponent` | `UAbilitySystemComponent` | 스펙 동적 태그에 따른 차단·취소 보완 |
| `UAgAbilityTask_Attack` | `UAbilityTask` | 한 판정의 스윕 또는 명시적 대상 공격. 이전 무기 위치·적중 기록·물리 이후 틱을 직접 소유하고 종료 시 정리 |
| `UAgAbilityTask_Dash` | `UAbilityTask` | 독립 루트 모션 소스 이동·유도·막힘 검사와 자기 소스 제거 |
| `UAgAbilityTask_HoldTag` | `UAbilityTask` | 자기 태그 횟수 획득·해제 |
| `UAgAbilityTask_SetCollisionResponseToChannel` | `UAbilityTask` | 지정 컴포넌트·채널의 응답 저장·변경·복원 |
| `UAgAbilityTask_TimeStop` | `UAbilityTask` | 소유권 있는 시간 정지 요청·해제 |
| `UAgAbilityTask_LockOn` | `UAbilityTask` | 대상 조회·보관·상실 검사·락온 지점 계산·카메라 추적, 종료 시 대상과 회전 상태 정리 |
| `UAgAbilityTask_PlayCutscene` | `UAbilityTask` | 기준 액터의 변환에서 시퀀스 재생·종료·복원 |
| `UAgAbilityTask_SpawnProjectile` | `UAbilityTask_SpawnActor` | 발사 설정 사본과 실행 입력으로 한 발 생성·초기화 |

모션과 완료 대기, 이벤트·이동 모드·속도 변화·그로기 종료 대기는 엔진 Task를 사용합니다. 발사 후 투사체의 수명은 투사체가 맡습니다. 처형은 양쪽에 `State.Execution`을 사용하며 역할은 자기 행동과 전달된 사건으로 구분합니다.

### GameplayEffect

GameplayEffect는 모두 C++ 클래스입니다. 대미지 GameplayEffect는 [대미지와 피격 처리](<대미지와 피격 처리.md>)에 있습니다. 행동 비용은 어빌리티의 비용 GameplayEffect로 엔진이 치르고, 자원 얻기, PP 회복, 패리로 잃는 PP는 그 일을 하는 어빌리티나 결과 처리 컴포넌트가 적용합니다. 시간 태그 GameplayEffect는 그 클래스의 함수(`Apply`)로 적용하고, 리젠과 그로기 면역 GameplayEffect는 캐릭터가 시작할 때 캐릭터 데이터의 목록(`Effects`)대로 겁니다. 리젠의 회복량은 크기 계산 클래스가 주기마다 캐릭터 데이터에서 읽습니다.

| 클래스 | 부모 | 역할 |
| - | - | - |
| `UAgEffect_Resource` | `UGameplayEffect` | 즉시 자원 증감. SP·MP·UP의 변화량을 SetByCaller로 받습니다. 회복과 비용이 같은 Modifier 정의를 사용합니다. |
| `UAgEffect_Cost` | `UAgEffect_Resource` | 행동 비용. 전달받은 Cost를 Spec의 음수 SetByCaller 값으로 설정합니다. |
| `UAgEffect_PPRefill` | `UGameplayEffect` | PP를 최대로 채웁니다(그로기가 끝날 때, 페이즈 전환). |
| `UAgEffect_ParryPPLoss` | `UGameplayEffect` | 패리로 공격한 쪽이 잃는 PP. 막은 쪽의 가드 어빌리티가 공격한 쪽에 적용합니다. |
| `UAgParryPPLossExecution` | `UGameplayEffectExecutionCalculation` | 패리로 잃는 PP의 계산. 공격한 쪽의 MaxPP × 막은 쪽의 패리 PP 비율(PP를 잃지 않는 대상은 그대로) |
| `UAgEffect_TimedTag` | `UGameplayEffect` | 적용할 때 정한 시간 동안 태그를 줍니다(리젠 대기, 패리 구간과 그동안의 이동 제한, 회피 반격 기회, 히트스톱). 거는 함수가 시간과 태그를 정합니다. |
| `UAgEffect_Cooldown` | `UGameplayEffect` | 어빌리티(플레이어 행동, 보스 패턴)의 쿨다운. 태그 없이, 걸 때 넣은 출처(그 스펙의 어빌리티 인스턴스)로 어빌리티를 구분합니다. |
| `UAgEffect_Regen` | `UGameplayEffect` | 리젠 주기 GameplayEffect의 부모. SP 평소용, SP 가드 중용, PP용이 태그 조건과 읽는 초당 회복량만 다릅니다. 주기는 이 클래스의 `Period`입니다. |
| `UAgRegenCalculation` | `UGameplayModMagnitudeCalculation` | 리젠의 한 주기 양. 주인의 캐릭터 데이터에서 읽은 초당 회복량 × 주기 |
| `UAgEffect_GroggyImmune` | `UGameplayEffect` | 그로기 면역('그로기 면역'). 무한 지속으로 `State.GroggyImmune` 태그를 줍니다. |

```mermaid
classDiagram
    direction LR
    class UAgEffect_Resource {
        <<자원 얻기>>
    }
    class UAgEffect_Cost {
        <<행동 비용>>
    }
    class UAgEffect_PPRefill {
        <<PP 회복>>
    }
    class UAgEffect_ParryPPLoss {
        <<패리로 잃는 PP>>
    }
    class UAgEffect_TimedTag {
        <<리젠 대기, 패리 구간, 히트스톱>>
    }
    class UAgEffect_Cooldown {
        <<행동 쿨다운>>
    }
    class UAgEffect_Regen {
        <<리젠>>
    }
    class UAgEffect_SPRegen {
        <<SP 회복>>
    }
    class UAgEffect_SPRegenGuarding {
        <<가드 중 SP 회복>>
    }
    class UAgEffect_PPRegen {
        <<PP 리젠>>
    }
    class UAgEffect_ResourceDrain {
        <<달리기 SP 소모>>
    }
    class UAgEffect_GroggyImmune {
        <<그로기 면역>>
    }
    UGameplayEffect <|-- UAgEffect_Resource
    UAgEffect_Resource <|-- UAgEffect_Cost
    UGameplayEffect <|-- UAgEffect_PPRefill
    UGameplayEffect <|-- UAgEffect_ParryPPLoss
    UGameplayEffectExecutionCalculation <|-- UAgParryPPLossExecution
    UAgEffect_ParryPPLoss --> UAgParryPPLossExecution : Executions
    UGameplayEffect <|-- UAgEffect_TimedTag
    UGameplayEffect <|-- UAgEffect_Cooldown
    UGameplayEffect <|-- UAgEffect_Regen
    UAgEffect_Regen <|-- UAgEffect_SPRegen
    UAgEffect_Regen <|-- UAgEffect_SPRegenGuarding
    UAgEffect_Regen <|-- UAgEffect_PPRegen
    UGameplayEffect <|-- UAgEffect_ResourceDrain
    UGameplayEffect <|-- UAgEffect_GroggyImmune
    UGameplayModMagnitudeCalculation <|-- UAgRegenCalculation
    UAgEffect_Regen --> UAgRegenCalculation : 크기 계산
    UAgRegenCalculation ..> UAgEffect_Regen : GetRatePerSecond

    UAgGameplayAbility --> UAgEffect_Cost : CostGameplayEffectClass<br>(회피, 스킬, 궁극기)
    UAgGameplayAbility --> UAgEffect_Cost : SetCost(Spec, Cost)
    UAgEffect_Cooldown <.. UAgGameplayAbility : Apply
    UAgGameplayAbility ..> UAgEffect_Resource : 극한 회피 보상
    UAgGameplayAbility ..> UAgEffect_Resource : 패리 충전
    UAgGameplayAbility ..> UAgEffect_ParryPPLoss : 패리
    UAgGameplayAbility ..> UAgEffect_PPRefill : 그로기 끝
    UAgGameplayAbility ..> UAgEffect_PPRefill : 페이즈 전환
```

## 공용 몽타주 Task

공용 `UAgAnimNotifyState_AbilityTask`에는 `Task`와 기능별 `ParamsId`만 저장합니다. Notify는 엔진 몽타주 알림 델리게이트로 방송하고, 재생 소유 Ability가 실제 MontageInstanceID와 Notify 인덱스를 확인해 실행 기록을 먼저 등록한 뒤 기능 Task를 활성화합니다. ANS 에셋 객체에는 실행 상태를 두지 않습니다. 단일 블록인 Dash·TimeStop은 ParamsId를 지정하지 않습니다.

Native Branching Point가 구간의 Begin/End·반복·섹션 이동을 처리합니다. Ability는 엔진 재생 Task의 활성화가 끝난 뒤 알림을 구독하고 재생 시작과 정확히 같은 시각의 공용 ANS를 동기 전달해 첫 프레임 보호를 보장합니다. 모션 교체·종료에서 구독을 해제합니다. 재생 시각·에셋 시각을 바꾸지 않으며, 첫 엔진 Begin의 중복은 같은 실행 키로 제거합니다. 구간을 매 틱 검색하거나 JSON에 복제하지 않습니다.

일반 End는 자기 실행만 닫고, 모션 교체·취소·Ability 종료는 해당 재생의 남은 실행과 워핑 목표를 정리합니다. HoldTag는 자기 횟수만 제거합니다. 충돌은 Task가 저장한 채널 응답을 직접 복원하며 순차 실행 계약은 [AbilityTask 기반 어빌리티 설계안](<AbilityTask 기반 어빌리티 설계안.md>)의 ‘캐릭터와 실행 기능의 책임 경계’를 따릅니다. 시간 정지는 요청 소유자만 해제합니다. 적용하지 못한 Task는 응답을 복원하지 않습니다.

Busy는 모션 재생 전에 획득하며 RecoveryStarted와 종료에서 자기 횟수만 해제합니다. 이동 입력은 실제 행동 모션 기록을 보므로 후딜·자연 블렌드 아웃 동안에도 제한됩니다. 가드 이동 제한은 ParryWindow 태그를 함께 사용합니다.

Sprint의 `Tasks.DrainResource`는 공통 `UAgEffect_ResourceDrain`을 시작 시 즉시 틱하지 않고 주기 적용합니다. 입력 해제·소진·취소·사망은 자기 GE 핸들만 제거합니다. 빈 Tasks 자체는 종료 조건이 아닙니다.
