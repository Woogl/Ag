# 경비 사수 AI 변경 기술 설계 (Plan)

## 1. 영향 범위 및 아키텍처

적마다 다른 점은 `Characters.json`, 투사체마다 다른 점은 `SpawnActors.json`으로만 정합니다. `ST_Enemy` 에셋과 StateTree Task 구성은 바꾸지 않습니다. 새 Task 클래스, 태그, 효과는 없고, Aim Task와 어빌리티의 조준점은 지웁니다.

### 패턴 후딜레이 움직임 (Spec 2.2)

| 대상 | 변경 |
| - | - |
| `UAgCharacterData` | 적만 쓰는 값 두 개를 더합니다. `PostPatternDelayMovement`(`EAgPostPatternDelayMovement`: `Strafe` 기본, `Stand`)와 `RetreatDistance`(cm, 기본 0 = 물러나지 않음)입니다. |
| `FAgEnemyAITask_PostPatternDelay::Tick` | 매 틱 이동 입력을 고릅니다. 대상이 `RetreatDistance` 미만(캡슐 중심 수평 거리)이면 대상 반대쪽 수평 방향으로 입력합니다. 그 밖이면 `Strafe`는 지금처럼 옆으로 입력하고, `Stand`는 입력하지 않습니다. 몸은 지금처럼 컨트롤러가 대상 쪽으로 돌립니다. |

- 막힘 검사는 따로 하지 않습니다. 벽은 캡슐 충돌로 멈추고, 비스듬히 닿으면 엔진 이동이 벽을 따라 미끄러뜨립니다. 낭떠러지는 아래 '낭떠러지'의 엔진 설정이 막습니다.
- 모든 적이 같은 Task와 같은 규칙을 씁니다. 커스터·절단자·샌드백은 두 값의 기본값(`Strafe`, 0)이라 지금과 같습니다.

### 낭떠러지 (Spec 2.5)

| 대상 | 변경 |
| - | - |
| `AAgEnemyCharacter` 생성자 | 엔진 `UCharacterMovementComponent::bCanWalkOffLedges`를 false로 둡니다. 걷기 상태의 이동(이동 입력, 경로 따라가기, 몽타주 루트 모션)을 엔진이 가장자리에서 멈추고 가장자리를 따라 미끄러뜨립니다. 공중 상태(Launch의 넉업·저글, 도약 패턴)는 걷기가 아니라 영향이 없습니다. |
| 적 BP 기본값 | `BP_Boss`·`BP_Cutter`·`BP_Gunner`·샌드백 BP가 이 값을 덮어쓰지 않는지 에셋 분석기로 확인합니다. |

- 낭떠러지의 높이 기준은 엔진 이동의 바닥 찾기(`MaxStepHeight`)를 따르며, 값은 바꾸지 않습니다.

### 예측 조준 (Spec 2.3)

| 대상 | 변경 |
| - | - |
| `AAgProjectile` | `OnSpawnedByAbility`에서 어빌리티의 행동 대상을 기억합니다. `BeginPlay`에서 자기 위치에서 그 대상의 예측 지점을 향하도록 비행 방향과 회전을 정합니다. 행동 대상이 없으면 지금처럼 생성 방향으로 날아갑니다. `bHorizontal`(`EditInstanceOnly`, 기본 false)이 켜져 있으면 대상 높이를 자기 높이로 놓고 계산해 수평으로 날아갑니다. |
| 예측 계산 (`AgProjectile.cpp` 익명 함수) | `PredictLocation(From, Target, Speed)`는 대상 위치 + 대상 수평 속도 × τ를 돌려줍니다. τ는 "From에서 Speed로 곧게 날아가 대상에 닿는 시간"이며 이차식의 근으로 구합니다. 근이 없으면(대상이 투사체보다 빠름) 지금 거리 ÷ Speed를 씁니다. |
| `UAgAbilityTask_SpawnActor` | 조준점 분기를 지웁니다. 생성 방향은 늘 캐릭터 정면 수평입니다. 액터 종류를 모르는 구조는 그대로입니다. |
| `UAgAbilityTask_Aim`, `FAgAimParams` | 삭제합니다. |
| `UAgGameplayAbility` | 조준점(`AimLocation`)과 조회·설정, 발동 때 비우기를 지웁니다. |
| `FAgEnemyAICondition_AimLost` | 조준점 검사를 지웁니다. `State.Aiming` 동안 다음 중 하나면 참입니다. 대상이 없음, `IsVisible`(시야와 총구 → 대상 가림)이 거짓, 몸 → 총구 선이 WorldStatic에 막힘. 몸 → 총구 검사는 지금은 조준점이 생긴 뒤에만 하지만, 조준하는 내내 합니다. |

- 예측을 투사체가 하는 이유: 예측에는 투사체 속도가 필요한데, SpawnActor는 액터 종류를 모르는 범용 Task입니다. 그래서 속도를 가진 투사체가 생성되는 순간 스스로 방향을 정합니다. 충격파(`AAgShockwave`)는 겨누지 않으므로 영향이 없습니다.
- 대상은 어빌리티가 발동할 때 정한 행동 대상입니다(`PistolShot`은 `Shooting`, `SwordWave`는 `Enemy` 프리셋). 위치와 속도는 발사 순간의 값입니다.
- 위치를 `BeginPlay`에서 읽는 이유: 판정 모양과 루트가 BP 컴포넌트라 생성을 마친 뒤에야 자리가 정해집니다. `BeginPlay`는 `FinishSpawning` 안에서 불리므로 생성과 같은 프레임입니다.
- 반사(`Reflect`)는 지금처럼 쏜 적의 현재 위치를 향하며 예측하지 않습니다.

### 탄환 소멸 (Spec 2.4)

| 대상 | 변경 |
| - | - |
| `AAgProjectile` | `bDestroyOnHit`(`EditInstanceOnly`, 기본 false = 관통)와 `OnDamageApplied()`를 더합니다. 켜져 있으면 스스로 사라집니다. 반사된 뒤에도 같은 값을 씁니다. |
| `UAgDamageResultGameplayEffectComponent` | 결과가 적중·가드·가드 브레이크이고 원인이 투사체면, 처리 끝에서 `OnDamageApplied()`를 부릅니다. 패리의 `Reflect` 호출과 같은 자리입니다. 극한 회피와 무적은 그보다 앞에서 끝나므로 부르지 않습니다. |

- 투사체 Tick은 이미 적중 처리 뒤 `IsActorBeingDestroyed`를 확인하고 멈추므로, 같은 틱의 남은 대상은 맞지 않습니다.

## 2. GAS 및 데이터 설계

GameplayAbility·GameplayEffect·태그는 바꾸지 않습니다.

| 파일 | 변경 |
| - | - |
| `Characters.json` `Gunner` | `SightDistance` 3000, `SightAngle` 180, `PostPatternDelayMovement` `Stand`, `RetreatDistance` 800 |
| `Abilities.json` `PistolShot` | `Targeting.Range` 3000, `PatternSelection.Distance.Max` 3000. `Aim` 섹션의 Aim 항목을 지우고, RootMotion 0 ~ 0.85를 0 ~ 1.2로 늘려 발사 순간까지 플레이어를 바라보게 합니다. |
| `Abilities.json` `SwordWave` | 변경 없음 |
| `SpawnActors.json` `PistolBullet` | `Range` 10000, `bDestroyOnHit` true |
| `SpawnActors.json` `SwordWave` | `bHorizontal` true |

## 3. 인터페이스 & 함수 시그니처

```cpp
// Data/AgCharacterData.h
UENUM()
enum class EAgPostPatternDelayMovement : uint8
{
	Strafe,
	Stand
};

// UAgCharacterData
UPROPERTY(Transient, VisibleAnywhere, Category = "AI")
EAgPostPatternDelayMovement PostPatternDelayMovement = EAgPostPatternDelayMovement::Strafe;

/** 패턴 후딜레이 동안 대상이 이 거리(캡슐 중심 수평, cm) 안이면 뒤로 걷습니다. 0이면 물러나지 않습니다. */
UPROPERTY(Transient, VisibleAnywhere, Category = "AI")
float RetreatDistance = 0.f;

// Combat/AgProjectile.h
void OnDamageApplied();

UPROPERTY(EditInstanceOnly, Category = "Projectile")
bool bHorizontal = false;

UPROPERTY(EditInstanceOnly, Category = "Projectile")
bool bDestroyOnHit = false;

TWeakObjectPtr<AActor> AimTarget;

// Combat/AgProjectile.cpp (익명 네임스페이스)
FVector PredictLocation(const FVector& From, const AActor& Target, float Speed);
```

지우는 것은 다음과 같습니다.
- `AgAbilityTask_Aim.h`·`.cpp`
- `UAgGameplayAbility`의 `GetAimLocation`·`SetAimLocation`·`AimLocation`
- `UAgAbilityTask_SpawnActor`의 조준점 분기
- `FAgEnemyAICondition_AimLost`의 조준점 검사

## 4. 4단계에서 고칠 문서

- 기획서
  - [적 시스템](<../../Docs/Design/System/적 시스템.md>): '패턴 후딜레이'의 움직임을 고치고, '투사체 조준'과 낭떠러지 규칙을 더합니다.
  - [경비 사수](<../../Docs/Design/Content/일반 적 - 경비 사수.md>): 시야, 사거리, 후딜레이, 조준과 취소(0.35초 고정 삭제), 탄환을 고칩니다.
  - [커스터](<../../Docs/Design/Content/보스 캐릭터 - 커스터.md>): SwordWave 검기 방향, 후딜레이 이동(좌우 걷기)을 고칩니다.
  - [린](<../../Docs/Design/Content/플레이어 캐릭터 - 린.md>): '투사체 반사'의 관통 문장을 고칩니다.
- 구현 문서
  - [몽타주 타임라인과 구간 판정](<../../Docs/Architecture/몽타주 타임라인과 구간 판정.md>): Aim Task와 조준점 문단을 지우고, SpawnActor 설명과 클래스 구조를 고칩니다.
  - [데이터 로딩과 스키마](<../../Docs/Architecture/데이터 로딩과 스키마.md>): `UAgCharacterData`의 두 값, `AAgProjectile`의 `bHorizontal`·`bDestroyOnHit`와 예측 조준, SpawnActor 생성 방향을 고칩니다.
  - [캐릭터 기반 구조와 GAS](<../../Docs/Architecture/캐릭터 기반 구조와 GAS.md>): `AimLost`의 검사 항목과 적의 낭떠러지 설정을 고칩니다.

## 5. 참고

- `AgEnemyAITasks.cpp`, `AgProjectile.*`, `Abilities.json`, `SpawnActors.json`에는 커밋 전인 정찰 경로·GroundSlam 충격파 작업의 변경이 있습니다. 그 위에 이어서 고칩니다.
