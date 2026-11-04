# 적 AI 이동 규칙 정리 기술 설계 (Plan)

## 1. 영향 범위 및 아키텍처

설계는 세 가지로 정리됩니다.
- 패턴 후딜레이는 컨트롤러가 들고 있는 "끝나는 시각" 하나입니다. 걸고 지우는 것은 행동을 끝낸 Task가 합니다.
- 공격하지 않는 동안의 움직임과 "공격할 때가 됐나"는 Task 하나('적: 간격 지키기')가 정합니다.
- 그래서 `ST_Enemy`에서 상태가 줄어듭니다. '패턴 후딜레이' 상태는 '간격 지키기'가 되고, '다가가기'와 '움찔' 상태는 사라집니다.

### 지금 구조 (확인한 값)

| 상태 | Task | 나가는 전환 |
| - | - | - |
| 교전 | 없음 | 움찔 중 → 움찔, 피격 반응 중 → 피격 반응 (둘 다 Critical) |
| └ 패턴 후딜레이 (첫 자식) | `PostPatternDelay` | 페이즈 전환 준비 → 페이즈 전환, 기본 공격 시작 + 반응 준비 → 반응 패턴, 대상 놓침 → 비전투, 완료 → 패턴 실행 |
| └ 패턴 실행 | `RunPattern` | 성공 → 패턴 후딜레이, 실패 → 다가가기, 조준 대상 놓침 → 패턴 후딜레이 |
| └ 다가가기 | `Approach` (0.5초 뒤 완료) | 페이즈 전환 준비 → 페이즈 전환, 대상 놓침 → 비전투, 완료 → 패턴 실행 |
| └ 반응 패턴 | `RunPattern`(반응) | 완료 → 페이즈 전환 또는 패턴 실행 |
| └ 페이즈 전환 | `PhaseTransition` | 완료 → 패턴 후딜레이 |
| 피격 반응 | `WaitReaction` | 완료 → 패턴 후딜레이 |
| 움찔 | `WaitReaction` | 움찔 아닌 반응으로 바뀜 → 피격 반응, 완료 → 페이즈 전환 또는 패턴 실행 |

### 바뀐 뒤 구조

```
Root
├─ 비전투 ─ 정찰
├─ 전투                     (전투 Task: 들어갈 때 후딜레이를 검)
│   ├─ 교전                 피격 반응 중 → 피격 반응
│   │   ├─ 간격 지키기      (첫 자식) 완료 → 패턴 실행
│   │   ├─ 패턴 실행        완료·조준 대상 놓침 → 간격 지키기
│   │   ├─ 반응형 패턴      완료 → 페이즈 전환 또는 패턴 실행
│   │   └─ 페이즈 전환      완료 → 간격 지키기
│   └─ 피격 반응            완료 → 간격 지키기
├─ 플레이어 사망 대기
└─ 사망
```

| 상태 | Task | 나가는 전환 |
| - | - | - |
| 교전 | 없음 | 피격 반응 중 → 피격 반응 (Critical) |
| └ 간격 지키기 | `KeepDistance` | 페이즈 전환 준비 → 페이즈 전환 (High), 기본 공격 시작 + 반응 준비 → 반응형 패턴 (Medium), 대상 놓침 → 비전투, 완료 → 패턴 실행 |
| └ 패턴 실행 | `RunPattern` | 완료 → 간격 지키기, 조준 대상 놓침 → 간격 지키기 |
| └ 반응형 패턴 | `RunPattern`(반응형) | 완료 → 페이즈 전환 또는 패턴 실행 (그대로) |
| └ 페이즈 전환 | `PhaseTransition` | 완료 → 간격 지키기 (그대로) |
| 피격 반응 | `WaitReaction` | 완료 → 간격 지키기 (그대로) |

- 바뀌는 상태:
  - '패턴 후딜레이' 상태 객체를 그대로 두고 이름을 '간격 지키기'로, Task를 `KeepDistance`로 바꿉니다. 이 상태로 들어오던 전환(패턴 성공, 조준 대상 놓침, 페이즈 전환·피격 반응 완료)과 이 상태에서 나가던 전환은 상태 ID로 이어져 있어 그대로 남습니다.
  - '다가가기'와 '움찔' 상태를 지웁니다.
  - '패턴 실행'의 실패 전환은 완료 전환 하나로 합쳐 간격 지키기로 갑니다.
  - '교전'의 "움찔 중 → 움찔" 전환을 지웁니다.
  - '반응 패턴'의 이름을 '반응형 패턴'으로 바꿉니다.
- 움찔만 끝나면 피격 반응 → 간격 지키기로 갑니다. 이때 후딜레이가 없고 쓸 수 있는 패턴이 있으면, 간격 지키기가 첫 틱에 완료해 바로 공격합니다(지금처럼 움찔 직후 공격).
- '교전' 밖(비전투, 플레이어 사망 대기, 사망)과 그 전환은 그대로입니다.

### 후딜레이를 거는 곳 (Spec 2.3)

| 거는 때 | 거는 곳 |
| - | - |
| 전투에 들어갈 때 | `FAgEnemyAITask_Combat::EnterState` |
| 공격 패턴을 끝까지 마쳤을 때 | `RunPattern::Tick`. 어빌리티가 끝난 틱에 적이 피격 반응 중이 아니면 겁니다. 피격 반응으로 끊겼으면 그 반응이 맡습니다. |
| 조준을 취소할 때 | `RunPattern::ExitState`. 어빌리티가 아직 돌고 있어 트리가 끊는 경우입니다. 실제로는 조준 대상 놓침 전환뿐입니다. |
| 넉업·넉다운·그로기·처형 피격이 끝날 때 | `WaitReaction::ExitState`. 머무는 동안 움찔이 아닌 반응(피격 반응 중이면서 움찔 중이 아님)이 한 번이라도 있었으면 겁니다. |
| 페이즈 전환이 끝날 때 | `PhaseTransition::ExitState` |
| (지움) 반응형 패턴을 끝까지 마쳤을 때 | `RunPattern`(반응형)의 `Tick`. 어빌리티가 끝난 틱에 피격 반응 중이 아니면 지웁니다. |

### 바꾸는 코드

| 대상 | 변경 |
| - | - |
| `AAgEnemyAIController` | 후딜레이가 끝나는 게임 시각(`PostPatternDelayEndTime`, 월드 시간)과 `StartPostPatternDelay()`(캐릭터 데이터 `AI.PostPatternDelayTime` 범위에서 무작위로, 다시 걸면 덮어씀)·`ClearPostPatternDelay()`·`IsInPostPatternDelay()`를 둡니다. 월드 시간이라 일시정지·전체 시간 배율에 함께 멈추고 느려지며, 움찔 같은 상태와 관계없이 흐릅니다. |
| `FAgEnemyAITask_KeepDistance` (신규, '적: 간격 지키기') | Spec 2.2를 따릅니다. 속성은 없습니다. |
| └ 확인 (첫 틱, 후딜레이가 끝난 틱, 그 뒤 0.5초마다) | 후딜레이가 없고 지금 쓸 수 있는 패턴(`GatherPatterns`)이 있으면 완료해 패턴 실행으로 갑니다. 아니면 목표 거리를 정합니다. 후딜레이가 없고 `GatherReadyPatternRanges`가 비어 있지 않으면 공격할 차례이고, 그 거리들이 목표입니다. 아니면 `AI.PreferredDistance`가 목표입니다. 목표보다 멀거나, 공격할 차례인데 목표 안이면(시야 등이 안 맞음) `MoveToActor`를 요청하고, 그 밖이면 경로만 끊습니다. |
| └ 목표 거리가 여럿일 때 | 하나라도 안이면 '안'입니다. 모두 밖이면 대상 거리에서 가장 가까운 끝(어느 거리의 Min 또는 Max)을 목표로 삼아, 멀면 다가가고 가까우면 물러납니다. 같은 거리 차이면 다가가는 쪽입니다. 지금 데이터에서는 일어나지 않는 경우라, 결과가 하나로 정해지게만 합니다. |
| └ 매 틱 (다가가지 않을 때) | 대상이 목표 거리의 가까운 쪽보다 가까우면 반대쪽으로 이동 입력을 줍니다. 그 밖이면 `AI.WaitMovement`가 `Strafe`일 때 옆으로 입력하고, `Stand`면 입력하지 않습니다. 좌우 걷기 방향은 들어갈 때 한 번 정합니다. 목표 거리는 확인 때 정한 값을 씁니다. |
| └ 나갈 때 | 경로 이동 중이면 경로만 끊고 속도는 감속에 맡깁니다(지금 `Approach`와 같음). |
| `FAgEnemyAITask_RunPattern` | 반응형 패턴이 아니고 후딜레이가 걸려 있으면 실패합니다(Spec 2.1을 지키는 곳). 위 표대로 후딜레이를 걸고 지웁니다. |
| `FAgEnemyAITask_WaitReaction`, `FAgEnemyAITask_PhaseTransition`, `FAgEnemyAITask_Combat` | 위 표대로 후딜레이를 겁니다. |
| `FAgEnemyAICondition_ResponseReady` | 후딜레이가 걸려 있을 때만 참입니다. 지금은 '패턴 후딜레이' 상태에만 붙어 있어 그 상태가 곧 조건이었습니다. 이제 간격 지키기에는 후딜레이가 없을 때도 머무르므로, 조건이 직접 봅니다. |
| `AAgEnemyCharacter::GatherReadyPatternRanges()` (신규) | 가중치가 있는 패턴 중 현재 페이즈에서 쓰고 쿨다운이 끝난 것의 발동 가능 거리(`PatternSelection.Distance`)를 모읍니다. 거리·대상 찾기는 보지 않습니다. 쿨다운은 어빌리티의 `CheckCooldown`을 씁니다. |
| `FAgEnemyAITask_PostPatternDelay`, `FAgEnemyAITask_Approach`, `FAgEnemyAICondition_Flinching` | 삭제합니다. 셋 다 `ST_Enemy`만 씁니다(에셋 분석기로 확인). `IsEnemyFlinching`은 `WaitReaction`이 계속 씁니다. |
| `FAgEnemyAIData` | `RetreatDistance`를 `PreferredDistance`(`FFloatInterval`, cm, 가까운 쪽~먼 쪽)로 바꿉니다. 팀·강인도처럼 모든 적이 적는 값이며, 빠뜨리면 0~0이라 늘 다가가는 것으로 바로 드러납니다. |
| `EAgPostPatternDelayMovement` → `EAgWaitMovement`, `AI.PostPatternDelayMovement` → `AI.WaitMovement` (이름 변경) | 후딜레이 밖(쿨다운을 기다릴 때)에서도 쓰는 값이라 이름을 바꿉니다. 값(`Strafe`, `Stand`)과 기본값은 그대로입니다. |
| 이름 '반응 패턴' → '반응형 패턴' | `ST_Enemy` 상태 이름, `FAgEnemyAICondition_ResponseReady`의 표시 이름('적: 반응형 패턴 발동 조건'), `AgEnemyCharacter.h`·`AgEnemyAIConditions.cpp`의 주석. 기획서는 Backstep을 '회피 패턴'으로 불러 그대로입니다. |

- 비전투 복귀(대상 놓침 전환)는 간격 지키기에 남아 있어 Spec 2.4와 같습니다.
- 거리 판단은 지금처럼 캡슐 중심 수평 거리(`GetHorizontalDistanceTo`)이고, 0.5초 간격은 지금 `Approach`의 `RetryInterval`을 그대로 씁니다.
- 간격 지키기는 쓸 수 있는 패턴이 생길 때만 완료합니다. 그래서 지금처럼 0.5초마다 패턴 실행과 다가가기를 오가지 않고, 좌우 걷기 방향과 다가가기 경로가 이어집니다.
- [기본 방침](<../../Docs/Architecture/기본 방침.md>): Task 둘과 조건 하나가 줄고, 새 태그와 효과는 없습니다.
  - 후딜레이 시각을 GAS 효과로 두지 않는 이유: 반응형 패턴은 후딜레이 중에 써야 해서 어빌리티 발동 차단으로는 막을 수 없고, 고르는 쪽(AI)만 보는 값입니다.
  - 후딜레이를 트리 상태가 아니라 Task가 거는 이유: 피격 반응 하나가 움찔과 다른 반응을 함께 다루려면, 끝난 뒤를 상태로 나누는 대신 반응을 지켜본 Task가 정해야 합니다.

## 2. GAS 및 데이터 설계

GameplayAbility·GameplayEffect·태그는 바꾸지 않습니다.

| 파일 | 변경 |
| - | - |
| `Characters.json` 모든 적 | `AI.PreferredDistance`를 적습니다: `Boss` 0~1000, `Cutter` 200~400, `Gunner` 800~3000, `Sandbag`·`AttackSandbag` 0~150. `Gunner`의 `AI.RetreatDistance`는 지웁니다. |
| `Characters.json` `AI.WaitMovement` | `Gunner`(`AI.PostPatternDelayMovement`에서 이름만 바뀜)와 `Sandbag`·`AttackSandbag`에 `Stand`를 적습니다. `Boss`·`Cutter`는 기본값(`Strafe`)이라 적지 않습니다. |

## 3. 인터페이스 & 함수 시그니처

```cpp
// AI/AgEnemyAIController.h
void StartPostPatternDelay();
void ClearPostPatternDelay();
bool IsInPostPatternDelay() const;
// private: double PostPatternDelayEndTime = 0.0;

// AI/AgEnemyAITasks.h
USTRUCT()
struct FAgEnemyAITask_KeepDistanceInstanceData
{
	UPROPERTY(EditAnywhere, Category = "Context") TObjectPtr<AAgEnemyAIController> Controller = nullptr;
	float CheckElapsed = 0.f;
	float StrafeDirection = 1.f;
	bool bApproaching = false;
	bool bWasDelaying = false;
};
USTRUCT(meta = (DisplayName = "적: 간격 지키기", Category = "Ag|Enemy AI"))
struct FAgEnemyAITask_KeepDistance : public FStateTreeAITaskBase { EnterState / Tick / ExitState };
// FAgEnemyAITask_WaitReactionInstanceData: bool bHeavyReaction = false;  (움찔이 아닌 반응이 있었음)
// FAgEnemyAITask_KeepDistanceInstanceData에 확인 때 정한 목표 거리(FFloatInterval TargetRange)를 더합니다.

// Character/AgEnemyCharacter.h
/** 현재 페이즈에서 쓰고 쿨다운이 끝난 패턴의 발동 가능 거리를 모읍니다. 거리와 대상 찾기는 보지 않습니다. */
void GatherReadyPatternRanges(TArray<FFloatInterval>& OutRanges) const;

// Data/AgCharacterData.h
enum class EAgWaitMovement : uint8 { Strafe, Stand };
// FAgEnemyAIData:
//   EAgWaitMovement WaitMovement = EAgWaitMovement::Strafe;
//   /** 기다리는 동안 지키는 대상과의 거리(캡슐 중심 수평, cm)입니다. */
//   FFloatInterval PreferredDistance = FFloatInterval(0.f, 0.f);
//   (RetreatDistance 삭제)
```

## 4. 작업 순서

`ST_Enemy`가 옛 Task·조건 구조체를 참조하므로 순서를 지킵니다.
1. 새 Task와 코드를 더해 빌드합니다. 옛 Task·조건은 아직 둡니다.
2. 에셋을 고쳐 컴파일하고 저장합니다.
3. 옛 Task·조건을 지워 다시 빌드합니다.

에디터를 닫고 다시 켜는 일이 두 번 있습니다.

## 5. 4단계에서 고칠 문서

- 기획서
  - [적 시스템](<../../Docs/Design/System/적 시스템.md>): '전투 시작과 상태 전환'의 후딜레이 문장, '패턴 선택', '패턴 후딜레이', '비전투와 정찰'의 두 문장을 Spec 2.1~2.4로 고칩니다.
  - [커스터](<../../Docs/Design/Content/보스 캐릭터 - 커스터.md>): 적정 거리를 더하고, 그로기·처형 피격·페이즈 전환 뒤의 대기 문장과 Backstep 조건의 뜻을 고칩니다.
  - [절단자](<../../Docs/Design/Content/일반 적 - 절단자.md>), [경비 사수](<../../Docs/Design/Content/일반 적 - 경비 사수.md>), [샌드백](<../../Docs/Design/Content/샌드백.md>): 적정 거리를 더하고, 다가가기와 후딜레이 움직임 문장을 고칩니다.
- 구현 문서
  - [캐릭터 기반 구조와 GAS](<../../Docs/Architecture/캐릭터 기반 구조와 GAS.md>): 컨트롤러의 후딜레이, `KeepDistance` Task 줄, 반응 준비 조건을 고치고, 후딜레이를 거는 곳 표를 둡니다. '움찔 중' 조건 줄을 지웁니다.
  - [데이터 로딩과 스키마](<../../Docs/Architecture/데이터 로딩과 스키마.md>): `AI.WaitMovement`, `AI.PreferredDistance`, '어빌리티'의 `PatternSelection` 줄(반응형 패턴)을 고칩니다.
