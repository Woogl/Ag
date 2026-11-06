# 락온 대상 전환 기술 설계 (Plan)

## 1. 영향 범위 및 아키텍처

### 지금 구조

- 락온은 `LockOn` 어빌리티(적은 `EnemyLockOn`)가 발동할 때 `LockOn` 대상 프리셋으로 고른 행동 대상(`ActionTarget`)이고, `WaitTargetLost` Task가 대상이 죽거나 해제 거리 밖으로 나가면 끝나 어빌리티가 끝납니다.
- 카메라(`AAgPlayerCameraManager`), 락온 마커·네임플레이트(`AAgEnemyCharacter`), 공격 대상(`AgTargeting::FindTarget`), 처형 대상, 대상 바라보기(`UAgAnimInstance`)는 모두 `GetLockOnTarget()`으로 그 대상을 매번 읽습니다. 카메라는 대상이 바뀌면 회전 속도를 0으로 돌리고 새 대상 쪽으로 부드럽게 돕니다.
- 따라서 이 작업은 **락온 어빌리티의 대상을 실행 중에 바꾸는 것**과 **바꿀 때를 정하는 입력**만 만들면 나머지는 그대로 따라옵니다.

### 바꾸는 곳

| 대상 | 바뀌는 내용 | 공통 규칙 |
| - | - | - |
| `UAgAbilityTask_WaitTargetLost` → `UAgAbilityTask_TrackTarget` (이름 변경) | 대상을 잃을 때까지 기다리기만 하지 않고 대상을 유지·교체하므로 이름을 바꿉니다. 대상을 발동 때 한 번 잡아 두지 않고 어빌리티의 지금 대상을 봅니다. 대상이 죽으면 어빌리티의 `RefreshTarget()`으로 다시 고르고, 없을 때만 끝납니다. 해제 거리 밖이면 지금처럼 끝납니다. | 재생 Task가 `RefreshTarget()`을 부르는 것과 같은 방식입니다. Task는 어빌리티를 끝내거나 취소하지 않고, Task가 모두 끝나면 어빌리티가 끝나는 지금 흐름을 씁니다. |
| `UAgGameplayAbility` | `SetActionTarget(AActor*)` 공개 함수를 더합니다. 플레이어의 대상 전환이 고른 대상을 넣습니다. | |
| `AgTargeting` | 게임 카메라 화면으로 투영하는 `ProjectToGameScreen`을 더하고, `AgTargetingFilterTask_OnScreen`·`AgTargetingSortTask_ScreenCenter`와 아래 `FindSideTarget`이 씁니다. 화면에서 지금 대상의 한쪽에 있는 가장 가까운 후보를 찾는 `FindSideTarget`을 더합니다. 게임 카메라는 조종하는 폰의 카메라 컴포넌트(`UCameraComponent`)의 시야이며, 화면 비율 처리는 엔진 `FMinimalViewInfo::CalculateProjectionMatrixGivenViewRectangle`과 `FSceneView::ProjectWorldToScreen`을 씁니다. | 처형 카메라(카메라 모디파이어)와 궁극기 컷신(레벨 시퀀스 카메라)은 카메라 매니저의 최종 시야만 바꾸고 폰의 카메라 컴포넌트는 락온 카메라 그대로이므로, 이 시야가 명세의 '게임 카메라의 화면'입니다. FOV 연출(달리기 +6° 등)은 빠지지만 화면 가장자리의 몇 도뿐입니다. |
| `AAgPlayerCameraManager` | `ApplyLookInput`이 카메라 상태로 시야 입력의 쓰임을 고릅니다. 처형 카메라·컷신(`State.Cutscene`)이면 무시, 락온 중이면 튕김 판정, 아니면 지금처럼 자유 시점 회전입니다. 튕김이 판정되면 플레이어의 `SwitchLockOnTarget`을 부릅니다. | 시야 입력은 이미 카메라 매니저가 받고, 처형 카메라 상태도 여기에만 있습니다. 컷신 중에는 엔진 시퀀서가 시야 입력을 막고 있어(`bDisableLookAtInput`), 무시해도 지금과 같습니다. |
| `AAgPlayerCharacter` | `SwitchLockOnTarget(float Direction)`이 락온 어빌리티를 찾아 `FindSideTarget`으로 고른 대상을 `SetActionTarget`으로 넣습니다. 에디터 전용 확인용 입력 `DevLookInput`을 더합니다. | 락온 켜고 끄기(`ToggleLockOn`)처럼 입력 계층(플레이어)이 락온 어빌리티를 찾아 직접 다룹니다. 요청을 받는 곳이 하나뿐이라 게임플레이 이벤트와 태그를 두지 않습니다(사용자 결정). 확인용 입력은 기존 `DevMoveInput`과 같은 방식입니다. |

### 튕김 판정

- 시야 입력의 가로값을 자유 시점이었다면 돌았을 각도로 바꿉니다. 엔진 `APlayerController::AddYawInput`과 같은 계산(레거시 입력 배율이 켜져 있으면 컨트롤러의 Yaw 배율을 곱함, 지금 프로젝트는 켜져 있고 2.5)을 씁니다.
- 입력마다 (실제 시간, 각도)를 기록하고 판정 시간(0.1초)보다 오래된 기록을 버립니다. 남은 기록의 합(부호 있음)이 판정 각도(30°) 이상이 되는 순간 그 부호 쪽으로 한 번 전환합니다. 두 값은 기획에서 자주 조정할 값이 아니므로 `AgPlayerCameraManager.cpp`의 상수 `LockOnFlickTime`·`LockOnFlickAngle`로 둡니다.
- 전환한 쪽을 `FlickDirection`(+1/−1, 0이면 다음 전환을 받음)에 둡니다. 합에 그 방향을 곱한 값이 다음 전환 각도(5°, 상수 `LockOnFlickRestAngle`) 아래가 되면 0으로 돌립니다. 판정 각도와 같은 기준으로 되돌리면 한 번 튕기는 도중 속도가 출렁일 때 합이 30° 언저리를 오르내려 두 번 바뀌므로(플레이 확인에서 발견), 거의 멈추거나 반대로 움직일 때만 되돌립니다. 반대로 움직이면 곱이 음수가 되어 바로 되돌아갑니다.
- 새 입력을 더하기 전에 오래된 기록을 버린 합으로 위 조건을 보므로, 마우스를 멈췄다가 다시 튕기면 첫 입력부터 판정합니다.
- 락온 중이 아니거나 처형 카메라·컷신 중이면 기록을 비우고 `FlickDirection`을 0으로 돌립니다.
- 시간은 실제 시간(`UWorld::GetRealTimeSeconds`)이라 슬로우모션·히트스톱의 영향을 받지 않습니다.

### 대상 전환 선택 (`AgTargeting::FindSideTarget`)

1. 락온 어빌리티 정의의 `Targeting`(락온 대상 조건과 같은 프리셋·반경)으로 `AgTargeting::FindTargets`를 불러 후보를 얻습니다.
2. 지금 대상과 각 후보를 `ProjectToGameScreen`으로 투영합니다. 투영 기준점은 OnScreen 필터와 같은 액터 위치(캡슐 중심)입니다.
3. 지금 대상보다 `Direction` 쪽(오른쪽은 X가 큼)에 있는 후보 가운데 가로 거리가 가장 작은 후보를 `SetActionTarget`으로 넣습니다. 없거나 지금 대상을 투영할 수 없으면 바꾸지 않습니다.

### 대상 사망 시 다시 고르기

- `TickTask`에서 지금 대상이 죽었으면 `RefreshTarget()`을 부릅니다. `FindTarget`이 락온 대상(죽은 지금 대상)을 먼저 보지만 죽은 적은 프리셋에서 빠지므로, 화면 중앙에 가장 가까운 생존 후보를 고릅니다. 고른 대상이 없으면 Task가 끝나 락온이 해제됩니다.
- 적의 `EnemyLockOn`도 같은 Task를 쓰므로 똑같이 다시 고르지만, 적의 적대 대상은 플레이어 하나라 플레이어가 죽으면 지금처럼 해제됩니다.

### 영향 없는 곳

- 카메라 회전·락온 마커·네임플레이트 앞 순서·공격 방향·처형 대상·대상 바라보기는 `GetLockOnTarget()`을 매번 읽으므로 고치지 않습니다. 기본 공격은 타마다 `RefreshTarget()`으로 대상을 다시 고르므로 다음 타부터 새 대상을 향합니다.
- 키 배치·입력 매핑 에셋(`IMC_Default`, `IA_Look`)과 HUD 조작 안내는 바꾸지 않습니다.

## 2. GAS 및 데이터 설계

### `DesignData/Abilities.json`

`LockOn`, `EnemyLockOn`의 `"Task": "WaitTargetLost"`를 `"TrackTarget"`으로 바꿉니다. 필드(`ReleaseRange`)는 그대로입니다.

## 3. 인터페이스 & 함수 시그니처

```cpp
// AgGameplayAbility.h
/** 실행 중에 대상을 바꾸는 Task가 고른 대상을 넣습니다. */
void SetActionTarget(AActor* NewTarget);

// AgTargeting.h
/** 조종하는 폰의 카메라 컴포넌트 시야로 투영합니다. 처형 카메라나 컷신이 화면을 바꿔도 게임 카메라 기준입니다. */
bool ProjectToGameScreen(const APawn& Pawn, const FVector& WorldLocation, FVector2D& OutScreenPosition);

/** 후보 가운데 게임 카메라 화면에서 Current보다 Direction 쪽(양수 = 오른쪽)에 있고 Current와 가로로 가장 가까운 대상입니다. */
AActor* FindSideTarget(const FAgTargetingRequest& Request, APawn& Source, const AActor& Current, float Direction);

// AgAbilityTask_TrackTarget.h
USTRUCT()
struct FAgTrackTargetParams : public FAgTaskParams
{
	UPROPERTY()
	TOptional<float> ReleaseRange;

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

UCLASS()
class UAgAbilityTask_TrackTarget : public UAbilityTask
{
public:
	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;

private:
	/** 대상이 죽었으면 다시 고르고, 붙잡을 대상이 남았는지 돌려줍니다. */
	bool UpdateTarget();

	UPROPERTY()
	FAgTrackTargetParams Params;

	float ReleaseRange = 0.f;
};

// AgPlayerCameraManager.cpp (익명 네임스페이스)
constexpr double LockOnFlickTime = 0.1;
constexpr float LockOnFlickAngle = 30.f;
constexpr float LockOnFlickRestAngle = 5.f;

// AgPlayerCameraManager.h (private)
void UpdateLockOnFlick(AAgPlayerCharacter& Player, float YawInput);
TArray<TPair<double, float>> FlickSamples;
float FlickDirection = 0.f;

// AgPlayerCharacter.h
/** 락온 어빌리티의 대상을 화면에서 Direction 쪽(양수 = 오른쪽)의 가장 가까운 후보로 바꿉니다. 없으면 그대로 둡니다. */
void SwitchLockOnTarget(float Direction);

#if WITH_EDITORONLY_DATA
UPROPERTY(EditInstanceOnly, Transient, Category = "Dev")
FVector2D DevLookInput = FVector2D::ZeroVector;
#endif
```

## 4. 문서 반영 (4단계)

| 문서 | 반영 |
| - | - |
| [캐릭터 기반 구조와 GAS](<../../Docs/Architecture/캐릭터 기반 구조와 GAS.md>) | '입력' 행의 입력 계층 규칙 '락온 켜고 끄기'에 '대상 전환'을 더합니다. 클래스 표의 `AAgPlayerCharacter`(락온 대상 전환)와 `AAgPlayerCameraManager`(시야 입력: 자유 시점 회전, 락온 중 튕김 판정) 행을 고칩니다. |
| [데이터 로딩과 스키마](<../../Docs/Architecture/데이터 로딩과 스키마.md>) | OnScreen 필터 설명의 '카메라 화면'을 '게임 카메라(조종하는 폰의 카메라 컴포넌트) 화면'으로 고칩니다. |
| [몽타주 타임라인과 구간 판정](<../../Docs/Architecture/몽타주 타임라인과 구간 판정.md>) '형식' | `WaitTargetLost`를 `TrackTarget`으로 고칩니다. |
| [개발 도구와 검증](<../../Docs/Architecture/개발 도구와 검증.md>) '확인용 입력' | `DevLookInput`을 더합니다. |
| 기획서 | 1-spec의 '기존 기획과의 관계' 표를 따릅니다. |
