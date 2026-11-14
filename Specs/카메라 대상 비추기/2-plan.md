# 카메라 대상 비추기 기술 설계 (Plan)

## 1. 영향 범위 및 아키텍처

### 방식

대상 비추기는 두 가지를 함께 합니다.

1. **당긴 구도(화면)**: 새 카메라 모디파이어가 게임 카메라의 화면을 덮습니다. 게임 카메라 위치에서 대상의 락온 지점을 잇는 선을 따라 카메라를 정한 거리까지 당기고, 락온 지점을 화면 한가운데 둡니다. 엔진 모디파이어의 알파(`AlphaInTime`·`AlphaOutTime`)로 들어가고 돌아오며, 처형 카메라처럼 알파에 이즈 인·아웃을 겁니다. 락온 중에도 덮습니다.
2. **게임 카메라의 방향(컨트롤 회전)**: 락온 중이 아니면 락온 카메라와 같은 길로 컨트롤 회전을 대상 쪽으로 돌립니다. 카메라 매니저가 매 프레임 '락온 대상이 있으면 락온 대상, 없으면 비추는 대상'을 골라 기존 `UpdateLockOnCamera`에 넘기므로 회전 코드는 새로 만들지 않습니다. 그래서 모디파이어가 돌아오면 대상을 본 게임 카메라가 남고, 비추는 동안 이동 방향도 게임 카메라와 맞습니다.

- 모디파이어는 매 프레임 그때의 게임 카메라 위치에서 선을 다시 그으므로, 게임 카메라가 대상 쪽으로 도는 동안과 플레이어가 밀려나는 동안에도 따라갑니다.
- 비추는 대상은 모디파이어 하나가 가집니다. 카메라 매니저는 '돌아오기 전의 대상'만 물어 회전·입력 차단·상대 쪽 회전 중단에 씁니다.
- 시작과 끝은 몽타주의 노티파이 상태가 정합니다. 모션 시점의 카메라 연출은 몽타주 노티파이에 둔다는 기존 규약(`AgAnimNotify_PlayCameraEffect`)을 따르며, 구간이 타임라인에 보이므로 길이를 숫자로 다시 적지 않습니다.
- 엔진은 몽타주가 끝나거나 끊기거나 액터가 사라질 때도 활성 노티파이 상태의 `NotifyEnd`를 부르므로(`UAnimInstance::TriggerMontageEndedEvent`, `EndNotifyStates`), 끝 처리를 따로 두지 않습니다.
- 보스 메시는 `AAgCharacterBase`에서 `AlwaysTickPoseAndRefreshBones`이므로, 보스가 화면 밖에 있어도 노티파이가 울립니다.

### 수정·추가할 클래스

| 클래스 | 변경 |
| - | - |
| `UAgLookAtCameraModifier` (신규) | `UCameraModifier`. 대상과 당기는 거리를 받아 위 1의 구도를 덮습니다. 우선순위 80: 처형 카메라(64) 뒤, 시야각 모디파이어(96)와 셰이크(127) 앞이라 당긴 구도 위에 시야각 변화와 셰이크가 더해집니다. 대상이 사라지면 바로 끕니다. |
| `AAgPlayerCameraManager` | 기본 모디파이어에 `UAgLookAtCameraModifier`를 더하고, 비추기 시작·끝 API, Tick의 대상 선택, 처형 카메라 중 돌아오기, 비추는 동안 시야 입력 차단, 상대 쪽 회전 중단, 컷신·폰 변경 시 정리를 맡습니다. |
| `FAgCameraLookAt` (신규, `AgCameraData.h`) | 당기는 거리(400), 들어가기(0.5초), 돌아오기(0.6초). 노티파이 상태에서 고치므로 `FAgFieldOfViewEffect`처럼 편집 가능하게 둡니다. |
| `UAgAnimNotifyState_CameraLookAt` (신규) | `UAnimNotifyState`. 시작할 때 거리 안이면 플레이어 카메라 매니저에 모션 주인을 비추도록 요청하고, 끝날 때 그 요청을 끝냅니다. |
| `AM_Boss_PhaseTransition` | `Camera` 트랙에 노티파이 상태 하나를 0.0001초부터 몽타주 끝까지 둡니다. (섹션 시작의 노티파이는 0.0001초 뒤에 둔다는 '몽타주에 남는 노티파이' 규약) 당기는 값은 구조체 기본값을 씁니다. `CameraShake` 트랙의 0.0001초에 포효 시작 줌인 `AgAnimNotify_PlayCameraEffect`(셰이크 없음, `FieldOfView` -12 / 0.5 / 0.08 / 0.6, `InnerRadius` 1500, `OuterRadius` 4000)를 둡니다. 반경은 0.58초 기합 노티파이와 같습니다. |

- 포효 시작 줌인은 기존 모션 시점 카메라 연출 노티파이로 되므로 코드를 바꾸지 않습니다. 기합 노티파이와 값이 달라 시야각 모디파이어에서 따로 재생되고 더해집니다.

### 검토하고 쓰지 않은 방법

- 처형 카메라 모디파이어를 넓혀 함께 쓰기: 처형은 시작할 때 정한 옆자리에 고정하고 줌·복귀 시각을 갖는 두 사람 구도이고, 대상 비추기는 게임 카메라를 따라 선 위로 당기는 한 사람 구도라 겹치는 코드는 알파 블렌드 몇 줄뿐입니다.
- 기존 `AgAnimNotify_PlayCameraEffect`에 비추는 시간 필드 추가: 몽타주 구간을 숫자로 다시 적어야 하고, 몽타주가 끊겨도 정한 시간까지 비춥니다.
- 상대 쪽 회전 확장: 가로만 돌고 입력이 들어오면 끝나는 규칙이라, 위아래 구도와 입력 차단을 옵션으로 더해야 합니다. 락온 카메라에는 둘 다 이미 있습니다.
- `EAgCameraSituation`과 `Camera.json`: 시작 시점이 모션에 묶여 있어 코드 이벤트가 없습니다. 모션이 아닌 상황이 생기면 그 코드가 같은 API를 바로 부릅니다.

## 2. GAS 및 데이터 설계

- 태그, 어빌리티, 이펙트, JSON 변경은 없습니다.
- 노티파이 상태의 값(`LookAt`, `Radius`)은 몽타주 에셋에 두며, '모션 시점 카메라 연출'의 노티파이 값과 같은 자리입니다.

## 3. 인터페이스 & 함수 시그니처

### `FAgCameraLookAt`

```cpp
// 몽타주의 대상 비추기 노티파이 상태에서 고치므로 편집 가능하게 둡니다.
USTRUCT()
struct FAgCameraLookAt
{
	GENERATED_BODY()

	/** 카메라를 대상의 락온 지점에서 이 거리까지 당깁니다. 이미 더 가까우면 당기지 않습니다. */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0"))
	float Distance = 400.f;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0"))
	float BlendInTime = 0.5f;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0"))
	float BlendOutTime = 0.6f;
};
```

### `UAgLookAtCameraModifier`

```cpp
public:
	void Start(const AActor& InTarget, const FAgCameraLookAt& LookAt);
	/** 돌아오기를 시작합니다. */
	void Stop();
	/** 돌아오기 전까지만 대상을 돌려줍니다. */
	const AActor* GetLookAtTarget() const;

protected:
	virtual void ModifyCamera(float DeltaTime, FVector ViewLocation, FRotator ViewRotation, float FOV, FVector& NewViewLocation, FRotator& NewViewRotation, float& NewFOV) override;

private:
	TWeakObjectPtr<const AActor> Target;
	float Distance = 0.f;
```

### `AAgPlayerCameraManager`

```cpp
public:
	/** 대상을 당긴 구도로 비춥니다. 처형 카메라 중이면 비추지 않습니다. */
	void StartLookAt(const AActor& Target, const FAgCameraLookAt& LookAt);
	/** 그 대상을 비추는 중일 때만 돌아오기를 시작합니다. */
	void StopLookAt(const AActor& Target);

private:
	UPROPERTY(Transient)
	TObjectPtr<UAgLookAtCameraModifier> LookAtCamera;
	uint64 LookAtStartFrame = 0;
```

- `PreviousLockOnTarget`은 락온 대상과 비추는 대상 모두를 가리키므로 `TWeakObjectPtr<const AActor> PreviousCameraTarget`으로 바꿉니다. `UpdateLockOnCamera`의 대상 매개변수는 `const AActor&`로 넓힙니다. (`GetTargetLocation`은 `AActor`의 가상 함수이고 적은 락온 지점을 돌려줍니다.)

동작:

| 위치 | 변경 |
| - | - |
| `StartLookAt` | 처형 카메라 중이면 하지 않습니다. 같은 프레임에 이미 시작한 비추기가 있고 그 대상이 더 가까우면 무시합니다(`StartCameraTurn`과 같은 순서 무관 규칙). 아니면 모디파이어를 시작하고 `LookAtStartFrame`을 정합니다. |
| `StopLookAt` | 모디파이어의 대상이 그 대상일 때만 멈춥니다. 거리 밖이라 시작하지 않았거나 다른 비추기로 바뀐 뒤의 끝은 무시됩니다. |
| `Tick` | 처형 카메라 중이면 모디파이어를 멈춥니다. 카메라 대상 = 락온 대상, 없으면 모디파이어의 대상. 대상이 바뀌면 회전 속도를 0으로 되돌리고(기존), 대상이 있으면 `UpdateLockOnCamera`. |
| `ProcessViewRotation` | 입력을 막는 조건에 모디파이어의 대상이 있음을 더합니다. 튕김 판정은 락온 중에만 하므로 그대로입니다. |
| `UpdateCameraTurn` | 멈추는 조건에 모디파이어의 대상이 있음을 더합니다. 비추기가 시작되면 다음 Tick에서 상대 쪽 회전이 끝납니다. |
| `HandleCutsceneChanged`, `ResetCameraState` | 모디파이어를 바로 끕니다. |

- 구간은 보스의 몽타주 시간, 회전은 락온과 같은 Tick 델타, 들어가기·돌아오기는 카메라 모디파이어의 델타를 씁니다. 모두 게임 시간입니다.

### `UAgAnimNotifyState_CameraLookAt`

```cpp
/** 모션 주인을 당긴 구도로 비춥니다. 구간이 끝나거나 몽타주가 끊기면 돌아옵니다. */
UCLASS(const, hidecategories = Object, collapsecategories)
class UAgAnimNotifyState_CameraLookAt : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	UPROPERTY(EditAnywhere, Category = "Camera Look At")
	FAgCameraLookAt LookAt;

	/** 플레이어 카메라가 모션 주인에게서 이보다 멀면 비추지 않습니다. */
	UPROPERTY(EditAnywhere, Category = "Camera Look At", meta = (ClampMin = "0"))
	float Radius = 4000.f;
};
```

- 카메라 매니저는 `UGameplayStatics::GetPlayerCameraManager(World, 0)`를 `AAgPlayerCameraManager`로 캐스트해 얻습니다. 애니메이션 편집기의 미리 보기 월드에서는 캐스트가 실패해 아무것도 하지 않습니다.
- 거리는 `AgAnimNotify_PlayCameraEffect`처럼 모션 주인의 위치와 카메라 위치 사이로 잽니다. 기본값 40m는 그 노티파이의 `OuterRadius` 기본값과 같고, 보스 아레나(약 33m × 33m) 어디서나 비춥니다.

## 4. 문서 반영 (4단계)

- `Docs/Architecture/캐릭터 기반 구조와 GAS.md`: `AAgPlayerCameraManager` 역할에 대상 비추기를 더하고, 표와 '클래스 구조'에 `UAgLookAtCameraModifier`를 더합니다.
- `Docs/Architecture/데이터 로딩과 스키마.md`: '모션 시점 카메라 연출' 행에 `AgAnimNotifyState_CameraLookAt`의 구간과 `LookAt`·`Radius`를 더합니다.
- `Docs/Architecture/몽타주 타임라인과 구간 판정.md`: '몽타주에 남는 노티파이'에 페이즈 전환 포효의 대상 비추기 노티파이 상태를 더합니다.
- 기획서 반영은 [1-spec](<1-spec.md>)의 2.4를 따릅니다.
