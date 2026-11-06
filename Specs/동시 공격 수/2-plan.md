# 동시 공격 수 기술 설계 (Plan)

## 1. 영향 범위 및 아키텍처

### 새 클래스: `UAgCombatSubsystem` (`UWorldSubsystem`, `Source/Ag/Combat/`)

플레이어와 전투 중인 적 목록을 들고, 그 목록으로 적의 공격 차례와 HUD의 보스를 정합니다.

| 맡는 일 | 방식 |
| - | - |
| 전투 중인 적 목록 | `ST_Enemy` '전투' 상태의 `FAgEnemyAITask_Combat`가 들어갈 때 등록하고 나갈 때 뺍니다. 사망·플레이어 사망 대기·비전투 복귀가 모두 '전투' 밖의 상태라(에셋 확인), 빙의 해제의 `StopLogic`까지 나가는 길이 이 `ExitState` 하나입니다. 모든 액터를 훑지 않습니다. |
| 변경 알림 | 목록이 바뀌면 `OnCombatantsChanged`를 보냅니다. HUD가 구독합니다. |
| 보스 | 목록에서 `AAgBossCharacter`를 찾아 돌려줍니다(`FindBoss`). 보스는 최대 한 명이라는 기존 규칙은 그대로입니다. |
| 공격 차례 | 적마다 고른 패턴을 실행 중인지와 재공격 대기가 끝나는 시각을 목록 항목에 둡니다. `HasAttackTurn`이 1-spec의 순서 규칙으로 판단합니다. |

- 재공격 대기는 차례를 나누는 규칙에만 쓰는 값이라 차례를 판단하는 서브시스템이 들고, 컨트롤러에는 두지 않습니다. 비전투로 빠지면 항목과 함께 사라지며, 다시 전투에 들어오면 대기가 없는 상태로 시작합니다.
- 시각은 패턴 후딜레이와 같은 월드 시간(`GetTimeSeconds`, 게임 시간)입니다.

### 차례 판단

`HasAttackTurn(Enemy)`는 다른 적 중 차례를 차지한 수를 세어 `MaxAttackers`보다 작으면 참입니다.

- 고른 패턴을 실행 중인 적은 차례를 차지합니다.
- 그 밖의 적은 차례를 기다리는 적(아래)이면서 순서가 앞이면 차지합니다.
  - 차례를 기다리는 적: 살아 있고, 패턴 후딜레이가 끝났고(`IsInPostPatternDelay`), 피격 반응 등이 아니고(`IsEnemyReacting`, `IsPhaseTransitionActive`), 쿨다운이 끝난 패턴이 있습니다(`GatherReadyPatternRanges`).
  - 순서: 재공격 대기가 끝난 적이 앞입니다. 같은 쪽이면 대상과의 캡슐 중심 수평 거리(`GetHorizontalDistanceTo`)가 짧은 적이 앞입니다.
- 묻는 적 자신은 호출하는 쪽이 이미 차례를 기다리는 적으로 확인한 뒤 묻습니다.
- 같은 프레임에서 적들이 어떤 순서로 판단해도 결과가 같습니다. 앞 순서의 적이 그 프레임에 패턴을 시작했든 아직 기다리든 똑같이 한 자리로 세기 때문입니다. 거리가 정확히 같은 두 적은 서로 앞이 아니어서 둘 다 차례를 받을 수 있지만, 순서를 정하는 규칙은 두지 않습니다.
- 반응형 패턴(Backstep)은 패턴 후딜레이 중에만 나오므로 그동안 그 적은 차례를 기다리는 적이 아니고, 실행해도 공격 중으로 기록하지 않습니다. 따로 처리하는 코드는 없습니다.

### 바꾸는 곳

| 위치 | 변경 |
| - | - |
| `FAgEnemyAITask_Combat` | `EnterState`에서 `AddCombatant`, `ExitState`에서 `RemoveCombatant` |
| `FAgEnemyAITask_KeepDistance::Tick` | 후딜레이가 끝났어도 `HasAttackTurn`이 거짓이면 '기다리는 중'으로 봅니다. 쓸 수 있는 패턴을 찾는 것과 공격할 차례의 목표 거리를 모두 이 조건 안에 둡니다. 차례를 잃으면 기존 코드가 경로 이동을 끊고 적정 거리로 돌아갑니다. |
| `FAgEnemyAITask_RunPattern::EnterState` | 반응형이 아니면 후딜레이 확인 옆에서 `HasAttackTurn`도 확인해 실패합니다. 반응형 패턴 상태가 끝나면 간격 지키기를 거치지 않고 패턴 실행으로 바로 가므로 여기서도 막아야 합니다. 실패하면 기존 전환(상태 완료 → 간격 지키기)을 탑니다. 어빌리티 발동에 성공하면 `StartAttack` |
| `FAgEnemyAITask_RunPattern::ExitState` | 반응형이 아니고 발동했던 패턴이면(`Handle` 유효) `EndAttack`. 끝까지 마침, 피격 반응으로 끊김, 조준 취소, 사망·비전투로 상태를 나감이 모두 여기를 지납니다. |
| `UAgHUDWidget` | 게임 스테이트의 보스 대신 서브시스템을 구독합니다. 목록이 바뀌면 `FindBoss`로 보스를 다시 찾아 `BossStatus`의 ASC를 바꾸고, 보스가 있을 때만 보스 상태를 보입니다. 보스 ASC의 `State.NonCombat` 구독(`BindBoss`, `BossTags`)은 지웁니다. |
| `AAgGameMode::BeginPlay` | 보스를 찾아 게임 스테이트에 넣는 부분을 지웁니다. 전투원 사망 구독은 그대로입니다. |
| `AAgGameState` | `Boss`, `SetBoss`, `GetBoss`, `OnBossChanged`를 지웁니다. 스테이지 결과만 남습니다. |

- `ST_Enemy` 에셋과 게임플레이 태그는 바꾸지 않습니다.
- 보스 사망 시 보스 상태 표시가 한 군데 달라집니다. 지금은 보스가 죽으면 '사망' 상태로 가서 `State.NonCombat`이 붙지 않아 보스 상태가 남아 있고, 스테이지에서는 같은 순간 결과 연출이 HUD를 숨깁니다. 바꾸면 보스가 전투 목록에서 빠지며 보스 상태도 숨겨집니다. 그래서 다른 적이 남은 시뮬레이션 룸에서만 보이는 동작이 달라지며, 이 규칙을 1-spec '기존 기획과의 관계'에 넣었습니다.

### 공통 규칙 확인

- 캐릭터 기반 구조와 GAS '어빌리티 구조': 새 Task나 어빌리티 없이 기존 AI Task와 주인 객체(서브시스템)에 규칙을 둡니다.
- 기본 방침 '객체 수명': 서브시스템은 적을 `TWeakObjectPtr`로만 가리킵니다.
- 기본 방침 '위젯 갱신': HUD는 델리게이트로만 갱신하고 Tick을 쓰지 않습니다.
- 타겟팅의 "모든 캐릭터를 훑지 않는다" 결정: 등록·해제 목록이라 해당하지 않습니다.

## 2. GAS 및 데이터 설계

- 게임플레이 태그·효과·어빌리티는 바꾸지 않습니다.
- `CombatRules.json`에 두 값을 더하고 `UAgCombatRules`(Category `AI`)에 같은 이름의 속성을 둡니다.

| 키 | 타입 | 값 | 뜻 |
| - | - | - | - |
| `MaxAttackers` | `int32` | 2 | 동시 공격 수 |
| `ReattackWaitTime` | `float` | 2 | 재공격 대기(게임 시간 초) |

## 3. 인터페이스 & 함수 시그니처

```cpp
// Source/Ag/Combat/AgCombatSubsystem.h
UCLASS()
class UAgCombatSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void AddCombatant(AAgEnemyCharacter& Enemy);
	void RemoveCombatant(const AAgEnemyCharacter& Enemy);

	AAgBossCharacter* FindBoss() const;

	/** 차례를 기다리는 적이 물어야 합니다. 다른 적이 차지한 차례가 동시 공격 수보다 적으면 참입니다. */
	bool HasAttackTurn(const AAgEnemyCharacter& Enemy) const;

	void StartAttack(const AAgEnemyCharacter& Enemy);
	/** 재공격 대기를 시작합니다. */
	void EndAttack(const AAgEnemyCharacter& Enemy);

	FSimpleMulticastDelegate OnCombatantsChanged;

private:
	struct FCombatant
	{
		TWeakObjectPtr<AAgEnemyCharacter> Enemy;
		bool bAttacking = false;
		double ReattackWaitEndTime = 0.0;
	};

	const FCombatant* FindCombatant(const AAgEnemyCharacter& Enemy) const;

	TArray<FCombatant> Combatants;
};
```

- `.cpp`의 익명 네임스페이스: `IsWaitingForTurn(const FCombatant&)`, `IsAhead(const FCombatant& A, const FCombatant& B, double Now)`
- `UAgHUDWidget`: `HandleBossChanged` → `HandleCombatantsChanged`, `BindBoss` 삭제
- `UAgCombatRules`: `int32 MaxAttackers = 0;`, `float ReattackWaitTime = 0.f;`

## 4. 검증

`Scripts/ScenarioTest`로 시뮬레이션 룸(`--map`, 구성 옵션)과 보스 스테이지에서 1-spec 완료 조건을 확인합니다. 차례는 밖에서 읽을 수 없으므로, 프레임마다 패턴 몽타주를 재생 중인 적 수와 적마다 플레이어와의 거리로 판정합니다. 회귀로 `enemy_wait_*`, `nameplate_boss`, `stage_boss_clear`, `sim_lineup`, `gunner_aim_cancel`, `boss_patterns`를 다시 돌립니다.

## 5. 정본 반영 (4단계)

| 문서 | 반영 |
| - | - |
| 적 시스템, 일반 적 - 절단자, 일반 적 - 경비 사수, UI 시스템 | 1-spec '기존 기획과의 관계' |
| 캐릭터 기반 구조와 GAS | 기술 결정 '게임 모드와 게임 스테이트'(보스를 서브시스템이 찾음), '게임 모드와 게임 스테이트' 표와 클래스 그림에 `UAgCombatSubsystem` 추가와 `AAgGameState`·`AAgGameMode` 역할 수정, '캐릭터' 표의 `FAgEnemyAITask_KeepDistance`·`FAgEnemyAITask_RunPattern` 설명 |
| 데이터 로딩과 스키마 | '파일 구성'의 `CombatRules.json` 설명에 동시 공격 수와 재공격 대기 |
