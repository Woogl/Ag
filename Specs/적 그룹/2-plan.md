# 적 그룹 기술 설계 (Plan)

## 1. 영향 범위 및 아키텍처

- `AAgEnemyCharacter`: 레벨 인스턴스 값 `GroupId`(`FName`, `EditInstanceOnly`). 같은 이름을 적은 적끼리 한 그룹이고, 비워 두면 그룹이 없습니다. 그룹 액터를 따로 두지 않아 기획자는 적에 이름만 적고, 적을 복사하면 그룹도 따라갑니다.
  - `BeginPlay`에 `UAgCombatSubsystem`의 그룹에 들고 `EndPlay`에 나가며, `Die` 처음에 그룹에 사망을 알립니다.
  - 맵 검사(`CheckForErrors`): 같은 `GroupId`를 가진 다른 적이 열린 레벨에 없으면 이름을 확인하라고 경고합니다(오타로 그룹이 갈리는 것을 잡음). 편집 중 값을 바꿀 때는 알리지 않습니다. 그룹의 첫 적에 이름을 넣는 순간 늘 경고가 떠서입니다.
- `UAgCombatSubsystem`: 그룹 이름별로 그룹원 목록과 '사망한 그룹원의 인식이 끝나는 시각'을 가집니다. 새 서브시스템을 만들지 않고 전투 명단을 가진 이 서브시스템에 둡니다(사용자 결정).
- `AAgEnemyAIController`: 지금의 `IsTargetNoticed` 본문을 `PerceivesTarget`(자기 감각만)으로 옮기고, `IsTargetNoticed`는 `GroupId`가 있으면 서브시스템에, 없으면 자기 감각에 묻습니다. StateTree 조건(`TargetNoticed`, `TargetLost`)과 `ST_Enemy`는 그대로입니다.
- 그룹원 서로의 인식은 각자의 AI 지각(시야·대미지 감각, 최대 나이 = `TargetLostTime`)을 그대로 OR 합니다. 그래서 '마지막으로 놓치거나 맞은 순간부터 5초'가 그룹 전체에 저절로 성립합니다. 엔진 팀 감각(`AISense_Team`)은 반경으로 퍼지고 벽을 무시해 쓰지 않습니다.
- 사망한 그룹원: 지각은 죽은 뒤에도 돌아가므로 셈에서 뺍니다. 사망하는 순간 자기 감각으로 인식 중이었다면 그룹이 '지금 + `TargetLostTime`'까지 인식을 유지합니다.

## 2. GAS 및 데이터 설계

- 태그, 어빌리티, 효과, JSON 변경 없음. 시야 상실 대기는 기존 `CombatRules.json` `TargetLostTime`(5초)을 그대로 씁니다.

## 3. 인터페이스 & 함수 시그니처

```cpp
// UAgCombatSubsystem
void AddGroupMember(AAgEnemyCharacter& Enemy);
void RemoveGroupMember(AAgEnemyCharacter& Enemy);
bool IsGroupNoticingTarget(FName GroupId) const;
void HandleGroupMemberDied(const AAgEnemyCharacter& Enemy);

struct FAgEnemyGroup
{
	TArray<TWeakObjectPtr<AAgEnemyCharacter>> Members;
	double DeadMemberNoticeEndTime = 0.0;
};
TMap<FName, FAgEnemyGroup> Groups;

// AAgEnemyAIController
bool IsTargetNoticed() const;   // 그룹 또는 자기 감각
bool PerceivesTarget() const;   // 자기 감각만

// AAgEnemyCharacter
FName GetGroupId() const;
UPROPERTY(EditInstanceOnly, Category = "AI") FName GroupId;
void CollectGroupProblems(TArray<TSharedRef<FTokenizedMessage>>& OutMessages) const;   // 에디터 전용
```

## 4. 알려진 한계

- 그룹 때문에 전투에 들어간 적도 대상은 기존 적 락온(`EnemyLockOn`, 범위 50m)으로 잡습니다. 플레이어와 50m보다 먼 그룹원은 전투 상태로 제자리에 있다가 50m 안에 들어오면 대상을 잡고 움직입니다.
- 월드 파티션 레벨에서 그룹원 일부가 에디터에 로드되지 않은 영역에 있으면, 맵 검사가 오타가 아닌데도 경고할 수 있습니다.
