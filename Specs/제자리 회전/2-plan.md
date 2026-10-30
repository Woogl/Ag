# 제자리 회전 기술 설계 (Plan)

## 1. 영향 범위 및 아키텍처

캐릭터 정면(액터)은 캐릭터 무브먼트의 엔진 회전으로 늘 회전 속도에 맞춰 돕니다. 몸은 루트 뼈를 액터와 반대로 돌려 뒤처지게 하고, 상체와 머리는 척추·머리 뼈를 목표 쪽으로 비틉니다. 값 계산은 `UAgAnimInstance`가 맡고, 적용은 ABP의 엔진 노드(`Rotate Root Bone`, `Transform (Modify) Bone`)가 맡습니다. Lyra와 같은 구조이며, Lyra의 에임 오프셋 자세 대신 뼈 회전을 씁니다(팩에 조준 자세 에셋이 없음).

| 대상 | 변경 |
| - | - |
| `UAgAnimInstance` | 몸과 액터의 Yaw 차이 `RootYawOffset`을 계산하고, 회전 모션의 루트 회전량을 그대로(배율 없이) 이 값에 더합니다. 상체 비틀기 `UpperBodyTwist`를 계산합니다. `IsTurningInPlace`·`ConsumeTurnYaw`·`TurnScale`은 지웁니다. |
| `UAgCharacterMovementComponent` | `PhysicsRotation`에서 애님 인스턴스 연동을 지우고, 사망·방향 고정 중 회전을 막는 부분만 남깁니다. (구현함) |
| `ABP_Character` 애님 그래프 | 제자리 회전 층(하체 블렌드) → `Rotate Root Bone`(Yaw = `RootYawOffset`, 구현함) → Local To Component → `Transform (Modify) Bone` 3개 → Component To Local → 그로기 전환. 회전 모션 목록 기본값은 비웁니다. |
| `ABP_Player` 기본값 | `MoveBlendSpace` = 새 `BS_PlayerMove`, `TurnAnimations` = Samurai `Turn_45L`·`R`·`Turn_90L`·`R` |
| `ABP_Boss` 기본값 | `TurnAnimations` = Warrior `Turn_45L`·`R`, Halberd `Turn_90L`·`R` |
| 새 `BS_PlayerMove` (`/Game/Animations`) | 템플릿 `BS_Idle_Walk_Run`을 복제하고 속도 0의 9개 샘플(`MM_Idle`)만 Samurai `Idle2`로 바꿉니다. `Idle2`는 Samurai 회전 모션과 다리 자세가 15.7cm 다릅니다(Spec 2.4의 예외, 사용자 결정). 템플릿 에셋은 고치지 않습니다. |
| 회전 시퀀스 6개 | Samurai `Turn_45L`·`R`·`Turn_90L`·`R`, Halberd `Turn_90L`·`R`의 Force Root Lock을 켭니다(RefPose). 지금은 꺼져 있습니다. |

### 매 프레임 계산 (`UpdateTurnInPlace`)

`RootYawOffset` = 몸 Yaw − 액터 Yaw이고, '목표 차이' = 목표 Yaw − 몸 Yaw입니다(`GetTurnOffset`). 목표 Yaw를 정하는 방법(락온 대상 방향, 아니면 컨트롤러 목표 회전)은 지금과 같습니다.

1. `CanTurn()`이 참이면(서 있음, 목표 회전 사용, 행동·그로기·사망 아님) 액터가 지난 프레임부터 돈 만큼 오프셋에서 뺍니다. 몸이 제자리에 남습니다.
2. 회전 모션을 재생하는 중이면 그 모션의 루트 회전 변화량을 그대로 오프셋에 더합니다. 몸이 모션의 회전량(45° 또는 90°)만큼 돕니다.
3. 회전 모션을 재생하지 않을 때 |목표 차이|가 목록의 가장 작은 회전량을 넘으면, 회전량이 목표 차이에 가장 가까운 모션을 시작합니다. 모션이 끝난 뒤에도 차이가 크면 다시 시작합니다(Spec 2.2).
4. `CanTurn()`이 거짓이면 회전 모션을 멈추고 오프셋을 `FMath::ExponentialSmoothingApprox`로 0까지 줄입니다(`RootYawOffsetSmoothingTime`). 노드가 행동 슬롯·그로기·공중 전환 앞에 있어서, 그 자세가 섞여 들어오는 동안 오프셋이 덮이고 그 사이에 0이 됩니다(Spec 2.3).
5. |오프셋|은 `MaxTurnAngle`을 넘지 않게 자릅니다. 회전 모션 중에 잘리면 남은 모션은 `PlayRateAtMaxAngle`로 재생합니다. 목표 차이의 부호가 회전 방향과 반대가 되면 `PlayRateOnDirectionChange`를 쓰는 것은 지금과 같습니다.
6. 상체 비틀기의 목표는 `CanTurn()`이 참이면 목표 차이를 ±`MaxUpperBodyTwist`로 자른 값, 거짓이면 0입니다. `UpperBodyTwist`의 Yaw를 이 목표로 `ExponentialSmoothingApprox`(`UpperBodyTwistSmoothingTime`)로 따라가게 합니다. 락온 대상을 바꿔 목표가 한 번에 바뀌어도 상체가 한 프레임에 돌지 않게 하기 위함입니다.
7. 액터 Yaw는 조건과 관계없이 매 프레임 기억합니다. 그래야 다시 1로 들어갈 때 오래된 차이를 빼지 않습니다.

### 상체 비틀기 노드

- `Transform (Modify) Bone` 3개를 `spine_02`, `spine_04`, `head`에 둡니다. Rotation Mode = Add to Existing, Rotation Space = Component Space, 이동·크기는 Ignore입니다. 세 노드의 Rotation 핀은 모두 `UpperBodyTwist`에 연결하고, Alpha를 0.35, 0.35, 0.3으로 둡니다.
- 컴포넌트 공간에서 더한 회전은 자식 뼈가 물려받으므로, 가슴(팔과 무기)은 비틀기의 70%, 머리는 100%를 돕니다. 회전축은 컴포넌트 공간의 위쪽(Z)이라, 메시가 액터에 대해 −90° 돌아 있어도 Yaw 부호가 액터와 같습니다.
- 노드가 그로기 전환·행동 슬롯 앞에 있어서, 행동 중에는 몽타주 자세가 덮고 C++ 값도 0으로 줄어듭니다(Spec 2.3).

### 그 밖

- **순서:** 엔진 `ACharacter`가 메시 틱을 캐릭터 무브먼트 틱 뒤로 묶어 둡니다(`Character.cpp`, `AddPrerequisite`). 그래서 `NativeUpdateAnimation`은 이번 프레임의 액터 회전을 읽고 같은 프레임에 반대로 돌려, 몸이 떨리지 않습니다.
- **루트 모션:** 몽타주의 루트 모션은 몽타주에서 직접 꺼내므로 이 노드들과 무관합니다. Motion Warping, RootMotion Task, 무기 판정은 지금처럼 액터와 몽타주 자세를 씁니다. 행동 중에는 오프셋과 비틀기가 0입니다.
- **판정:** 액터 정면을 쓰는 코드는 그대로 두면 Spec 2.1과 맞습니다(`AgDamageResultGameplayEffectComponent`의 등 쪽 판정, Dash·RootMotion·SpawnProjectile·PlayMontage Task의 정면).
- **값의 의미 변화:** `MaxTurnAngle`(135°)은 '몸과 액터의 차이'의 상한입니다. 값은 그대로 두고 플레이로 맞춥니다.
- **블렌드 스페이스:** 스크립트로 바꾼 블렌드 스페이스는 `ResampleData()`가 불리지 않아 기본 자세를 재생할 수 있으므로, 에디터로 열고 dirty로 만든 뒤 저장합니다(에디터 스킬 '애니메이션').
- **ABP 원칙:** 노드가 6개 늘지만(그중 2개는 공간 변환), 상태 머신과 관성 블렌드는 없고 값 계산은 C++가 맡습니다([기본 방침](<../../Docs/Architecture/기본 방침.md>) 'ABP').

### 고르지 않은 방식

| 방식 | 고르지 않은 이유 |
| - | - |
| 엔진 `Offset Root Bone` (AnimationWarping) | 노드에 엔진의 실험 기능 표시가 있습니다. 오프셋을 노드가 갖고 있어서, C++가 회전 모션을 시작할지 판단하려면 노드 함수 연결(스레드 안전 UFUNCTION)과 모드 핀 전환이 필요합니다. `Rotate Root Bone`은 상태 없이 C++ 값만 적용하므로 상태가 `UAgAnimInstance` 한 곳에 있습니다. 이 이유는 `RootYawOffset` 선언에 한 줄 주석으로 남깁니다. |
| 에임 오프셋 | 팩에 조준 자세가 없어 새 에셋을 만들어야 합니다. |
| 엔진 `Orientation Warping` (AnimationWarping) | 하체를 돌리고 척추를 반대로 돌리는 노드이지만, 이동 중 하체를 이동 방향에 맞추는 용도라 설정이 그쪽에 맞춰져 있습니다(루트 모션 속도, 반대 방향 뒤집기). 플러그인도 새로 켜야 합니다. 이미 검증한 `Rotate Root Bone`에 뼈 회전만 더하는 쪽이 단순합니다. |
| Lyra ABP 이식 | 상태 머신과 BP 함수가 들어 있어 'ABP 최소' 원칙과 맞지 않습니다. 쓰는 노드는 같습니다. |
| GASP 도입 | 모션 매칭 데이터가 맨손 마네킹뿐이고, ABP·Chooser·데이터베이스가 커집니다. |

## 2. GAS 및 데이터 설계

- GameplayAbility, GameplayEffect, 태그, JSON은 바꾸지 않습니다.
- 새 값 `RootYawOffsetSmoothingTime`(0.2초), `MaxUpperBodyTwist`(60°), `UpperBodyTwistSmoothingTime`(0.15초)은 제자리 회전의 다른 연출 값과 같이 ABP 기본값입니다([데이터 로딩과 스키마](<../../Docs/Architecture/데이터 로딩과 스키마.md>) '제자리 회전 모션' 행).

## 3. 인터페이스 & 함수 시그니처

```cpp
// AgAnimInstance.h
// 지움: bool IsTurningInPlace() const;  float ConsumeTurnYaw();  float TurnScale;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace")
	float RootYawOffsetSmoothingTime = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace")
	float MaxUpperBodyTwist = 60.f;

	UPROPERTY(EditDefaultsOnly, Category = "TurnInPlace")
	float UpperBodyTwistSmoothingTime = 0.15f;

	UPROPERTY(BlueprintReadOnly, Category = "TurnInPlace")
	float RootYawOffset = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "TurnInPlace")
	FRotator UpperBodyTwist = FRotator::ZeroRotator;

private:
	// PendingTurnYaw를 대신합니다.
	float LastActorYaw = 0.f;
```

- `UpdateTurnInPlace`, `StartTurn`, `AdvanceTurn`, `CanTurn`, `GetTurnAnimations`, `GetTurnOffset`의 이름과 시그니처는 그대로입니다. `GetTurnOffset`은 '목표 − 몸'입니다. `StartTurn`은 배율을 정하지 않습니다.
- `NativeInitializeAnimation`에서 `LastActorYaw`를 캐릭터의 현재 Yaw로 둡니다.
- `UAgCharacterMovementComponent::PhysicsRotation`은 `!Character || IsDead() || IsFacingFixed()`이면 돌지 않고, 그 밖에는 `Super::PhysicsRotation`입니다.

## 4. 정본 동기화 대상 (4단계)

| 문서 | 고칠 곳 |
| - | - |
| [전투 시스템](<../../Docs/Design/System/전투 시스템.md>) | '목표 방향 회전'을 Spec 2.1~2.3으로 바꿉니다. |
| [플레이어 캐릭터 - 린](<../../Docs/Design/Content/플레이어 캐릭터 - 린.md>), [보스 캐릭터 - 커스터](<../../Docs/Design/Content/보스 캐릭터 - 커스터.md>) | '모션: 제자리 회전' 줄을 Spec 2.4로 바꿉니다. |
| [ABP와 로코모션](<../../Docs/Art/ABP와 로코모션.md>) | '요약', '애님 그래프' 표(몸 방향·상체 비틀기 층, 플레이어 이동 블렌드 스페이스), '이동 블렌드 스페이스' 표, '제자리 회전'의 모션 표·회전 방식·조건 |
| [Samurai 애니메이션 조사](<../../Docs/Art/Samurai 애니메이션 조사.md>), [Halberd 애니메이션 조사](<../../Docs/Art/Halberd 애니메이션 조사.md>) | '행동별 모션'·'보스 모션'의 제자리 회전 행 |
| [데이터 로딩과 스키마](<../../Docs/Architecture/데이터 로딩과 스키마.md>) | '제자리 회전 모션' 행: 목록 위치(자식 ABP), 새 값 3개 |
| [캐릭터 기반 구조와 GAS](<../../Docs/Architecture/캐릭터 기반 구조와 GAS.md>) | `UAgCharacterMovementComponent` 역할 설명 |
