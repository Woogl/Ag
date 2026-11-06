# 전투 중 대상 바라보기 기술 설계 (Plan)

## 1. 영향 범위 및 아키텍처

새 클래스는 없습니다. 대상을 정하고 각도를 판단하는 일은 `UAgAnimInstance`가, 뼈를 돌리는 일은 엔진 `Look At` 노드가 맡습니다.

| 대상 | 변경 |
| - | - |
| `UAgAnimInstance` | 쳐다볼 대상·지점·비율을 프레임마다 계산해 애님 그래프에 넘김 |
| `ABP_Character` (공통 애님 그래프) | 상체 비틀기 층 바로 뒤(그로기·공중 전환과 행동 슬롯 앞)에 엔진 `Look At` 노드 두 개(`neck_01`, `head`) 추가 |
| `UAgCombatRules` / `CombatRules.json` | 모든 캐릭터가 쓰는 `LookAtTargeting` 필드 추가 |
| `UAgAssetManager` | `CombatRules.json`이 대상 프리셋 ID를 가리키므로 `TargetingPresets.json` 뒤에 읽음 |

### 뼈를 돌리는 방법: 엔진 `Look At` 노드

- 엔진 `FAnimNode_LookAt`은 한 뼈의 축을 목표 지점으로 향하게 하고, 한도 각도(`LookAtClamp`)를 넘지 않게 자릅니다. 좌우·위아래를 함께 다루고, 그때 자세의 뼈 방향에서 계산합니다.
- 목과 머리에 하나씩 둡니다. `neck_01` 노드는 비율의 절반(알파 배율 0.5)으로 돌고, `head` 노드가 나머지를 돌아 머리가 대상을 정확히 봅니다. 머리 한 뼈만 크게 돌리면 목이 꺾여 보이기 때문입니다. 두 노드 모두 한도 50°라, 합쳐 약 75°까지 돕니다.
- `LookAtLocation`은 대상 뼈를 비우면 월드 좌표로 읽힙니다. C++가 부드럽게 따라가게 만든 지점을 넘기고, 노드의 보간(`InterpolationTime`)은 0으로 둡니다. 노드 보간은 목표가 멈췄다 움직일 때만 다시 시작하는 방식이라 계속 움직이는 대상에 맞지 않습니다.
- 뼈의 앞 축(`LookAt_Axis`, 뼈 로컬)은 `SK_Mannequin` 기준 자세에서 잰 얼굴 앞 방향입니다. `head`는 +Y, `neck_01`은 기준 자세의 얼굴 앞(+Y)을 목 뼈 로컬로 옮긴 (0.238, 0.971, 0)입니다. 목 뼈 자체의 +Y는 약 14° 아래를 향해, 그대로 쓰면 목이 들려 보입니다.
- 층 위치의 이유: 쳐다보는 것은 대기·이동 자세에만 얹으므로, 상체 비틀기와 같은 컴포넌트 공간 구간에서 상체 비틀기 바로 뒤에 둡니다. 그 뒤의 그로기·공중 전환과 행동 슬롯이 섞여 들어오면 그 블렌드로 시선이 덮이고, 섞여 나가면 드러납니다. 상체 비틀기가 풀리고 돌아오는 방식과 같습니다. 가드 움찔(`AdditiveSlot`)은 그 위에 더해져, 막을 때의 흔들림 동안에도 쳐다봅니다.
  - 처음 설계(행동 중에도 쳐다봄)에서는 행동 슬롯 뒤에 두었습니다. 사용자가 플레이한 뒤 공격 중에 쳐다보는 것이 어색하다며 "대기·이동 중에만"을 골랐고, 그래서 이 위치로 옮겼습니다.
- 서 있을 때의 상체 비틀기(`spine_02`·`spine_04`·`head`)는 그대로 둡니다. 쳐다보는 동안에는 `head` 노드가 머리 방향을 정하므로 상체 비틀기의 머리 몫은 덮이고, 쳐다보지 않을 때(대상 없음)는 지금처럼 남습니다.

### 쳐다볼 대상

```
대상 = 락온 대상
대상이 없고 자신이 비전투(State.NonCombat)가 아니면 = AgTargeting::FindTarget(CombatRules.json의 LookAtTargeting)
```

- 모든 캐릭터가 같은 코드와 같은 데이터를 씁니다. 캐릭터 종류로 갈라지는 코드나 캐릭터별 데이터가 없습니다.
- 적은 전투에 들어갈 때 락온 어빌리티를 켜고 플레이어를 대상으로 잡으며, 비전투로 돌아갈 때 끕니다(`ST_Enemy`의 전투 Task). 그래서 전투 중인 적은 1번에서 플레이어를 쳐다보고, 비전투 중인 적은 2번에서 찾지 않습니다.
- `LookAtTargeting`은 기존 `Enemy` 프리셋(적대·생존·거리순)에 `TargetBlockedTags: ["State.NonCombat"]`을 더해, 비전투 중인 상대를 뺍니다.

```json
"LookAtTargeting": {"Preset": "Enemy", "Range": 2000, "TargetBlockedTags": ["State.NonCombat"]}
```

- 찾기는 락온 대상이 없는 전투 중인 캐릭터가 매 프레임 반경 20m 구 겹침 한 번을 합니다. 실제로는 락온하지 않은 플레이어뿐이고, 적은 전투 중에 늘 락온 대상이 있습니다.

### 쳐다보는 지점과 비율 (`UAgAnimInstance`)

| 속성 | 내용 |
| - | - |
| `LookAtLocation` (애님 그래프로) | 대상 메시의 `head` 뼈 위치를 `LookAtSmoothingTime`(`ABP_Character` 0.5초) 동안 따라잡은 월드 좌표. 비율이 거의 0인 채로 새로 쳐다보기 시작할 때는 따라잡지 않고 바로 그 지점에서 시작해, 엉뚱한 곳에서 쓸어 오지 않습니다. 대상을 잃으면 마지막 지점을 유지한 채 비율만 내려갑니다. |
| `LookAtAlpha` (애님 그래프로) | 쳐다볼 때 1, 아닐 때 0을 목표로 같은 시간 동안 따라갑니다. 행동 모션이 섞여 들어오고 나가는 동안 시선이 풀리고 돌아옵니다. |
| `LookAtBackAngle` (기본값 110) | 대상이 몸의 방향(캐릭터 Yaw + `RootYawOffset`)에서 이 각도보다 뒤에 있으면 비율 목표를 0으로 둡니다. 뼈를 읽지 않아 한 프레임 늦는 일이 없습니다. |
| `LookAtSmoothingTime` (C++ 기본값 0.2, `ABP_Character` 0.5) | 지점과 비율이 따라잡는 시간. 지점은 지수 평활, 비율은 이 시간에 0 ↔ 1을 일정 속도로 오갑니다. 플레이 후 사용자가 0.5로 정함 |

- 두 기본값과 노드의 한도·목 배율은 상체 비틀기 값처럼 아트 연출 값이라 `ABP_Character` 기본값(노드 설정)에 둡니다. JSON에 두지 않습니다.

### 쳐다보지 않는 모션 (`UAgAnimInstance`의 private 판단)

- 아래 중 하나면 비율 목표가 0입니다. 1-spec의 '쳐다보는 때'와 같고, 상체 비틀기를 멈추는 판단과 같은 기준을 씁니다.
  - `AAgCharacterBase::IsFacingFixed()`: 어빌리티가 몽타주를 재생 중(행동, 피격 반응, 적의 패턴과 페이즈 전환)이거나 그로기
  - 공중(`bIsInAir`, 캐릭터 무브먼트가 떨어지는 중)
  - 사망(`State.Dead`), 컷신(`State.Cutscene`)
- `IsFacingFixed`는 블렌드 아웃을 시작한 몽타주를 세지 않으므로(엔진 `GetCurrentMontage`), 모션이 섞여 나가기 시작할 때부터 비율이 다시 오릅니다.
- 태그 목록을 두지 않아, 나중에 데이터로 추가하는 행동·피격 반응도 따로 적지 않아도 빠집니다.
- 노드가 행동 슬롯 앞에 있어 보이는 시선은 몽타주 블렌드가 먼저 덮습니다. 비율은 몽타주가 끝난 뒤 시선이 0.5초에 걸쳐 돌아오게 하고, 대상이 없거나 등 뒤로 갈 때 시선을 풉니다.

## 2. GAS 및 데이터 설계

- 새 태그, 어빌리티, 효과는 없습니다. 기존 태그만 읽습니다.
- `CombatRules.json`: `LookAtTargeting`(`FAgTargetingRequest`, 위 JSON) 추가.
- `UAgCombatRules`에 `FAgTargetingRequest LookAtTargeting` 필드 추가. 기존 읽기 경로(리플렉션)로 읽힙니다. 프리셋 ID 검사는 그 파일을 먼저 읽어야 하므로, `UAgAssetManager`의 파일 목록에서 `CombatRules.json`을 `TargetingPresets.json` 뒤로 옮깁니다(이 파일을 ID로 가리키는 파일은 없음).

## 3. 인터페이스 & 함수 시그니처

```cpp
// AgCombatRules.h
UPROPERTY(Transient, VisibleAnywhere, Category = "LookAt")
FAgTargetingRequest LookAtTargeting;

// AgAnimInstance.h
protected:
	UPROPERTY(EditDefaultsOnly, Category = "LookAt")
	float LookAtBackAngle = 110.f;

	UPROPERTY(EditDefaultsOnly, Category = "LookAt")
	float LookAtSmoothingTime = 0.2f;

	UPROPERTY(BlueprintReadOnly, Category = "LookAt")
	FVector LookAtLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "LookAt")
	float LookAtAlpha = 0.f;

private:
	void UpdateLookAt(float DeltaSeconds);
	AAgCharacterBase* FindLookAtTarget() const;
	bool IsLookAtBlocked() const;
```

## 4. 1-spec 수정 (승인되어 반영함)

아래 세 줄과, 대상 규칙을 모든 캐릭터 공통으로 바꾼 것(사용자 결정: 비전투 중인 적은 쳐다보지 않음)을 1-spec에 반영했습니다.

| 1-spec 현재 | 제안 | 이유 |
| - | - | - |
| 머리는 상체 방향에서 좌우 70°, 위 40°, 아래 40°까지 | 목과 머리를 합쳐 모션의 머리 방향에서 어느 쪽으로든 약 75°까지 | 엔진 `Look At`의 한도는 한 각도(원뿔)입니다. 좌우·위아래를 따로 자르려면 엔진 노드 대신 직접 만든 노드가 필요합니다. 75°면 1.5m 앞에 누운 적(약 45° 아래)도 내려다봅니다. |
| 대상이 상체 방향에서 110°보다 뒤 | 대상이 몸의 방향에서 110°보다 뒤 | 몸의 방향은 이미 계산하는 값이라 뼈를 읽지 않아도 됩니다. 상체 비틀기가 최대 60°라 두 기준의 차이는 서 있을 때만 생깁니다. |
| 락온하지 않았으면 전투 중인 적 가운데 가장 가까운 적 (거리 제한 없음) | 플레이어 캡슐 중심에서 반경 20m 안에 캡슐이 닿는 전투 중인 적 가운데 가장 가까운 적 | 범위 찾기는 구 겹침으로 하는 것이 규칙이라 반경이 필요합니다. 락온 타겟팅 범위(20m)와 같은 값입니다. |

## 5. 검증

- 시나리오 테스트(`Scripts/ScenarioTest`)로 각 프레임의 머리 뼈 앞 축과 대상 머리 방향 사이의 각도를 기록해 판정합니다. 통과 기준은 실행 전에 정합니다(예: 쳐다보는 동안 그 각도 5° 이하, 쳐다보지 않는 모션 중 비율 0).
  - 락온 없이 보스 옆을 걸을 때, 보스가 등 뒤 110° 밖으로 갈 때
  - 락온 대상과 더 가까운 다른 적이 있을 때, 락온 해제 뒤 옮겨 갈 때(프레임 사이 머리 각도 변화가 한 번에 튀지 않음)
  - 정찰 중인 적(쳐다보지 않음)과 전투에 들어간 적
  - 점프한 플레이어를 보는 적, 넉업·넉다운된 적을 보는 플레이어
  - 기본 공격·회피 중, 극한 회피·움찔·넉업·가드 브레이크·그로기 중
- 회귀: 기존 회귀 시나리오 가운데 제자리 회전·조준·피격 반응(`gunner_aim_offset`, `hit_reactions`, `player_dodge` 등)을 다시 돌립니다.
- 보이는 모습(목이 꺾여 보이지 않는지, 한도·따라잡는 시간)은 사용자 플레이로 확인합니다.
