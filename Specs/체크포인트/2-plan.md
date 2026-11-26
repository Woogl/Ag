# 체크포인트 기술 설계 (Plan)

[1-spec](<1-spec.md>)의 체크포인트와 휴식을 구현합니다. 적을 처음 상태로 되돌리는 방법으로 적 스포너를 두고, 스포너가 생기는 김에 캐릭터 에셋을 비동기로 미리 읽게 합니다.

## 1. 영향 범위 및 아키텍처

### 1.1 방식

| 항목 | 결정 | 이유 |
| - | - | - |
| 적 되돌리기 | 값을 되돌리지 않고, 적을 지우고 다시 생성합니다. | 처음 상태(HP, PP, 그로기, 페이즈, 쿨다운, 자리, 방향, 비전투, AI 기억, 래그돌)를 항목마다 되돌리면 기능이 늘 때마다 목록을 고쳐야 합니다. 다시 생성하면 레벨을 처음 열 때와 같은 코드 경로를 탑니다. |
| 적 배치 | 레벨에는 적 대신 스포너(`AAgEnemySpawner`)를 놓습니다. 스포너가 적 클래스, 정찰 경로, 그룹 이름을 갖고, 스포너의 위치와 방향이 적의 처음 자리와 방향입니다. | 적 액터는 죽거나 움직이면 처음 배치 정보를 잃습니다. 배치 정보의 주인이 따로 있어야 다시 생성할 수 있고, 캐릭터가 생기기 전에 에셋을 읽을 주체도 됩니다. |
| 에셋을 읽는 때 | 스포너가 시작할 때 적 클래스와 그 종류의 어빌리티 에셋을 비동기로 읽고, 다 읽은 뒤 생성합니다. 캐릭터의 데이터 읽기(`LoadData`)는 지금 코드 그대로 두며, 에셋이 이미 메모리에 있어 멈추지 않습니다. | 캐릭터는 생성되는 순간 몽타주가 있어야 하므로 캐릭터 안에서는 기다릴 수 없습니다. 읽기 경로는 하나로 남아 '아직 읽히지 않은 에셋'을 다루는 코드가 생기지 않습니다. |
| 미리 읽을 에셋 찾기 | 그 캐릭터의 어빌리티 항목과 그 항목이 가리키는 생성 액터 정의에서 에셋 경로 문자열을 모읍니다. 데이터 구조체의 하드 참조는 바꾸지 않습니다. | 경로를 놓쳐도 `LoadData`가 그 자리에서 읽으므로 오류가 아니라 한 번의 멈춤으로 끝납니다. 소프트 참조나 종류별 수집 함수를 데이터마다 두지 않습니다. |
| 에셋을 붙잡는 주체 | 스포너가 적 클래스와 어빌리티 묶음(`UAgAbilitySet`)을 UPROPERTY로 붙잡습니다. | 적이 모두 죽어도 에셋이 내려가지 않아, 휴식할 때는 읽기 없이 같은 프레임에 다시 생성합니다. 스포너가 레벨과 함께 사라지면 지금처럼 GC가 내립니다. |
| 레벨 시작 | 스포너가 모두 생성을 마칠 때까지 게임 모드가 로딩 화면으로 가립니다. | 비동기라 레벨이 열린 뒤 몇 프레임에서 수백 ms 뒤에 적이 나타납니다. |
| 휴식의 주인 | `AAgCheckpoint::Interact`가 플레이어 회복(`AAgPlayerCharacter::Rest`)과 적 되돌리기(`AAgGameMode::RespawnEnemies`)를 차례로 부릅니다. | spec 2.2의 순서가 한 함수에 보입니다. 회복은 ASC와 인벤토리를 아는 플레이어가, 레벨 전체의 적은 게임 모드가 맡습니다. |
| 전투 기록 초기화 | `RespawnEnemies`는 모든 스포너의 적을 먼저 지운 뒤 모두 생성합니다. 그룹원이 모두 빠진 그룹은 전투 서브시스템이 지웁니다. | 그룹의 '죽은 그룹원 인식 유지 시각'이 남으면 새로 생성된 그룹이 곧바로 플레이어를 인식합니다. 두 단계로 나누면 모든 그룹이 한 번 비워집니다. |
| 스포너가 사라질 때 | 스포너는 `EndPlay`에서 읽기 요청을 취소하고 자기 적을 지웁니다. | 월드 파티션이 스포너를 내릴 때 적만 남지 않습니다. 지금 배치된 적이 셀과 함께 내려가는 것과 같은 결과입니다. |

### 1.2 클래스

| 클래스 | 변경 |
| - | - |
| `AAgEnemySpawner` (신규, `Game/`) | 블루프린트 없이 레벨에 바로 놓는 C++ 액터. 배치 값(`EnemyClass` 소프트 클래스, `PatrolRoute`, `GroupId`), 생성한 적, 붙잡은 클래스와 어빌리티 묶음, 읽기 핸들. 에디터 전용으로 적 외형 미리 보기 메시와 시야·정찰 경로 미리 보기 컴포넌트, 배치 검사를 가집니다. |
| `AAgCheckpoint` (신규, `Interaction/`) | `IAgInteractable` 장치. 메시, 범위 구(`AgInteractionRange` 프로파일, 반지름 2m), 안내 지점(바닥에서 1.2m), 행동 이름. 범위 충돌을 끄지 않으므로 항상 쓸 수 있습니다. |
| `AAgEnemyCharacter` | `PatrolRoute`·`GroupId`는 레벨에서 편집하지 않고(보기 전용) `SetPlacement`로 받습니다. 시야·정찰 미리 보기 컴포넌트, `PostEditChangeProperty`·`PostEditMove`, 정찰·그룹 검사는 스포너로 옮깁니다. `CheckForErrors`는 레벨에 직접 놓인 적을 경고합니다. |
| `UAgEnemySightPreviewComponent`, `UAgPatrolPreviewComponent` | 주인을 적 대신 스포너로 보고, 캐릭터 데이터 ID는 스포너의 적 클래스 기본 객체에서 읽습니다. |
| `AAgPlayerCharacter` | `Rest` 추가. |
| `AAgGameMode` | `RespawnEnemies`, `StartPlay`(로딩 화면), `LoadingScreenClass` 추가. |
| `UAgCombatSubsystem` | `RemoveGroupMember`가 빈 그룹을 지웁니다. |
| `UAgAssetManager`, `UAgJsonData`, `FAgJsonReader` | 미리 읽기(`PreloadData`)와 경로 모으기(`GatherAssetPaths`) 추가. `UAgAbilitySet`만 재정의합니다. |
| `FAgItemData` | `bRefill` 추가. 휴식할 때 최대 보유 수까지 채우는 아이템입니다. |

`AAgSimulationRoomGameMode`와 `AAgEnemySpawnPoint`는 바꾸지 않습니다. 시뮬레이션 룸은 게임 모드가 적을 직접 생성하며, 스포너 없이 생성한 적도 지금처럼 동작합니다.

### 1.3 흐름

**레벨 시작**

1. 엔진이 레벨을 엽니다. 적 블루프린트는 스포너의 소프트 참조라 레벨과 함께 읽히지 않습니다.
2. 스포너가 `BeginPlay`에서 `Spawn`을 부릅니다. 적 클래스를 비동기로 읽고, 끝나면 그 클래스 기본 객체의 `CharacterDataId`로 `PreloadData<UAgAbilitySet>`을 요청합니다.
3. 게임 모드는 `StartPlay`에서 생성을 기다리는 스포너가 하나라도 있으면 로딩 화면을 Menu 레이어에 열고, 각 스포너의 `OnSpawnFinished`를 구독합니다.
4. 미리 읽기가 끝난 스포너는 `LoadData`로 어빌리티 묶음을 받아 붙잡고, 적을 지연 생성해 `SetPlacement`를 준 뒤 생성을 마칩니다.
5. 마지막 스포너가 생성을 마치면 게임 모드가 로딩 화면을 닫습니다.

**휴식**

1. `AAgCheckpoint::Interact` → `AAgPlayerCharacter::Rest`: HP를 MaxHP로, `Effect.Cooldown` 태그의 효과를 모두 제거(충전 횟수도 함께 돌아옴), `bRefill`인 아이템을 최대 보유 수까지 더합니다. MP와 UP는 건드리지 않습니다.
2. `AAgGameMode::RespawnEnemies`: 모든 스포너의 `DestroyEnemy`를 부른 뒤, 모든 스포너의 `Spawn`을 부릅니다. 클래스와 어빌리티 묶음을 이미 붙잡고 있으므로 같은 프레임에 생성됩니다.

성문은 레벨을 다시 열지 않으므로 그대로 남습니다. 보스도 다른 적과 같은 경로로 되돌립니다.

### 1.4 공통 규칙과의 관계

- [데이터 로딩과 스키마](<../../Docs/Architecture/데이터 로딩과 스키마.md>) '에셋을 메모리에 두는 동안'의 결정(하드 참조, 쓰는 쪽이 붙잡고 GC가 내림, 읽기는 동기)을 유지합니다. 같은 절의 "플레이 중에 캐릭터를 생성하는 장치는 생성할 종류의 데이터를 미리 받아 들면 됩니다"를 스포너가 구현하고, 받기 전에 에셋을 비동기로 읽는 단계만 더합니다.
- [기본 방침](<../../Docs/Architecture/기본 방침.md>) '객체 수명': 핸들은 읽는 동안만 쓰고, 읽은 뒤에는 UPROPERTY 하드 참조로 붙잡습니다.
- [상호작용](<../../Docs/Architecture/상호작용.md>) '새 상호작용을 더하는 방법'의 장치 절차를 그대로 따릅니다. 플레이어 쪽 코드와 안내 위젯은 고치지 않습니다.

## 2. GAS 및 데이터 설계

- **GameplayTag, GameplayAbility, GameplayEffect:** 새로 더하는 것이 없습니다. 쿨다운 제거는 기존 `Effect.Cooldown` 태그를 씁니다.
- **`Items.json`:** `bRefill`(기본 false)를 더하고 `Potion`에 true로 적습니다.
- **`StringTable.csv`:** `Interaction.Rest` = 휴식 / Rest.
- **에셋:** `BP_Checkpoint`(`/Game/Interaction`, 임시 메시), 로딩 화면 `WBP_LoadingScreen`(부모 `UAgMenuScreen`, 검은 배경), `BP_StageGameMode` 기본값 `LoadingScreenClass`.
- **레벨:** `LV_FortressBreach`에 놓인 적을 같은 자리·방향·정찰 경로·그룹의 스포너로 바꿉니다. 레벨 배치는 기획자와 함께 쓰는 파일이므로 바꾸기 전에 알립니다.

## 3. 인터페이스 & 함수 시그니처

```cpp
// Game/AgEnemySpawner.h
UCLASS()
class AAgEnemySpawner : public AActor
{
public:
	/** 에셋을 붙잡고 있으면 그 자리에서, 아니면 비동기로 읽은 뒤 적을 생성합니다. */
	void Spawn();

	/** 살아 있든 죽었든 자기 적을 지웁니다. */
	void DestroyEnemy();

	bool IsSpawnPending() const;

	/** 읽기를 기다린 생성이 끝나면 부릅니다. 클래스를 읽지 못해 생성하지 못했을 때도 부릅니다. */
	FSimpleMulticastDelegate OnSpawnFinished;

private:
	void HandleClassLoaded();
	void SpawnEnemy();

	UPROPERTY(EditInstanceOnly, Category = "Enemy")
	TSoftClassPtr<AAgEnemyCharacter> EnemyClass;

	UPROPERTY(EditInstanceOnly, Category = "Enemy")
	TObjectPtr<AAgPatrolRoute> PatrolRoute;

	UPROPERTY(EditInstanceOnly, Category = "Enemy")
	FName GroupId;

	UPROPERTY()
	TObjectPtr<AAgEnemyCharacter> Enemy;

	UPROPERTY()
	TSubclassOf<AAgEnemyCharacter> LoadedClass;

	UPROPERTY()
	TObjectPtr<UAgAbilitySet> AbilitySet;

	TSharedPtr<FStreamableHandle> LoadHandle;
};

// Interaction/AgCheckpoint.h
UCLASS(Abstract)
class AAgCheckpoint : public AActor, public IAgInteractable
{
public:
	virtual FText GetInteractionText() const override;
	virtual FVector GetInteractionLocation() const override;
	virtual void Interact(AAgPlayerCharacter& Player) override;
};

// AAgEnemyCharacter
/** 스포너가 생성을 마치기 전에 정찰 경로와 그룹을 줍니다. */
void SetPlacement(AAgPatrolRoute* InPatrolRoute, FName InGroupId);

// AAgPlayerCharacter
void Rest();

// AAgGameMode
void RespawnEnemies();
virtual void StartPlay() override;

// AAgPlayerController: 연 화면을 돌려줍니다. 게임 모드가 로딩 화면을 닫을 때 씁니다.
UCommonActivatableWidget* PushScreen(EAgUILayer Layer, TSubclassOf<UCommonActivatableWidget> ScreenClass);

// UAgCheatManager: 휴식 없이 적을 되돌립니다.
UFUNCTION(Exec)
void AgRespawnEnemies();

// UAgAssetManager
/** LoadData<T>(Key)가 읽을 에셋을 비동기로 미리 읽습니다. 끝난 뒤의 LoadData는 에셋을 읽느라 멈추지 않습니다. */
template <typename T>
TSharedPtr<FStreamableHandle> PreloadData(FName Key, FStreamableDelegate OnLoaded);

// UAgJsonData
/** PreloadData가 미리 읽을 에셋 경로를 모읍니다. 기본은 이 클래스의 파일 전체에서 모읍니다. */
virtual void GatherAssetPaths(const FAgJsonReader& Reader, FName Key, TArray<FSoftObjectPath>& OutPaths) const;
```

## 4. 범위

- **비동기 미리 읽기를 지금 넣습니다.** Before 측정(6장)에서 레벨 열기 전체가 중앙값 0.089초라 지금 줄어드는 멈춤은 한두 프레임이지만, 에셋이 무거워지면 필요하고 '적은 레벨 시작보다 늦게 나타난다'는 전제를 이후 코드와 테스트가 처음부터 따르게 합니다.
- **레벨 시작은 로딩 화면으로 가립니다.** 플레이에 보이는 변화이므로 구현 전에 1-spec에 규칙을 더하고, 정본에서는 [게임 플로우](<../../Docs/Design/System/게임 플로우.md>) '스테이지 공통'에 둡니다. 레벨 시작 때만 그 자리에서 기다리는 방식은 비동기 경로가 지금 콘텐츠에서 한 번도 실행되지 않아 고르지 않았습니다.
- **레벨 자체의 로딩은 다루지 않습니다.** `OpenLevel`은 그대로 한 프레임에 맵, 지형, 플레이어 에셋을 읽습니다.
- **시뮬레이션 룸은 다루지 않습니다.** 후보 적 목록(`EnemyClasses`)이 하드 참조라 그 레벨에 들어갈 때 모든 적 블루프린트가 읽히므로, 그 목록을 먼저 바꾸지 않으면 스포너로 옮겨도 얻는 것이 없습니다.

## 5. 검증에서 확인할 것

- spec 3장의 완료 조건 4개.
- 적이 지워질 때의 주변 상태: 락온 중인 대상, 날아가는 투사체와 충격파의 주인, 전투 서브시스템의 전투 중 목록과 HUD 보스 상태, 스킬 버튼의 쿨다운 표시.
- 미리 읽기가 끝난 뒤 `LoadData`와 적 생성 동안 새로 읽히는 패키지가 없는지.
- 이미 메모리에 있는 에셋을 요청했을 때 완료 콜백이 불리는 시점(엔진 소스로 확인). Restart 직후처럼 에셋이 남아 있는 경우의 로딩 화면 여부가 여기에 달려 있습니다.
- 기존 회귀 시나리오: 적이 레벨 시작 몇 프레임 뒤에 나타나므로, 시작하자마자 적을 찾는 시나리오는 기다리도록 고칩니다.
- 패키지 빌드에서 스포너의 소프트 참조만으로 적 블루프린트가 쿡되는지.

## 6. 로딩 시간 Before/After

스포너와 비동기 미리 읽기가 로딩을 어떻게 바꾸는지 같은 조건에서 재어 비교합니다.

**재는 값**

| 값 | Before (지금 코드) | After |
| - | - | - |
| 멈춘 시간 | 엔진 로그의 `LoadMap` 소요 시간. 적 블루프린트와 어빌리티 에셋 읽기가 모두 이 안에 있습니다. | 같은 로그. 적 블루프린트가 레벨과 함께 읽히지 않으므로 줄어야 합니다. |
| 조작할 수 있을 때까지 | `LoadMap`이 끝난 시각 | 로딩 화면이 닫힌 시각. 게임 모드가 닫을 때 스포너 수와 걸린 시간을 로그로 남깁니다. |
| 가장 긴 프레임 | 레벨을 열기 시작해서 조작할 수 있을 때까지의 프레임 시간 가운데 최댓값 (엔진 CSV 프로파일러) | 같음 |
| 휴식 프레임 | 없음 | 휴식한 프레임의 길이. 모든 적을 한 프레임에 지우고 생성합니다. |

**조건**

- 패키지 빌드(Development)의 `LV_FortressBreach`에서 잽니다. 에디터와 테스트 창은 쿡하지 않은 에셋을 읽어 로딩 시간이 실제와 다릅니다.
- 게임을 켠 뒤 처음 들어갈 때와 Restart 할 때를 따로 잽니다.
- 경우마다 5번 재어 최소·중앙·최대를 적습니다. 같은 PC에서, 다른 테스트 창과 빌드가 돌지 않을 때 잽니다.
- Before는 코드를 고치기 전의 패키지로 먼저 재 둡니다.

**읽는 법**

- 읽는 에셋은 같으므로 조작할 수 있을 때까지의 시간은 줄지 않고, 요청과 콜백만큼 조금 늘 수 있습니다. 줄어야 하는 것은 멈춘 시간과 가장 긴 프레임입니다.
- 지금 `LV_FortressBreach`의 적은 종류가 적어 차이가 작게 나올 수 있습니다. 이 수치는 적 종류가 늘 때 다시 비교할 기준으로도 남깁니다.
- 결과는 `4-test.md`에 표로 적습니다.

**Before 결과** (커밋 `f45a0e06`의 패키지, 5회, 최소 / 중앙 / 최대, 초)

| 경우 | `LoadMap` | `LoadMap`이 든 프레임 | 그다음 프레임 |
| - | - | - | - |
| 게임을 켠 뒤 첫 진입 | 0.083 / 0.089 / 0.450 | 0.096 / 0.109 / 0.471 | 0.115 / 0.117 / 0.141 |
| Restart | 0.138 / 0.149 / 0.229 | 0.168 / 0.189 / 0.267 | 0.007 / 0.009 / 0.026 |

- 첫 진입은 `-ExecCmds="open /Game/Maps/LV_FortressBreach"`로 타이틀에서 열었고, Restart는 스테이지에서 시작해 `-ExecCmds=RestartLevel`의 두 번째 `LoadMap`을 읽었습니다. 프레임 시간은 `-csvCaptureFrames=900`의 `FrameTime`입니다. 창 1280×720, 소리 끔.
- 적 블루프린트, 메시, 어빌리티 에셋 읽기는 모두 `LoadMap` 안에서 끝납니다. 첫 진입의 한 번(로그)에서 맵 패키지와 그 하드 참조 읽기가 0.031초, 액터 초기화와 BeginPlay가 0.021초였습니다. 월드 파티션 스트리밍 셀은 쿡되지 않아 모든 액터가 레벨과 함께 읽힙니다.
- 첫 진입의 그다음 프레임(약 0.12초)은 Restart에서는 0.01초이므로 에셋 읽기가 아니라 처음 그리는 화면의 비용입니다.
- 첫 진입의 최댓값 0.450초는 5회 가운데 한 번이며 원인은 보지 않았습니다.
- 한계: 게임을 켤 때 모든 JSON 데이터를 한 번 읽어 검사하므로 파일이 OS 캐시에 올라와 있습니다. 디스크에서 처음 읽는 경우는 재지 못했습니다. 측정한 PC는 i7-14700KF, RTX 5080, 메모리 64GB입니다.
- 측정 스크립트와 원본 기록은 `Saved/LoadMeasure/`에 있습니다(커밋되지 않음).

## 7. 정본에 반영할 문서

- [플레이어 시스템](<../../Docs/Design/System/플레이어 시스템.md>) '상호작용', 체크포인트 규칙을 둘 기획서 (spec 2.6)
- [상호작용](<../../Docs/Architecture/상호작용.md>) 클래스 구조
- [캐릭터 기반 구조와 GAS](<../../Docs/Architecture/캐릭터 기반 구조와 GAS.md>) 클래스 구조, '레벨 시작에서 BeginPlay까지'
- [데이터 로딩과 스키마](<../../Docs/Architecture/데이터 로딩과 스키마.md>) '에셋을 메모리에 두는 동안', '아이템', 'JSON 외부 데이터 위치'
- [개발 도구와 검증](<../../Docs/Architecture/개발 도구와 검증.md>) 적 시야 미리 보기
- [스테이지 - 미니던전 요새 돌파](<../../Docs/Design/Level/스테이지 - 미니던전 요새 돌파.md>)의 "적 스포너 … 없습니다", 레벨 작업 스킬의 적 배치 절차와 3D HTML에 적을 읽어 넣는 부분
