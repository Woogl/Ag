# GroundSlam 충격파 기술 설계 (Plan)

## 1. 영향 범위 및 아키텍처

`Projectiles.json`을 `SpawnActors.json`으로 바꾸고, 항목을 '어빌리티가 생성할 액터의 클래스와 그 클래스의 속성값'(액터 템플릿)으로 바꿉니다. 생성 Task(`SpawnActor`)는 액터의 종류를 모르고 템플릿을 그대로 복제해 생성합니다. 검기·탄환(`AAgProjectile`)과 충격파(`AAgShockwave`)는 서로 관계없는 액터 클래스이고, 각자 자기 값(속도, 사거리·최대 반경, 높이, 공격 값)을 속성으로 가집니다. 어빌리티에서 받을 것(대미지 스펙)은 인터페이스 하나로 받습니다.

| 대상 | 변경 |
| - | - |
| `SpawnActors.json` (`Projectiles.json`에서 이름 변경) | 항목을 `{"Class": 액터 클래스 경로, 그 클래스의 속성...}`으로 바꿉니다. 대상 프리셋의 Task와 같은 형식입니다. 기존 `SwordWave`·`PistolBullet`은 값을 그대로 옮기고, `GroundSlamShockwave`를 더합니다. |
| `UAgSpawnActorSet` (`UAgProjectileSet`에서 이름 변경) | `SpawnActors`를 `TMap<FName, TObjectPtr<AActor>>`(ID별 액터 템플릿)로 둡니다. `FAgProjectileDefinition`·`FAgProjectileParams`는 지웁니다. 파일은 `Data/AgSpawnActorData.h`입니다. |
| `UAgAssetManager` | 파일 목록(`JsonFiles`)의 줄과 담는 객체를 새 이름으로 바꾸고, 조회는 효과·대상 프리셋처럼 ID로 하나를 찾습니다(`FindSpawnActor`). 에디터에서는 엔진의 다시 인스턴스화 알림(`FCoreUObjectDelegates::OnObjectsReinstanced`) 때 옛 클래스로 남은 템플릿이 있으면 JSON을 다시 읽습니다. 엔진은 블루프린트를 다시 컴파일할 때 템플릿(아키타입)을 새 클래스로 바꾸지 않아(자식 액터 템플릿만 예외), 그대로 두면 옛 클래스로 생성되어 컴포넌트가 등록되지 않습니다(구현 중 PIE에서 충격파가 보이지 않던 원인). |
| `FAgJsonReader` | `Class`로 만드는 객체가 액터면 `RF_ArchetypeObject`로 만듭니다. 엔진의 액터 템플릿 스폰(`FActorSpawnParameters::Template`)에 쓰는 객체이며, 엔진의 `UChildActorComponent`도 같은 플래그로 템플릿을 만듭니다. |
| `IAgAbilitySpawnedActor` (신규 인터페이스) | 어빌리티가 생성한 액터가 생성을 마치기 전에 그 어빌리티를 받습니다(`OnSpawnedByAbility`). 공격 액터는 여기서 자기 공격 값으로 어빌리티의 대미지 스펙을 만듭니다. 적중 보상 상태와 어빌리티 태그가 지금처럼 실립니다. |
| `UAgAbilityTask_SpawnActor` (`SpawnProjectile`에서 이름 변경) | 필드는 `Actor`(`SpawnActors.json`의 ID)와 `SocketName`입니다. ID로 템플릿을 찾아 그 클래스를 템플릿으로 지연 생성하고, 인터페이스가 있으면 어빌리티를 넘긴 뒤 생성을 마칩니다. 위치(소켓 또는 캐릭터 위치)와 방향(조준점 또는 몸 정면 수평)은 그대로이며, 방향은 생성 회전으로 줍니다. |
| `AAgProjectile` | `IAgAbilitySpawnedActor`를 구현합니다. `Launch`를 지우고 `Speed`·`Range`·`Damage`를 속성으로 둡니다. 비행 방향은 시작할 때 액터의 정면에서 읽습니다. 직진 비행, 벽 소멸, 사거리, 패리 반사, 판정 모양 컴포넌트는 그대로입니다. 기획서의 투사체는 이 클래스입니다. |
| `AAgShockwave` (신규, `AActor`, Abstract) | `IAgAbilitySpawnedActor`를 구현하는 퍼지는 고리. 속성 `Speed`·`MaxRadius`·`Height`·`Damage`. 아래 '충격파 판정'. |
| `BP_GroundSlamShockwave` (신규) | `AAgShockwave`의 자식. 외형만 둡니다(아래 '에셋'). |

- 액터 속성은 `EditInstanceOnly`로 둡니다. JSON 리더는 편집 가능한 속성 중 인스턴스에서 편집할 수 있는 것만 채우고, 이 지정자는 BP 클래스 기본값에서 편집할 수 없으므로 값이 JSON 한 곳에만 있습니다.
- 그대로 맞는 기존 동작(변경 없음):
  - 보스는 멈추지 않음: 피격 결과 처리는 효과 주체(Causer)가 공격자일 때만 공격자에게 히트스톱을 거는데, 충격파가 효과 주체입니다.
  - 피격 방향과 넉업 밀림: 피격 결과 처리가 효과 주체의 위치에서 피격자 쪽으로 방향을 정하고, 반응 이벤트의 Instigator도 효과 주체입니다. 넉업 어빌리티는 이벤트의 Instigator를 행동 대상으로 삼아(`UAgGameplayAbility::ActivateAbility`) 충격파 중심의 반대쪽으로 밀립니다.
  - 가드 불가: `bGuardable` false라 가드·패리 결과가 없습니다. 반사와 투사체 패리의 PP 규칙은 피격 결과 처리가 `AAgProjectile`에만 적용합니다.
  - 무적, 극한 회피: 실행 계산이 대상 상태로 정합니다. 극한 회피 캡슐은 `Pawn` 오브젝트 타입이라 아래 겹침 질의에 걸립니다.
  - 같은 팀 무시(`CanBeHitBy`), 시간 정지·슬로우모션(게임 전체 시간 배율), 보스의 상태 변화와 관계없이 계속(투사체와 같이 발사자 액터가 사라질 때만 소멸).
- [기본 방침](<../../Docs/Architecture/기본 방침.md>): 새 Task와 새 태그는 없습니다(기존 Task의 이름 변경). 생성 Task는 액터 종류로 분기하지 않습니다. 적중 적용(대미지 스펙 복사본에 컨텍스트를 넣어 적용하는 10줄 남짓)은 두 액터에 각자 둡니다.

### 충격파 판정

| 단계 | 처리 |
| - | - |
| 시작 (`BeginPlay`) | 생성 위치(보스 캡슐 중심)에서 발사자 캡슐의 반 높이만큼 내려 지면(발이 닿은 높이)에 액터를 둡니다. 반경은 0입니다. |
| 매 틱 | 이번 틱의 반경 구간 `r0` ~ `r1`(`r1` = `r0` + `Speed` × 시간, `MaxRadius`에서 자름)을 정합니다. 지면에서 `Height` 두께의 상자(가로·세로 반 크기 `r1`)로 `Pawn` 오브젝트를 겹침 질의하고, 결과의 캡슐마다 충격파 중심과 캡슐 중심의 수평 거리 `d`와 캡슐 반지름 `R`로 `d − R ≤ r1`이고 `d + R ≥ r0`이면 이번 틱에 가장자리가 그 캡슐을 지났다고 봅니다. 결과 캡슐의 위치를 쓰므로 극한 회피 캡슐도 같은 검사를 받습니다. |
| 적중 | 처음 닿은 대상이고 `CanBeHitBy`를 통과하면 적중을 적용합니다. 적중 위치는 대상 캡슐 중심에서 충격파 중심 쪽으로 캡슐 반지름만큼 옮긴 점입니다. |
| 외형 | 액터의 수평 배율을 반경 ÷ 100cm로 맞춥니다. BP 외형은 반경 1m 고리로 만듭니다. |
| 소멸 | 반경이 `MaxRadius`에 닿으면, 또는 발사자 액터가 사라지면 소멸합니다. 벽은 보지 않습니다. |

- 높이 검사는 상자 질의가 정확하게 하고, 수평 검사는 캡슐을 반지름 `R`의 원기둥으로 봅니다. 캡슐의 둥근 바닥이 0.1m 두께에 걸치면 실제보다 최대 10cm 일찍 닿는 것으로 보며, 초당 15m에서 0.01초 미만입니다.

## 2. GAS 및 데이터 설계

### JSON

`SpawnActors.json`:

```json
{
  "SwordWave": {
    "Class": "/Game/Projectiles/BP_SwordWave.BP_SwordWave_C",
    "Speed": 1500, "Range": 2000,
    "Damage": {"HPMultiplier": 1.2, "HitReaction": "Flinch", "bGuardable": true}
  },
  "PistolBullet": {
    "Class": "/Game/Projectiles/BP_PistolBullet.BP_PistolBullet_C",
    "Speed": 2000, "Range": 3000,
    "Damage": {"HPMultiplier": 1, "HitReaction": "Flinch", "bGuardable": true}
  },
  "GroundSlamShockwave": {
    "Class": "/Game/Projectiles/BP_GroundSlamShockwave.BP_GroundSlamShockwave_C",
    "Speed": 1500, "MaxRadius": 1500, "Height": 10,
    "Damage": {"HPMultiplier": 2, "HitReaction": "KnockUp", "bGuardable": false}
  }
}
```

| 파일 | 항목 |
| - | - |
| `SpawnActors.json` | 위와 같습니다. 검기·탄환 값은 바뀌지 않습니다. |
| `Abilities.json` | `GroundSlam`의 `End` 섹션: `AreaHit` 항목을 `{"Task": "SpawnActor", "Start": 0, "Actor": "GroundSlamShockwave"}`로 바꿉니다. `SwordWave`의 세 항목과 `PistolShot`의 항목은 Task 이름과 필드 이름(`SpawnProjectile` → `SpawnActor`, `Projectile` → `Actor`)만 바꿉니다. 나머지는 그대로입니다. |

- `bAttackerHitstop` false는 적지 않습니다. 효과 주체가 공격자가 아니면 공격자 히트스톱이 원래 없습니다.

### 에셋

| 에셋 | 내용 |
| - | - |
| `BP_GroundSlamShockwave` | `/Game/Projectiles`. 부모 `AAgShockwave`. 외형은 프로토타입 `SM_Cylinder` 하나(반경 1m, 높이 50cm, 바닥에서 1cm 띄움)에 `M_Shockwave`를 씌운 것입니다. 그림자·충돌·Nanite는 끕니다. |
| `M_Shockwave` (신규, `/Game/VFX`) | 사용자 요청으로 직접 만든 효과 머티리얼(가산, 언릿, 양면). 계산은 Custom HLSL 노드 하나에 둡니다. 원통의 아래 면에는 가장자리에서 밝고 안쪽으로 빠르게 옅어지는 지면 띠를, 옆면에는 위로 옅어지는 벽을 그리고, 위 면은 그리지 않습니다. 액터 배율이 커져도 띠 두께(`Edge` 12cm, 안쪽 잔광 `Trail` 80cm)는 월드 거리로 일정합니다. 색은 `Color` 파라미터(붉은 주황)입니다. 회색 격자 바닥에서 얇은 하늘색 조각 고리는 잘 보이지 않아(첫 구현) 바꿨습니다. |
| `BP_SwordWave`, `BP_PistolBullet` | 변경 없음. 부모 클래스와 판정 모양 컴포넌트가 그대로입니다. |

### 문서 반영 (4단계)

| 문서 | 반영 |
| - | - |
| [보스 캐릭터 - 커스터](<../../Docs/Design/Content/보스 캐릭터 - 커스터.md>) | 'GroundSlam'의 타 표·동작·설계 의도를 Spec 2.1 ~ 2.3으로 바꾸고 '충격파' 표를 둡니다. |
| [시간 규칙](<../../Docs/Design/System/시간 규칙.md>) | 궁극기 시간 정지 문장의 '보스의 투사체(검기)'에 GroundSlam 충격파를 더합니다. |
| [데이터 로딩과 스키마](<../../Docs/Architecture/데이터 로딩과 스키마.md>) | '파일 구성'의 표와 흐름도, '기본값 작성'의 가드 가능 여부 행, '재로딩 반영 시점'의 투사체 정의 행, '투사체 정의' 절(→ '생성 액터 정의': 액터 템플릿 형식, `EditInstanceOnly` 속성, 인터페이스), 클래스 표와 클래스 그림의 `UAgProjectileSet`을 새 이름과 형식으로 고칩니다. |
| [에디터 파이썬 이름](<../../.agents/skills/ag-editor-workflow/에디터 파이썬 이름.md>) | JSON 데이터 객체 이름 목록의 `Projectiles`를 `SpawnActors`로 바꿉니다. |
| [아트 표현 사양](<../../Docs/Art/아트 표현 사양.md>) | '효과와 UI 리소스'에 직접 제작한 충격파 머티리얼(`M_Shockwave`) 행을 더합니다. |

### 결정한 항목

1. **이름**: 파일 `SpawnActors.json`, 데이터 클래스 `UAgSpawnActorSet`, Task `SpawnActor`와 필드 `Actor`. 담긴 것이 어빌리티가 생성하는 액터이고, 기획 용어의 투사체(검기·탄환, 반사 규칙)는 `AAgProjectile`에만 남기기 위해서입니다. `Actors.json`은 범위가 너무 넓어 쓰지 않습니다.

## 3. 인터페이스 & 함수 시그니처

```cpp
// Combat/AgAbilitySpawnedActor.h
UINTERFACE()
class UAgAbilitySpawnedActor : public UInterface
{
	GENERATED_BODY()
};

/** 어빌리티가 생성한 액터가 생성을 마치기 전에 그 어빌리티를 받습니다. */
class IAgAbilitySpawnedActor
{
	GENERATED_BODY()

public:
	virtual void OnSpawnedByAbility(const UAgGameplayAbility& Ability) = 0;
};

// AbilitySystem/Tasks/AgAbilityTask_SpawnActor.h
USTRUCT()
struct FAgSpawnActorParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY(meta = (DataId = "SpawnActors"))
	FName Actor;

	UPROPERTY()
	FName SocketName;

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

UCLASS()
class UAgAbilityTask_SpawnActor : public UAbilityTask
{
	GENERATED_BODY()
	friend struct FAgSpawnActorParams;

public:
	virtual void Activate() override;

private:
	UPROPERTY()
	FAgSpawnActorParams Params;
};

// Combat/AgProjectile.h
UCLASS(Abstract)
class AAgProjectile : public AActor, public IAgAbilitySpawnedActor
{
public:
	AAgProjectile();

	virtual void OnSpawnedByAbility(const UAgGameplayAbility& Ability) override;
	void Reflect(AAgCharacterBase& Defender);
	bool IsReflected() const;

protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void ApplyHit(AAgCharacterBase& Target, const FVector& HitLocation);

	UPROPERTY(EditInstanceOnly, Category = "Projectile")
	float Speed = 0.f;

	UPROPERTY(EditInstanceOnly, Category = "Projectile")
	float Range = 0.f;

	UPROPERTY(EditInstanceOnly, Category = "Projectile")
	FAgDamage Damage;

	// 이하 HitShape, TouchedActors, Reflector, DamageSpec, FlightDirection, Traveled는 그대로
};

// Combat/AgShockwave.h
/** 지면에서 모든 수평 방향으로 퍼지는 고리입니다. */
UCLASS(Abstract)
class AAgShockwave : public AActor, public IAgAbilitySpawnedActor
{
	GENERATED_BODY()

public:
	AAgShockwave();

	virtual void OnSpawnedByAbility(const UAgGameplayAbility& Ability) override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void ApplyHit(AAgCharacterBase& Target, const FVector& HitLocation);

	UPROPERTY(EditInstanceOnly, Category = "Shockwave")
	float Speed = 0.f;

	UPROPERTY(EditInstanceOnly, Category = "Shockwave")
	float MaxRadius = 0.f;

	UPROPERTY(EditInstanceOnly, Category = "Shockwave")
	float Height = 0.f;

	UPROPERTY(EditInstanceOnly, Category = "Shockwave")
	FAgDamage Damage;

	UPROPERTY()
	FGameplayEffectSpecHandle DamageSpec;

	TArray<TWeakObjectPtr<AActor>> TouchedActors;
	float Radius = 0.f;
};

// Data/AgSpawnActorData.h
UCLASS()
class UAgSpawnActorSet : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, VisibleAnywhere, Category = "SpawnActors")
	TMap<FName, TObjectPtr<AActor>> SpawnActors;
};

// Core/AgAssetManager.h
AActor* FindSpawnActor(FName Id) const;
```
