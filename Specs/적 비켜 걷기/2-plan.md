# 적 비켜 걷기 기술 설계 (Plan)

## 1. 영향 범위 및 아키텍처

### 비켜 걷기는 엔진 Detour Crowd로 합니다

- `AAgEnemyAIController`의 경로 이동 컴포넌트를 엔진 `UCrowdFollowingComponent`로 바꿉니다. 생성자를 `FObjectInitializer`를 받는 형태로 바꾸고 `SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent"))`를 씁니다. 정찰, 처음 자리로 돌아가기, 다가가기는 지금도 경로 이동이라 이것만으로 다른 적을 비켜 걷습니다.
- Detour의 장애물 회피는 이웃 에이전트와 내비메시 경계를 함께 장애물로 넣습니다(`DetourCrowd.cpp`의 `DT_CROWD_OBSTACLE_AVOIDANCE`, 엔진 기본 켜짐). 그래서 비켜 걷는 중에도 내비메시 밖으로 나가지 않습니다.
- 엔진 RVO(이동 컴포넌트의 `bUseRVOAvoidance`)는 쓰지 않습니다. 내비메시 경계를 받는 통로(`UAvoidanceManager::SetNavEdgeProvider`)를 구현한 엔진 클래스가 없습니다. 또 회피 속도를 정하면 0.2초(`LockTimeAfterAvoid`) 동안 그 속도로 고정해, 그동안 AI가 입력을 멈춰도 계속 걷습니다. 이 이유는 컨트롤러 생성자에 한 줄 주석으로 남깁니다.

### 물러나기·좌우 걷기를 경로 이동으로 바꿉니다

군중 회피는 경로를 따라 걷는 중(`EPathFollowingStatus::Moving`)에만 속도를 고칩니다(`UCrowdFollowingComponent::ApplyCrowdAgentVelocity`). 지금의 이동 입력(`AddMovementInput`)으로 걷는 적은 다른 적을 비켜 가지 않으므로, `FAgEnemyAITask_KeepDistance`의 물러나기·좌우 걷기를 경로 이동으로 바꿉니다.

- 걷는 방향은 지금과 같습니다. 물러나기는 대상 반대쪽, 좌우 걷기는 적의 오른쪽 × `StrafeDirection`입니다.
- 매 틱, 가장 가까운 내비메시 점에서 걷는 방향으로 정한 거리만큼 `NavigationRaycast`를 합니다. 막히지 않으면 레이캐스트의 끝점을, 막히면 닿은 점을 목적지로 `MoveToLocation`을 요청합니다. 경로 탐색은 켜고, 목적지 투영과 부분 경로는 끄고, 대상을 바라본 채 걷기(`bCanStrafe`)를 켭니다.
- 닿은 점까지 걸어가 도착 반경 안에 들면 서므로, 벽이나 낭떠러지 앞에서 멈추는 자리는 지금과 같습니다. 직선으로 열린 점만 목적지가 되므로 경로가 벽을 돌아가지도 않습니다.
- 닿은 점이 10cm 안이면 요청하지 않고 섭니다. 끝에 비스듬히 닿으면 닿은 점이 걸을 때마다 옆으로 조금씩 밀려나, 그 점으로 걷기를 되풀이하며 끝을 따라 미끄러지기 때문입니다(첫 테스트에서 30° 비스듬히 물러난 적이 벽을 따라 56cm 미끄러짐).
- 매 틱 다시 요청해도 걸음은 끊기지 않습니다. 움직이는 중에 새 요청이 오면 엔진은 앞 요청을 `NewRequest`로 끝내되 멈추지 않습니다(`UPathFollowingComponent::RequestMove`의 `bStopMovementOnFinish = false`). 군중 쪽에서도 끊기지 않는지는 테스트로 확인합니다.
- 가장 가까운 내비메시 점에서 레이캐스트하는 이유(넉다운 등으로 내비메시를 조금 벗어난 적도 안쪽·옆으로 걷게)는 지금 주석 그대로 둡니다.

### 군중 설정

- 도착 감속(`SetCrowdSlowdownAtGoal`)을 끕니다. 물러나기·좌우 걷기의 목적지는 늘 가까이 있어, 켜 두면 계속 느리게 걷습니다. 다가가기와 정찰의 도착 감속도 함께 꺼지는데, 지금(군중 없는 경로 이동)과 같은지는 테스트에서 봅니다.
- 그 밖에는 엔진 기본값입니다.
- 크라우드 매니저의 엔진 기본값 `MaxAgentRadius` 100과 `MaxAgents` 50이 가장 큰 적(`BP_Boss02`)의 캡슐 반지름과 레벨당 적 수를 넘는지 확인합니다. 모자라면 `DefaultEngine.ini`의 `[/Script/AIModule.CrowdManager]`에서 올립니다.

### `FAgEnemyAITask_KeepDistance`의 상태 정리

- 다가가기 요청: 지금은 "다가가야 하고 멈춰 있으면 요청"입니다. 물러나기·좌우 걷기도 경로 이동 중이 되므로, "다가가기로 바뀐 확인 시점이거나, 다가가는 중에 멈춰 있으면 요청"으로 바꿉니다.
- 다가가기를 그만둘 때 경로를 끊는 것(`StopPathMove`)은 그대로 둡니다. 나갈 때는 다가가기뿐 아니라 어떤 걷기든 끊습니다.
- 공격 중 이동과 피격 반응: 패턴과 반응은 `KeepDistance`나 `Patrol`에서 나간 뒤에 시작하므로 경로가 이미 끊겨 있고, 군중은 그 동안 속도를 고치지 않습니다. Spec대로 비켜 가지 않습니다. 서 있거나 공격 중인 적도 군중 에이전트로 남아 있으므로, 다른 적은 그 적을 비켜 걷습니다.
- 플레이어는 군중 에이전트가 아니므로, 적이 플레이어를 비켜 가지는 않습니다. 지금과 같습니다.

### 문서

- [캐릭터 기반 구조와 GAS](<../../Docs/Architecture/캐릭터 기반 구조와 GAS.md>)에서 `AAgEnemyAIController` 행에 군중 경로 이동을, `FAgEnemyAITask_KeepDistance` 행에 물러나기·좌우 걷기의 경로 이동을, `AAgEnemyCharacter` 행의 "낭떠러지를 넘지 않는 것" 문장을 맞춥니다.

## 2. GAS 및 데이터 설계

- 태그, 어빌리티, JSON 변경은 없습니다.
- 레이캐스트 거리와 도착 반경은 `AgEnemyAITasksPrivate`의 상수입니다.

## 3. 인터페이스 & 함수 시그니처

```cpp
// AAgEnemyAIController
AAgEnemyAIController(const FObjectInitializer& ObjectInitializer);

// AgEnemyAITasksPrivate (DeltaTime 인자를 뺌)
void WalkInsideNavMesh(AAgEnemyAIController& Controller, AAgEnemyCharacter& Enemy, const FVector& Direction);
```
