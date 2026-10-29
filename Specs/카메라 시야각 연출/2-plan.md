# 카메라 시야각 연출 기술 설계 (Plan)

## 1. 영향 범위 및 아키텍처

### 역할 나누기

| 변화 | 맡는 것 | 바뀌는지 |
| - | - | - |
| 흔들림 | 엔진 카메라 셰이크 (`CS_Small`, `CS_Medium`, `CS_Large`) | 패리 성공은 `CS_ParryShake` 대신 `CS_Medium` |
| 처형 구도 | `UAgExecutionCameraModifier` (`CM_Execution`) | 그대로 |
| 시야각 변화 (순간 연출, 달리기) | 새 `UAgFieldOfViewCameraModifier` | 셰이크에서 옮김 |
| 상대 쪽 회전 | `AAgPlayerCharacter` | 그대로 |

- 셰이크는 흔들림만 맡고, 시야각 변화는 카메라 모디파이어가 맡습니다. 엔진도 셰이크를 셰이크 전용 모디파이어로 돌리므로, 새 모디파이어는 셰이크와 같은 `DeltaTime`을 받습니다. 슬로우모션, 히트스톱, 일시정지에서의 흐름이 지금과 같습니다.
- 지울 것: `UAgOffsetCameraShakePattern`, 시야각만 담은 셰이크 에셋 7개(`CS_Parry`, `CS_PerfectDodge`, `CS_DodgeCounter`, `CS_ExecutionStab`, `CS_ExecutionBlow`, `CS_BossKill`, `CS_PhaseTransition`), 쓰지 않게 되는 `CS_ParryShake`.

### `UAgFieldOfViewCameraModifier`

- 시야각 연출 여러 개를 재생하고, 그 합을 시야각에 더합니다.
- `Priority`는 처형 카메라(64)보다 뒤, 셰이크(127)보다 앞인 96입니다. 처형 카메라는 시야각을 자기 구도로 덮어쓰므로, 시야각 연출은 그 뒤에 더해야 처형 중에도 남습니다.
- 연출마다 세기(0 ~ 1)를 두고, 들어가기·돌아오기에 지금 셰이크 패턴과 같은 이즈 인·아웃을 겁니다.
- 기획서의 '공통 규칙'을 이 클래스가 맡습니다.

| 규칙 | 처리 |
| - | - |
| 다른 연출은 더함 | 재생 중인 연출의 시야각 × 세기 × 거리 배율을 모두 더합니다. |
| 같은 연출은 다시 재생 | 재생 중인 같은 연출이 있으면 새로 더하지 않고, 지금 세기에서 들어가기부터 다시 시작합니다. |
| 거리 감쇠 | 재생할 때 받은 거리 배율을 곱합니다. 배율은 셰이크의 거리 감쇠와 같은 식(카메라 위치 기준, 감쇠 지수 1)으로 노티파이가 구합니다. 엔진 `CalcRadialShakeScale`은 protected라 부를 수 없습니다. |
| 궁극기 컷신에서 끝남 | `StopAll`로 모두 바로 지웁니다. |

- 같은 연출의 판별: 연출 정의(`FAgFieldOfViewEffect`)의 주소로 판별합니다. 정의는 `BP_Player`의 값과 몽타주 노티파이에만 있고 실행 중에 바뀌지 않으므로, 같은 정의에서 재생한 것이 같은 연출입니다. 주소는 비교에만 쓰고, 값은 재생할 때 복사합니다.
  - 다른 방법으로 연출마다 에셋을 두는 안(지금의 `CS_*`처럼)은 에셋 7개가 이름만 바꿔 남으므로 쓰지 않습니다.
- 모디파이어는 지금 처형 카메라처럼 플레이어 캐릭터가 `NotifyControllerChanged`에서 붙입니다. 조정할 기본값이 없으므로 BP 에셋 없이 C++ 클래스를 그대로 붙입니다.

### 값 위치

| 연출 | 위치 | 필드 |
| - | - | - |
| 상황별 연출 | `BP_Player`의 `CameraEffects` | `FAgCameraEffect`에 `FieldOfView`(`FAgFieldOfViewEffect`) 추가. `CameraShakes`에는 셰이크 3단계만 남습니다. |
| 모션 시점 연출 | 몽타주의 노티파이 | 노티파이에 `FieldOfView` 추가 |
| 달리기 시야각 | `BP_Player` 기본값 | `SprintFieldOfView`(`FAgFieldOfViewEffect`) |

- 7개 에셋의 값을 그대로 옮깁니다. `FieldOfView` → `Offset`, `BlendInTime` → `BlendInTime`, `Duration`에서 두 시간을 뺀 값 → `HoldTime`, `BlendOutTime` → `BlendOutTime`.
- `Offset`이 0이면 재생하지 않습니다. `Hold`로 재생한 연출은 `Release`할 때까지 유지하므로 `HoldTime`을 쓰지 않습니다.

### 노티파이 이름

- `UAgAnimNotify_PlayCameraShake`가 셰이크와 시야각을 함께 재생하게 되므로 `UAgAnimNotify_PlayCameraEffect`로 바꿉니다. 이 노티파이를 쓰는 몽타주는 `AM_Boss_GroundSlam`, `AM_Boss_PhaseTransition`, `AM_Player_Ultimate` 세 개입니다.
- 이름 변경은 ag-editor-workflow의 순서를 따릅니다(임시 `[CoreRedirects]`로 빌드 → 세 몽타주 다시 저장 → 옛 이름 확인 → 리다이렉트 삭제).

### 공통 규칙 준수

- 연출 값은 표현 값이라 JSON이 아니라 에셋에 둡니다([데이터 로딩과 스키마](<../../Docs/Architecture/데이터 로딩과 스키마.md>) 'JSON 외부 데이터 위치').
- 카메라 로직의 주인은 지금처럼 플레이어 캐릭터입니다. 새로 생기는 이벤트나 관리자는 없습니다.

## 2. GAS 및 데이터 설계

- 새 태그 없음. 달리기 시야각은 이미 있는 두 태그로 정합니다. Sprint 어빌리티가 주는 `State.Sprinting`과 이동 입력이 있을 때 붙는 `Input.Move`가 둘 다 있으면 켜고, 하나라도 없어지면 끕니다. 두 태그의 이벤트에서 판정하므로 매 프레임 확인하지 않습니다.
- `DesignData` 변경 없음.

## 3. 인터페이스 & 함수 시그니처

```cpp
// Camera/AgFieldOfViewCameraModifier.h
USTRUCT()
struct FAgFieldOfViewEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float Offset = 0.f;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0"))
	float BlendInTime = 0.f;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0"))
	float HoldTime = 0.f;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0"))
	float BlendOutTime = 0.f;
};

UCLASS()
class UAgFieldOfViewCameraModifier : public UCameraModifier
{
	GENERATED_BODY()

public:
	UAgFieldOfViewCameraModifier(const FObjectInitializer& ObjectInitializer);

	void Play(const FAgFieldOfViewEffect& Effect, float Scale = 1.f);
	void Hold(const FAgFieldOfViewEffect& Effect);
	void Release(const FAgFieldOfViewEffect& Effect);
	void StopAll();

protected:
	virtual void ModifyCamera(float DeltaTime, FVector ViewLocation, FRotator ViewRotation, float FOV, FVector& NewViewLocation, FRotator& NewViewRotation, float& NewFOV) override;

private:
	struct FActiveEffect
	{
		const FAgFieldOfViewEffect* Source = nullptr;
		FAgFieldOfViewEffect Effect;
		float Scale = 1.f;
		float Weight = 0.f;
		float Elapsed = 0.f;
		bool bHold = false;
		bool bReleased = false;
	};

	TArray<FActiveEffect> ActiveEffects;
};
```

```cpp
// Camera/AgCameraEffect.h
struct FAgCameraEffect
{
	// 기존 CameraShakes, bTurnToOpponent, TurnAngle에 더해
	UPROPERTY(EditAnywhere)
	FAgFieldOfViewEffect FieldOfView;
};
```

```cpp
// Character/AgPlayerCharacter.h
UPROPERTY(EditDefaultsOnly, Category = "Camera|Sprint")
FAgFieldOfViewEffect SprintFieldOfView;

UPROPERTY(Transient)
TObjectPtr<UAgFieldOfViewCameraModifier> FieldOfViewCamera;

void HandleSprintFieldOfViewChanged(const FGameplayTag Tag, int32 NewCount);

bool bSprintFieldOfViewHeld = false;
```

```cpp
// Animation/AgAnimNotify_PlayCameraEffect.h (UAgAnimNotify_PlayCameraShake에서 이름 변경)
UPROPERTY(EditAnywhere, Category = "Camera Effect")
FAgFieldOfViewEffect FieldOfView;
```

- `PlayCameraEffect`: 셰이크와 상대 쪽 회전은 지금처럼 하고, `Effect->FieldOfView`를 `FieldOfViewCamera->Play`로 재생합니다.
- 노티파이: 셰이크는 지금처럼 `PlayWorldCameraShake`로 재생합니다. 시야각은 로컬 플레이어 컨트롤러마다 카메라 매니저에서 모디파이어를 찾아, 거리 배율이 0보다 클 때 재생합니다.
- `HandleSprintFieldOfViewChanged`: `State.Sprinting`과 `Input.Move`의 태그 이벤트에 등록합니다. 두 태그가 모두 있는지가 `bSprintFieldOfViewHeld`와 달라지면 `Hold(SprintFieldOfView)` 또는 `Release(SprintFieldOfView)`를 부릅니다.
- `HandleTimeStopChanged`: `StopAllCameraShakes` 옆에서 `FieldOfViewCamera->StopAll()`을 부릅니다.

## 4. 정본 동기화 (4단계)

- [카메라와 타겟팅](<../../Docs/Design/System/카메라와 타겟팅.md>): '달리기 시야각' 절 추가, 패리 성공의 셰이크를 '보통'으로, '셰이크 단계'에서 '패리' 삭제.
- [데이터 로딩과 스키마](<../../Docs/Architecture/데이터 로딩과 스키마.md>) 'JSON 외부 데이터 위치': '카메라 연출과 셰이크', '상황별 카메라 연출', '모션 시점 카메라 연출' 행을 새 위치로 고치고 '달리기 시야각' 행 추가.
- [캐릭터 기반 구조와 GAS](<../../Docs/Architecture/캐릭터 기반 구조와 GAS.md>): `AAgPlayerCharacter` 역할에 시야각 연출 모디파이어와 달리기 시야각 추가.
