# 전투 통계 기술 설계 (Plan)

## 1. 영향 범위 및 아키텍처

기록은 플레이어에 붙은 컴포넌트가 하고, 레벨을 다시 열어도 남아야 하는 도전 횟수와 파일은 게임 인스턴스 서브시스템이 맡습니다.

| 클래스 | 구분 | 하는 일 |
| - | - | - |
| `UAgCombatStatsSubsystem` | 신규, `UGameInstanceSubsystem` (`Game/`) | 도전 횟수(레벨을 열 때 직전에 Restart를 요청했으면 +1, 아니면 1), 이번 실행의 기록 목록과 파일 저장. 파일 경로는 `Initialize`에서 `FPaths::ProjectSavedDir()/CombatStats/<실행 시각>.json`으로 정합니다. |
| `UAgCombatStatsComponent` | 신규, `UActorComponent` (`Combat/`) | 플레이어의 지금 전투를 기록합니다. 전투 중인 적 목록이 비었다가 생기면 시작, 비면 끝(결과는 플레이어 사망 → 사망, 상대한 적이 모두 사망 → 처치, 그 밖 → 이탈), Stage Clear가 정해지면 바로 처치로 끝. 끝나면 서브시스템에 넘겨 저장하고, 스테이지 결과가 정해진 뒤에 끝난 기록은 결과창용으로 따로 둡니다. Tick 없음. |
| `UAgCombatSubsystem` | 수정 | 전투 중인 적을 돌려주는 `GetCombatants` 추가. 등록·해제와 `OnCombatantsChanged`는 그대로입니다. |
| `AAgGameMode` | 수정 | `InitGame`에서 `BeginAttempt`, `RestartStage`에서 `RequestRestart`를 부릅니다. |
| `AAgPlayerCharacter` | 수정 | 생성자에서 `CombatStats` 컴포넌트를 만들고 `GetCombatStats()`로 내줍니다. `BP_Player`는 고치지 않습니다. |
| `UAgBattleResultScreen` | 수정 | 게임 모드가 `AAgSimulationRoomGameMode`이고 플레이어의 결과 기록이 있으면 통계 표를 채워 보이고, 아니면 접습니다. |
| `WBP_BattleResult` | 에셋 수정 | 아래 '4. 결과창 위젯' |

### 신호를 받는 곳

| 항목 | 신호 | 이유 |
| - | - | - |
| 전투 시작·끝, 상대한 적 | `UAgCombatSubsystem::OnCombatantsChanged` + `GetCombatants` | 전투 중인 적 목록이 이미 '플레이어와 전투 중인 적'의 정본입니다. 바뀔 때마다 지금 목록을 보고 처음 보는 적을 덧붙이므로 등록 순서에 기대지 않습니다. |
| Stage Clear | `AAgGameState::OnStageResultChanged` | 보스가 죽어도 다른 적이 남을 수 있어 목록만으로는 끝을 알 수 없습니다. |
| 패리 성공 | 플레이어 ASC의 `Event.Parry` (`GenericGameplayEventCallbacks`) | 패리 처리가 패리마다 한 번 보냅니다(투사체 포함). |
| 회피 성공 | 플레이어 ASC의 `Event.PerfectDodge` | 극한 회피 구간 효과를 실제로 지운 때만 보내므로 회피 한 번에 한 번입니다. 같은 프레임에 두 공격이 닿아도 한 번입니다. |
| 피격 | 플레이어 ASC의 엔진 `OnGameplayEffectAppliedDelegateToSelf`, 스펙의 동적 태그에 `Effect.Damage.Result.Hit` | 피격 결과마다 꼭 오는 이벤트가 없습니다(반응 없는 피격, 다른 반응 중의 피격). 실행 계산이 결과 태그를 붙인 그 스펙이 알림으로 옵니다(엔진 `ApplyGameplayEffectSpecToSelf`가 같은 스펙으로 실행 뒤 알림). 가드·패리·극한 회피·무적은 이 태그가 없고, 낙하 대미지는 실행 계산을 거치지 않습니다. 새 태그나 이벤트는 만들지 않습니다. |
| 전투 시간 | `UWorld::GetAudioTimeSeconds` | 일시정지에는 멈추고 시간 배율은 받지 않는 엔진 시계입니다. 극한 회피 포스트프로세스(`UAgPostProcessComponent`)도 같은 시계를 씁니다. |

### 사망 기록을 Game Over가 아닌 목록이 빌 때 끝내는 이유

- 플레이어를 쓰러뜨린 공격의 적용 알림은 그 공격의 사망 처리(어트리뷰트 세트 → 사망 이벤트 → 게임 모드 → Game Over)가 끝난 뒤에 옵니다. Game Over에서 끝내면 마지막 피격이 빠집니다.
- 플레이어가 사망하면 모든 적(보스 포함)이 StateTree의 플레이어 사망 전환으로 전투를 나가므로, 목록이 비는 순간에 사망으로 끝냅니다. 기록은 사망 뒤 한두 프레임 길어집니다.
- 이 이유는 컴포넌트의 해당 처리에 한 줄 주석으로 남깁니다.

### 도전 횟수

- `AAgGameMode::RestartStage`가 `RequestRestart`를 부른 뒤 레벨을 다시 엽니다. 새 레벨의 `AAgGameMode::InitGame`이 `BeginAttempt`를 불러, 요청이 있었으면 +1, 없었으면 1로 정하고 요청을 지웁니다.
- 타이틀(`LV_FrontEnd`도 `AAgGameMode`), 스테이지 선택, 시뮬레이션 룸의 Start, PIE 시작은 요청이 없으므로 모두 1이 됩니다. Restart 경로를 바꿔도 셈이 한 곳(`RestartStage`)에만 묶입니다.
- 레벨 주소 옵션(`?restart`)으로 세는 방법은 엔진 `RestartLevel`의 주소 형식에 기대므로 쓰지 않습니다.

### 저장

- 기록은 `USTRUCT`로 두고 엔진 `FJsonObjectConverter::UStructToFormattedJsonObjectString`(`SkipStandardizeCase`, 들여쓰기 출력)으로 파일 전체를 다시 쓴 뒤 `FFileHelper::SaveStringToFile`(UTF-8, BOM 없음)로 저장합니다. 폴더는 파일 쓰기가 만듭니다. 실패하면 `LogAg` Error를 남기고 게임은 이어 갑니다.
- 엔진 `USaveGame`은 바이너리라 사람이 읽을 수 없어 쓰지 않습니다(1-spec '2.4').
- 전투 시간은 `float` 초입니다. 엔진 JSON 출력이 실수를 유효 숫자 17자리로 써서(48.3 → 48.299999999999997) 읽기 어려우므로, 변환 콜백(`CustomExportCallback`)이 이 속성만 엔진 `FJsonValueNumberString`(밀리초 세 자리 글자를 숫자로 그대로 씀)으로 내보냅니다.
- 좌표는 `FIntVector`(`{"X", "Y", "Z"}`), 결과는 `UENUM` 이름으로 나옵니다.

### `Docs/Architecture/` 반영

- [캐릭터 기반 구조와 GAS](<../../Docs/Architecture/캐릭터 기반 구조와 GAS.md>)의 게임 흐름 클래스 표와 클래스 구조 그림에 두 신규 클래스를 넣고, `UAgCombatSubsystem`·`AAgGameMode`·`AAgPlayerCharacter` 행을 고칩니다.
- 공통 기술 규약은 바뀌지 않습니다.

## 2. GAS 및 데이터 설계

- 새 어빌리티, 이펙트, 태그는 없습니다. 기존 `Event.Parry`, `Event.PerfectDodge`, `Effect.Damage.Result.Hit`를 받기만 합니다.
- `DesignData` JSON은 바꾸지 않습니다. 기록 파일은 기획 데이터가 아닌 게임 출력입니다.

## 3. 인터페이스 & 함수 시그니처

```cpp
// Game/AgCombatStatsSubsystem.h
UENUM()
enum class EAgCombatResult : uint8
{
	Victory,
	Death,
	Disengage
};

USTRUCT()
struct FAgCombatEnemyStats
{
	GENERATED_BODY()

	/** 캐릭터 데이터 ID입니다. */
	UPROPERTY()
	FName Id;

	/** 레벨에 처음 놓인 자리입니다. */
	UPROPERTY()
	FIntVector Location = FIntVector::ZeroValue;
};

USTRUCT()
struct FAgCombatStats
{
	GENERATED_BODY()

	UPROPERTY() FString Level;
	UPROPERTY() FString EndedAt;
	UPROPERTY() int32 Attempt = 1;
	UPROPERTY() EAgCombatResult Result = EAgCombatResult::Victory;
	/** 실제 시간(초)입니다. 파일에는 밀리초까지 씁니다. */
	UPROPERTY() float Duration = 0.f;
	UPROPERTY() TArray<FAgCombatEnemyStats> Enemies;
	UPROPERTY() int32 ParryCount = 0;
	UPROPERTY() int32 PerfectDodgeCount = 0;
	UPROPERTY() int32 HitCount = 0;
};

UCLASS()
class UAgCombatStatsSubsystem : public UGameInstanceSubsystem
{
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	void RequestRestart();
	void BeginAttempt();
	int32 GetAttempt() const;
	void Save(const FAgCombatStats& Stats);

private:
	FString FilePath;
	TArray<FAgCombatStats> Records;   // 파일에는 { "Records": [...] }로 씁니다(파일용 USTRUCT 하나).
	int32 Attempt = 1;
	bool bRestartRequested = false;
};

// Combat/AgCombatStatsComponent.h
UCLASS()
class UAgCombatStatsComponent : public UActorComponent
{
public:
	UAgCombatStatsComponent();
	/** 스테이지 결과가 정해진 뒤에 끝난 기록입니다. 그때 기록 중인 전투가 없었으면 nullptr입니다. */
	const FAgCombatStats* GetStageResultStats() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleCombatantsChanged();
	void HandleStageResultChanged();
	void HandleParry(const FGameplayEventData* Payload);
	void HandlePerfectDodge(const FGameplayEventData* Payload);
	void HandleEffectApplied(UAbilitySystemComponent* Source, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle);
	void EndRecording(EAgCombatResult Result);

	TOptional<FAgCombatStats> Recording;
	double StartTime = 0.;
	/** Recording.Enemies와 같은 순서입니다. 끝날 때 모두 사망했는지 봅니다. */
	TArray<TWeakObjectPtr<const AAgEnemyCharacter>> Enemies;
	TOptional<FAgCombatStats> StageResultStats;
};

// UAgCombatSubsystem
void GetCombatants(TArray<AAgEnemyCharacter*>& OutEnemies) const;

// AAgGameMode
virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

// AAgPlayerCharacter
UAgCombatStatsComponent* GetCombatStats() const;
```

- 레벨 이름은 `UGameplayStatics::GetCurrentLevelName(this, true)`(PIE 접두사 제거), 끝난 시각은 `FDateTime::Now()`, 배치 좌표는 `AAgEnemyCharacter::GetHomeTransform()`의 위치를 반올림합니다.
- 결과창의 적 이름은 `UAgAssetManager::FindCharacterData(Id)->DisplayName`, '× 수'와 쉼표는 `FText::Format(INVTEXT(...))`로 붙입니다(기존 UI 숫자 표기와 같은 방식).

## 4. 결과창 위젯

지금 `WBP_BattleResult`는 루트 캔버스에 배경, 제목(위 296px), 버튼 목록(위 560px)을 고정 위치로 두고, `FadeInAnimation`이 루트 캔버스를 페이드합니다. 제목과 버튼 사이 약 140px에는 6줄 표가 들어가지 않습니다.

- 제목, 통계 표, 버튼 목록을 화면 가운데에 맞춘 세로 상자 `Content` 하나에 차례로 넣습니다. 통계 표가 접히면 제목과 버튼의 간격과 위치가 지금과 같도록 여백을 맞춥니다(가운데 정렬이라 위치는 수 px 안에서 같음). 통계 표가 보이면 그만큼 위아래로 벌어집니다.
- 통계 표 `CombatStatsTable`은 2열 `GridPanel`입니다. 왼쪽 열은 항목 이름(고정 글자), 오른쪽 열은 값 글자 6개로, 모두 `TS_Medium`입니다. 값 글자 이름은 `AttemptText`, `DurationText`, `EnemiesText`, `ParryCountText`, `PerfectDodgeCountText`, `HitCountText`로 C++에 `BindWidget`합니다.
- 루트 아래에 있으므로 페이드 애니메이션은 고치지 않습니다.
- 편집은 엔진 MCP 위젯 도구(없으면 에디터 파이썬)로 하고 바로 저장합니다. C++를 먼저 빌드해 `BindWidget` 이름을 맞춥니다.

## 5. 검증 방법

- 테스트 창(`Scripts/ScenarioTest`, `--map /Game/Maps/LV_SimulationRoom`)에서 기록 파일과 컴포넌트의 값을 읽어 판정합니다. 시나리오는 스크래치패드에 두고, 다시 쓸 만한 것만 `Saved/ScenarioTest/Scenarios`에 둡니다.
- 결과창은 테스트 창에서 결과창이 뜬 뒤 `shot showui`로 찍어 확인합니다.
- 던전·보스 스테이지의 Stage Clear 기록과 결과창에 표가 없는 것은 보스 스테이지 창에서 확인합니다.
