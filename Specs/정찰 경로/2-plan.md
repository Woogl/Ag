# 정찰 경로 기술 설계 (Plan)

## 1. 영향 범위 및 아키텍처

정찰 지점을 `ATargetPoint` 배열에서 스플라인 하나를 가진 정찰 경로 액터로 바꿉니다. 점 편집은 엔진 스플라인 편집을 그대로 쓰고, 지면 스냅·경로 표시·경고만 직접 만듭니다. 지금 `PatrolPoints`를 쓰는 레벨이 없어 옮길 배치는 없습니다.

| 대상 | 변경 |
| - | - |
| `AAgPatrolRoute` (신규, `Source/Ag/AI`) | 부모 `AActor`. 루트 `USplineComponent Spline`(닫힌 고리, 기본 점 2개는 직선 점, 내비게이션에 영향 없음)과 에디터 전용 `UAgPatrolPreviewComponent`. 게임에서는 점 개수와 월드 위치만 제공합니다. 에디터에서 지면 스냅, 점 위치 목록 설정, 경로 확인과 경고를 맡습니다. |
| `UAgPatrolPreviewComponent` (신규, `Source/Ag/AI`) | 부모 `UActorComponent`. 정찰 경로와 적의 에디터 전용 기본 서브오브젝트. 주인이 정찰 경로면 그 경로를, 적이면 그 적의 `PatrolRoute`와 처음 자리에서 가장 가까운 점까지의 구간을 표시용 데이터로 만듭니다. `UAgEnemySightPreviewComponent`와 같은 구성입니다. |
| `FAgPatrolVisualizer` (신규, `AgEditor`) | 부모 `FComponentVisualizer`. `UAgPatrolPreviewComponent`에 등록해 선(`DrawVisualization`)과 순서 번호(`DrawVisualizationHUD`)를 그립니다. |
| `AAgEnemyCharacter` | `PatrolPoints`를 `TObjectPtr<AAgPatrolRoute> PatrolRoute`(`EditInstanceOnly`)로 바꾸고, 에디터 전용 `PatrolPreview`를 더합니다. 에디터에서 경로 지정·이동 뒤 확인과 맵 검사(`CheckForErrors`)를 더합니다. |
| `FAgEnemyAITask_Patrol` | 점을 정찰 경로에서 읽고, 갈 수 없는 점을 건너뛰며, 한 바퀴 모두 실패하면 처음 자리로 돌아가 대기합니다. |
| `FAgEditorModule` | `FAgPatrolVisualizer`를 등록·해제합니다. |
| `Ag.Build.cs` | 경로 계산에 `NavigationSystem`을 비공개 의존성으로 더합니다. |

- 정찰 경로를 적의 컴포넌트가 아닌 레벨의 독립 액터로 둡니다. 여러 적이 함께 쓸 수 있고, 적을 옮겨도 경로가 그대로이며, 정찰하지 않는 적에는 아무것도 붙지 않습니다(Spec '정찰 경로 배치').
- 표시는 엔진 스플라인 선이 아닌 내비게이션 경로로 그립니다. 적은 점 위치로만 이동을 요청하고 내비게이션 경로를 따라 걸으므로, 스플라인의 직선은 실제로 걷는 길과 다릅니다. 엔진 스플라인 선은 경로를 선택했을 때 함께 보입니다.
- `USplineComponent`의 하위 클래스에 시각화를 등록하면 엔진 스플라인 편집 시각화가 가려지므로(엔진은 부모 클래스 쪽으로 처음 찾은 시각화 하나만 씁니다), 표시는 별도 미리 보기 컴포넌트에 둡니다.
- [기본 방침](<../../Docs/Architecture/기본 방침.md>): 게임 로직은 C++, 에디터 전용 코드는 `WITH_EDITOR`와 `AgEditor`에 둡니다. 기획 데이터(JSON)는 바뀌지 않습니다. 정찰 위치는 레벨 배치 데이터라 지금처럼 레벨에 둡니다.

## 2. GAS 및 데이터 설계

- GAS, 태그, JSON 변경은 없습니다.
- 적의 MOV는 에디터에서 `UAgAssetManager::FindCharacterData(CharacterDataId)`의 `Attributes.MOV`로 읽습니다(시야 미리 보기와 같은 방식).

### 지면 스냅 (`AAgPatrolRoute`, 에디터 월드에서만)

- `OnConstruction`에서 점마다 그 수평 위치에 위아래 1km 선을 긋고(`LineTraceMultiByObjectType`, `WorldStatic`), 맞은 면 가운데 걸을 수 있는 경사(법선 Z ≥ 엔진 기본 걸을 수 있는 경사 44.765°의 코사인)이고 볼륨(`AVolume`)이 아니며 캐릭터를 막는 면을 모읍니다. 위에서 그은 선은 맞은 컴포넌트를 건너뛰며 내려가므로 한 메시 안의 아래층을 찾지 못해, 점 바로 위(1m)에서도 한 번 긋습니다. 그중 기본 내비게이션 데이터에 투영되는(수평 10cm, 수직 50cm) 면만 남기고(남는 면이 없거나 내비게이션이 없으면 전부), 점 높이와 가장 가까운 면으로 옮깁니다. 바닥이 없으면 그대로 둡니다. LV_BossStage 아레나 바닥 아래에 내비게이션이 없는 구조물 윗면(Z −200)이 있어, 높이 0으로 넣은 점이 그 면에 붙던 문제를 내비게이션 우선으로 막습니다.
- `OnConstruction`은 경로를 놓을 때, 경로를 옮길 때(끄는 동안 포함), 점 끌기를 마쳤을 때(엔진이 `PostEditMove(true)`로 다시 실행), 디테일 패널에서 스플라인 값을 바꿨을 때 실행됩니다. 점을 끄는 동안에는 실행되지 않아, 놓는 순간 바닥에 붙습니다.
- 게임 월드(시뮬레이션·PIE·패키지)에서는 스냅하지 않습니다.

### 경로 확인 (`AAgPatrolRoute`, 에디터 전용 정적 함수)

- 두 위치 사이의 결과를 `걸을 수 있음(경로 점 목록)`, `갈 수 없음`, `알 수 없음` 셋으로 돌려줍니다. 월드의 내비게이션 시스템과 기본 내비게이션 데이터로 `FindPathSync`를 부르고, 성공했고 부분 경로가 아니면 걸을 수 있음입니다. 내비게이션 시스템이나 데이터가 없으면 알 수 없음입니다.
- 시각화가 선택된 동안 매 프레임 이 함수를 부릅니다. 구간이 수 개이고 에디터에서 선택했을 때만이라 결과를 저장해 두지 않습니다.

### 경고

| 확인 | 위치 | 시점 |
| - | - | - |
| 점 아래에 바닥이 없음 | `AAgPatrolRoute` | 점 끌기·경로 이동을 마쳤을 때(`PostEditMove(true)`), 점 위치 목록 설정, 맵 검사 |
| 점 사이를 걸어서 갈 수 없음 (마지막 → 첫 점 포함) | `AAgPatrolRoute` | 위와 같음 |
| 처음 자리에서 가장 가까운 점까지 걸어서 갈 수 없음 | `AAgEnemyCharacter` | `PatrolRoute`를 바꿨을 때, 적 이동을 마쳤을 때, 맵 검사 |
| MOV가 0인데 정찰 경로가 있음 | `AAgEnemyCharacter` | 위와 같음 |

- 메시지는 `FMessageLog("MapCheck")`에 액터 링크(`FUObjectToken`)와 함께 남깁니다. 편집 중 확인은 새 페이지를 열고 경고가 있을 때만 알림을 띄웁니다. 맵 검사는 엔진이 페이지를 관리합니다.
- 경로를 쓰는 적이 여럿이면 경로 쪽 경고는 경로에서 한 번만 남깁니다.
- 점 삭제는 엔진이 액터 쪽 알림을 보내지 않아 편집 중 경고가 없습니다. 뷰포트 표시와 맵 검사로 확인합니다.
- 메시지는 다른 로그와 같이 영어로 씁니다. 점 번호는 화면 번호와 같게 1부터 셉니다.

### AI가 쓰는 함수

- `AAgPatrolRoute::SetPointLocations(const TArray<FVector>& WorldLocations)`: 에디터 전용 `UFUNCTION(BlueprintCallable)`. 점을 모두 바꾸고(직선 점), 스냅과 확인을 실행합니다. 파이썬에서 `route.set_point_locations([...])` 한 번으로 경로를 넣습니다. 엔진 `set_spline_points`는 에디터 알림이 없어 스냅이 실행되지 않습니다.

### 표시 (`FAgPatrolVisualizer`)

| 표시 | 방법 |
| - | - |
| 경로 선 | 걸을 수 있는 구간은 내비게이션 경로 점을 초록색 선으로, 갈 수 없는 구간은 붉은 직선으로, 알 수 없는 구간은 흐린 회색 직선으로 잇습니다. `SDPG_Foreground`로 그려 지형에 가려지지 않습니다. 시야 부채꼴(청록색)과 구분됩니다. |
| 처음 자리 → 가장 가까운 점 | 적을 선택했을 때만, 같은 색 규칙의 선으로 그립니다. 가장 가까운 점은 게임과 같은 수평 거리로 고릅니다. |
| 순서 번호 | `DrawVisualizationHUD`에서 점 위치를 화면에 투영해 1부터 씁니다. 엔진 큰 글꼴을 1.6배로 키우고 그림자를 넣어, 점 표시의 오른쪽 위에 씁니다. |

- 경로 액터나 적을 선택하면 그리고, 에디터 월드가 아니면 그리지 않습니다(시야 미리 보기와 같음).

### 정찰 태스크 (`FAgEnemyAITask_Patrol`)

- 점 위치는 `AAgPatrolRoute`에서 읽습니다. 경로가 없으면 점 0개로 봅니다. 처음 갈 점은 `FindNearestPoint`로 지금과 같이 수평 거리로 고릅니다.
- 이동 요청은 `MoveToLocation`에 부분 경로를 허용하지 않고(`bAllowPartialPath = false`) 보냅니다. 요청이 실패하면 그 점을 갈 수 없는 점으로 보고 경고 로그를 남긴 뒤, 같은 틱에 다음 점을 요청합니다.
- 연달아 실패한 점 수가 경로의 점 수가 되면 처음 자리로 이동을 요청합니다. 이것도 실패하면 그 자리에서 처음 방향을 바라봅니다. 그 뒤로는 태스크가 다시 시작될 때(다음 비전투 복귀)까지 이동을 요청하지 않습니다.
- 한 점에 도착하면 연속 실패 수를 0으로 되돌립니다.
- 지금처럼 처음 자리로 돌아갈 때도 부분 경로를 허용하지 않습니다. Spec이 처음 자리에 갈 수 없으면 그 자리에서 대기하게 하기 때문입니다.

## 3. 인터페이스 & 함수 시그니처

```cpp
// Source/Ag/AI/AgPatrolRoute.h
UCLASS()
class AAgPatrolRoute : public AActor
{
	GENERATED_BODY()

public:
	AAgPatrolRoute();

	int32 GetPointCount() const;
	FVector GetPointLocation(int32 Index) const;
	/** 수평 거리로 가장 가까운 점. 게임의 정찰 시작과 에디터 표시·경고가 함께 씁니다. */
	int32 FindNearestPoint(const FVector& Location) const;

	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	static EAgPatrolPathResult FindWalkPath(UWorld& World, const FVector& From, const FVector& To, TArray<FVector>& OutPath);
	static TSharedRef<FTokenizedMessage> MakeWarning(const UObject& Object, const FString& Text);
	static void ReportEditProblems(const TArray<TSharedRef<FTokenizedMessage>>& Messages);

	UFUNCTION(BlueprintCallable, Category = "Patrol")
	void SetPointLocations(const TArray<FVector>& WorldLocations);

	virtual void PostEditMove(bool bFinished) override;
	virtual void CheckForErrors() override;
#endif

private:
#if WITH_EDITOR
	void SnapPointsToGround();
	void CollectProblems(TArray<TSharedRef<FTokenizedMessage>>& OutMessages) const;
#endif

	UPROPERTY(VisibleAnywhere, Category = "Patrol")
	TObjectPtr<USplineComponent> Spline;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UAgPatrolPreviewComponent> PatrolPreview;
#endif
};

// Source/Ag/AI/AgPatrolPreviewComponent.h
enum class EAgPatrolPathResult : uint8 { Walkable, Unreachable, Unknown }; // AgPatrolRoute.h, 에디터 전용

struct FAgPatrolPreviewSegment
{
	TArray<FVector> Path;
	EAgPatrolPathResult Result;
};

UCLASS()
class UAgPatrolPreviewComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAgPatrolPreviewComponent();

#if WITH_EDITOR
	/** 번호를 그리는 HUD는 경로 계산이 필요 없어 점과 구간을 따로 모읍니다. */
	AG_API bool GetPreviewPoints(TArray<FVector>& OutPoints) const;
	AG_API void GetPreviewSegments(TArray<FAgPatrolPreviewSegment>& OutSegments) const;
#endif
};

// AAgEnemyCharacter
AAgPatrolRoute* GetPatrolRoute() const;
#if WITH_EDITOR
virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
virtual void PostEditMove(bool bFinished) override;
virtual void CheckForErrors() override;
void CollectPatrolProblems(TArray<TSharedRef<FTokenizedMessage>>& OutMessages) const; // private
#endif
UPROPERTY(EditInstanceOnly, Category = "AI")
TObjectPtr<AAgPatrolRoute> PatrolRoute;

// FAgEnemyAITask_PatrolInstanceData
/** 도착한 뒤로 연달아 이동 요청에 실패한 정찰 지점 수. 처음 자리로 가면 PointIndex가 INDEX_NONE이 되어 다시 요청하지 않습니다. */
int32 FailedCount = 0;
```

## 4. 문서 반영 (4단계)

| 문서 | 반영 |
| - | - |
| [적 시스템](<../../Docs/Design/System/적 시스템.md>) '비전투와 정찰' | 갈 수 없는 정찰 지점의 처리 |
| [캐릭터 기반 구조와 GAS](<../../Docs/Architecture/캐릭터 기반 구조와 GAS.md>) | `AAgEnemyCharacter` 역할의 정찰 지점 문구, `AAgPatrolRoute` 행, 클래스 구조도의 `ATargetPoint : PatrolPoints` → `AAgPatrolRoute : PatrolRoute`와 미리 보기 컴포넌트 |
| [데이터 로딩과 스키마](<../../Docs/Architecture/데이터 로딩과 스키마.md>) 'JSON 외부 데이터 위치' | 정찰 지점 행: 레벨의 `AAgPatrolRoute`(스플라인 점)와 적 인스턴스의 `PatrolRoute` |
| [개발 도구와 검증](<../../Docs/Architecture/개발 도구와 검증.md>) | 기술 결정 '정찰 경로 미리 보기'(표시·스냅·경고·`SetPointLocations`), 클래스 구조의 `UAgPatrolPreviewComponent`·`FAgPatrolVisualizer` |
| ag-editor-workflow 스킬 | 정찰 경로를 파이썬으로 넣는 방법(`set_point_locations`)과 엔진 `set_spline_points`로는 스냅되지 않는 함정 |
