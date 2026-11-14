# 스킬 4 소용돌이 기술 설계 (Plan)

## 1. 영향 범위 및 아키텍처

### 1.1. 결정

소용돌이는 기존 충격파 `AAgShockwave`로 만듭니다. 충격파에 '여러 번 퍼지기'를 더하고, 스킬 4 타임라인의 AreaHit을 SpawnActor로 바꿉니다. 새 클래스는 없습니다.

소용돌이는 바닥 한 점에서 반경 4m를 한 번에 치는 일을 0.25초마다 다섯 번 합니다. 충격파는 이미 바닥 한 점에서 퍼지는 원형 판정이고, 페이즈 전환 밀어내기는 속도를 크게 줘서 첫 틱에 반경 전체를 칩니다([데이터 로딩과 스키마](<../../Docs/Architecture/데이터 로딩과 스키마.md>) '생성 액터 정의'). 여기에 파동 수와 간격만 더하면 됩니다.

| 항목 | 결정 | 이유 |
| - | - | - |
| 판정 주체 | 생성 액터(충격파). SpawnActor Task가 내려찍는 순간(지금 AreaHit 시각 1.03초) 생성합니다. | 플레이어가 후딜에서 다른 행동으로 넘어가 어빌리티가 끝나도 남은 타를 쳐야 합니다(Spec 2.2). 타임라인의 AreaHit을 여러 번 두면 어빌리티와 함께 끝나고, 판정 중심이 플레이어를 따라가며, 타마다 플레이어도 히트스톱으로 멈춥니다. |
| 히트스톱 | 코드를 바꾸지 않습니다. 대미지 스펙의 EffectCauser가 충격파 액터라 공격자에게 히트스톱을 걸지 않습니다(`Causer == Attacker`일 때만 겁니다). | Spec 2.4. GroundSlam 충격파와 같습니다. |
| UP 충전 1회 | 생성할 때 파동마다 스펙을 만들되 모두 적중 상태(`FAgHitState`) 하나를 가리키게 합니다. 처음 적중한 파동만 `Event.AttackHit`을 보냅니다. HitReward의 스킬 4 행은 섹션 조건이 없어 언제 맞아도 UP +15입니다. | Spec 2.4. |
| 피격 방향 | 코드를 바꾸지 않습니다. 밀려나는 방향, 움찔 모션, 백어택은 EffectCauser 위치(충격파 중심)로 정합니다. | Spec 2.2. |
| 팀 | 코드를 바꾸지 않습니다. 충격파는 쏜 캐릭터로 맞을 대상을 가립니다(`CanBeHitBy(*Shooter)`). | 플레이어는 자기 소용돌이에 맞지 않습니다. |
| 판정 모양 | 충격파 그대로 바닥에서 `Height`까지의 원기둥입니다. 반경 4m, 높이 4m로 둡니다. | 지상의 적은 지금(캡슐 중심에서 구 4m)과 같습니다. 떠 있는 적은 높이 4m 안이면 반경까지 닿습니다. Spec의 범위 문장을 이에 맞췄습니다. |
| 중심 | 충격파 그대로, 생성 자리(플레이어 캡슐 중심)에서 쏜 캐릭터의 캡슐 반높이만큼 내려 바닥에 둡니다. | VFX와 판정이 같은 자리에 그려집니다. |
| 적중 위치 | 충격파 그대로 대상 캡슐에서 중심 쪽 가장자리입니다. 피격 VFX와 대미지 숫자가 여기에 납니다. | 다른 공격과 같은 '공격 판정이 닿은 위치'라, 전투 시스템 '대미지 숫자'의 "스킬 4는 대상의 캡슐 중심" 예외가 없어집니다. Spec 2.4를 이에 맞췄습니다. |
| 시간 | 충격파는 Tick의 게임 시간으로 퍼지고 기다립니다. 슬로우모션과 궁극기 시간 정지는 전역 시간 배율이라 함께 느려지거나 멈춥니다. 정지 배율 0.001에서 4초 동안 0.004초가 흐르는 것은 시간 규칙 '정지의 허용 오차' 안입니다. | Spec 2.5. |

### 1.2. 충격파에 더하는 것

| 더하는 것 | 내용 |
| - | - |
| 파동마다 공격 데이터 | `Damage` 하나를 `Hits` 배열로 바꿉니다. 파동 수는 배열 길이이고, 파동마다 맞은 대상 목록을 비우고 다시 퍼집니다. |
| 파동 사이 간격 | `Interval`(초). 앞 파동이 최대 반경까지 퍼진 뒤 다음 파동이 퍼지기 시작할 때까지입니다. 한 번에 퍼지는 소용돌이에서는 타 사이 간격과 같습니다. 파동이 겹치지 않으므로 맞은 대상 목록 하나로 충분합니다. |
| 나이아가라 파라미터 | 생성을 마칠 때 BP의 나이아가라 컴포넌트마다 `User.Radius`(= `MaxRadius`)와 `User.Duration`(첫 파동 시작부터 마지막 파동 시작까지 = (파동 수 − 1) × (`MaxRadius` ÷ `Speed` + `Interval`))을 넣습니다. 이름은 코드의 상수입니다. |
| VFX가 끝난 뒤 사라짐 | 마지막 파동이 최대 반경에 닿으면, 켜져 있는 나이아가라 컴포넌트가 없으면 바로, 있으면 모두 `OnSystemFinished`를 보낸 뒤 사라집니다. 바로 지우면 마지막 타에 터지는 VFX가 함께 사라집니다. 지금의 두 충격파는 나이아가라가 없어 지금처럼 바로 사라집니다. |
| 외형 배율 | 지금은 외형과 관계없이 액터 배율을 판정 크기에 맞춥니다. 외형 메시(`UStaticMeshComponent`)가 있을 때만 맞추도록 바꿉니다. |

외형 배율을 바꾸는 이유: 소용돌이 BP의 나이아가라는 파라미터로 크기를 정하는데, 액터 배율이 파동마다 0에서 4로 바뀌면 함께 커졌다 작아집니다. 외형이 없는 페이즈 전환 밀어내기는 배율이 바뀌어도 보이는 것이 없었으므로 동작이 같습니다.

### 1.3. 지우는 것

스킬 4가 AreaHit을 쓰지 않게 되어, 내려찍기 VFX를 판정 Task가 실행하던 경로가 쓰는 곳 없이 남습니다.

| 지우는 것 | 쓰던 곳 |
| - | - |
| `FAgAreaHitParams::Cue`와 `UAgAbilityTask_AreaHit::Activate`의 큐 실행 | 스킬 4 AreaHit 하나. 궁극기 AreaHit은 큐를 쓰지 않습니다. |
| `UAgGameplayCueNotify_Burst::MagnitudeNiagaraParameter`와 `OnExecute_Implementation`의 그 분기 | `GC_PunishingEdge` 하나(Content 전체에서 이 속성을 저장한 에셋은 이것뿐). 남는 재정의는 부모 실행 뒤 카메라 연출 요청뿐입니다. |
| `GameplayCue.PunishingEdge` 태그(`AgGameplayTags.h`·`.cpp`) | `Abilities.json` 스킬 4 |
| `GC_PunishingEdge` 에셋 | 위 태그 |

내려찍기 VFX(`NS_PunishingEdge`)는 그대로 쓰며, 소용돌이 BP의 나이아가라 컴포넌트로 옮깁니다(3절).

### 1.4. 바꾸지 않는 것

- 스킬 4의 몽타주, MP·쿨다운, 대상 지정, RootMotion·Busy 구간, 아이콘.
- 궁극기 AreaHit, 투사체, 대상 찾기(`AgTargeting`).
- 생성 Task(`UAgAbilityTask_SpawnActor`)와 JSON 리더.
- GroundSlam 충격파와 페이즈 전환 밀어내기의 동작. 데이터 형식만 `Hits` 한 칸으로 바뀝니다.

## 2. GAS 및 데이터 설계

### 2.1. `SpawnActors.json`

새 항목:

```json
"PunishingEdgeVortex": {
  "Class": "/Game/Projectiles/BP_PunishingEdgeVortex.BP_PunishingEdgeVortex_C",
  "Speed": 100000,
  "MaxRadius": 400,
  "Height": 400,
  "Interval": 0.25,
  "Hits": [
    {"HPMultiplier": 0.4, "PPMultiplier": 0.4, "HitReaction": "Flinch", "bGuardable": true},
    {"HPMultiplier": 0.4, "PPMultiplier": 0.4, "HitReaction": "Flinch", "bGuardable": true},
    {"HPMultiplier": 0.4, "PPMultiplier": 0.4, "HitReaction": "Flinch", "bGuardable": true},
    {"HPMultiplier": 0.4, "PPMultiplier": 0.4, "HitReaction": "Flinch", "bGuardable": true},
    {"HPMultiplier": 0.4, "PPMultiplier": 0.4, "HitReaction": "KnockDown", "bGuardable": true}
  ]
}
```

- `Speed`는 페이즈 전환 밀어내기와 같이 크게 주어 첫 틱에 최대 반경까지 판정합니다. `Width`를 적지 않으므로 원 전체를 칩니다.
- 마지막 타만 다른 값을 '마지막 타' 필드로 따로 두지 않고 파동마다 적습니다. 전투 시스템의 "여러 번 때리는 공격은 타마다 공격 판정을 따로 둡니다"와 같은 모양입니다.
- `bGuardable`은 문서의 '항상 적는 필드' 규칙대로 모든 타에 적습니다.

기존 항목 `GroundSlamShockwave`와 `PhaseTransitionShockwave`는 `"Damage": {...}`를 `"Hits": [{...}]`로 옮깁니다. 값은 그대로입니다.

### 2.2. `Abilities.json` `Skill4`

```json
{"Task": "SpawnActor", "Start": 1.03, "Actor": "PunishingEdgeVortex"}
```

AreaHit 항목을 이 항목으로 바꿉니다. 생성 위치는 캐릭터 위치(캡슐 중심), 방향은 몸 정면 수평입니다. 내려찍기 전에 스킬이 끊기면 이 항목에 닿지 않아 소용돌이가 생기지 않습니다(Spec 2.1).

### 2.3. 태그와 효과

새 태그와 효과는 없습니다. `GameplayCue.PunishingEdge`만 지웁니다(1.3절).

## 3. 에셋

| 에셋 | 내용 |
| - | - |
| `/Game/Projectiles/BP_PunishingEdgeVortex` (새로 만듦) | 부모 `AAgShockwave`. 루트 아래에 나이아가라 컴포넌트 둘: `NS_PunishingEdge`(내려찍기 섬광·충격파, 지금 큐에서 옮김)와 소용돌이 VFX. 외형 메시는 없습니다. 생성 회전이 몸 정면이라 섬광이 지금처럼 앞쪽에 납니다. |
| 소용돌이 VFX `NS_PunishingEdgeVortex`와 머티리얼 (새로 만듦) | 레퍼런스와 GIF 후보를 보여 드리고 고르신 뒤 만듭니다(3-tasks 'VFX'). `User.Radius`·`User.Duration`을 읽고, 끝이 있는 시스템이어야 액터가 사라집니다. 고르기 전에는 BP에 `NS_PunishingEdge`만 두고 판정을 먼저 검증합니다. |
| `GC_PunishingEdge` (지움) | 1.3절 |

판정 값(반경, 타 시각)을 에셋에 다시 적지 않으므로, JSON을 고쳐도 VFX가 판정과 어긋나지 않습니다. 지금 AreaHit `Cue`가 반경을 큐에 실어 보내던 것과 같은 이유이고, 그 일을 판정 주체인 충격파가 합니다.

## 4. 인터페이스 & 함수 시그니처

### 4.1. `AAgShockwave` (`Source/Ag/Combat/AgShockwave.h`)

```cpp
/**
 * 지면에서 모든 수평 방향으로 퍼지는 고리입니다. Hits마다 한 번씩 퍼지며, 앞 파동이 최대 반경에 닿은 뒤 Interval초 뒤에 다음 파동이 퍼집니다.
 * 외형 메시가 있으면 반경·높이 1m 원통으로 만들고 판정 반경·높이에 맞춰 액터 배율을 바꿉니다. 너비는 사용자 지정 프리미티브 데이터 0번으로 넘깁니다.
 * 나이아가라 컴포넌트에는 최대 반경과 첫 파동부터 마지막 파동까지의 시간을 넣습니다.
 */
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
	void UpdateLook();
	void DestroyWhenEffectsFinish();
	UFUNCTION()
	void HandleEffectFinished(UNiagaraComponent* Effect);

	// Speed, MaxRadius, Height, Width는 지금 그대로입니다.

	/** 앞 파동이 최대 반경에 닿은 뒤 다음 파동이 퍼지기 시작할 때까지의 시간입니다. */
	UPROPERTY(EditInstanceOnly, Category = "Shockwave")
	float Interval = 0.f;

	/** 파동마다 공격 데이터입니다. 파동 수는 배열 길이입니다. */
	UPROPERTY(EditInstanceOnly, Category = "Shockwave")
	TArray<FAgDamage> Hits;

	/** 파동마다 하나이며 모두 같은 적중 상태를 가리킵니다. 엔진이 적용할 때 복사하므로 적중 위치만 바꿔 다시 씁니다. */
	UPROPERTY()
	TArray<FGameplayEffectSpecHandle> HitSpecs;

	TArray<TWeakObjectPtr<AActor>> TouchedActors;
	float Radius = 0.f;
	int32 Wave = 0;
	float WaitTime = 0.f;
};
```

`Damage`와 `DamageSpec`은 `Hits`와 `HitSpecs`로 바뀝니다.

동작:

- `OnSpawnedByAbility`: 적중 상태 하나를 만들고 `Hits`마다 `Ability.MakeDamageSpec`으로 스펙을 만들어 `AddInstigator(GetInstigator(), this)`합니다.
- `BeginPlay`: 지금처럼 바닥으로 내리고 외형을 맞춘 뒤, 나이아가라 컴포넌트마다 두 파라미터를 넣습니다.
- `Tick`: 기다리는 중이면 `WaitTime`을 줄이고 0 이하가 되면 맞은 대상 목록과 반경을 비워 다음 파동을 시작합니다. 퍼지는 중이면 지금과 같이 판정하되 `HitSpecs[Wave]`를 씁니다. 최대 반경에 닿으면 마지막 파동이었으면 Tick을 끄고 `DestroyWhenEffectsFinish`, 아니면 `WaitTime = Interval`로 기다립니다. 쏜 캐릭터가 없어지면(레벨 전환) 지금처럼 사라지며, 쏜 캐릭터가 맞았거나 죽었는지는 보지 않습니다(Spec 2.2).
- `UpdateLook`: 외형 메시가 있을 때만 액터 배율과 프리미티브 데이터를 맞춥니다.

### 4.2. 지우는 선언

- `FAgAreaHitParams::Cue`, `UAgGameplayCueNotify_Burst::MagnitudeNiagaraParameter`
- `AgGameplayTags::GameplayCue_PunishingEdge`

## 5. 검증

빌드 뒤 시뮬레이션 룸(`--map "/Game/Maps/LV_SimulationRoom?Enemies=..."`)에서 시나리오로 확인합니다. 통과 기준은 실행 전에 아래로 정합니다.

| Spec 완료 조건 | 확인 방법 | 기준 |
| - | - | - |
| 5타, 0.25초 간격, 그 자리에 머묾 | 샌드백 셋을 플레이어 둘레 2~3m에 두고 스킬 4. 내려찍은 뒤 플레이어를 6m 밖으로 옮김 | 샌드백마다 HP가 5번 줄고, 줄어든 시각 사이가 0.25초 ± 1프레임. 플레이어가 옮긴 뒤에도 남은 타가 들어감 |
| 타당 배율 0.4 | 위 기록 | 타마다 감소량이 같고, ATK·DEF로 계산한 배율 0.4의 값(반올림 ±1) |
| 소형 1~4타 움찔, 5타 넉다운 | 샌드백 몽타주 순서 | 움찔이 4번, 다섯째 타에 넉다운 |
| 떠 있는 적 | 스킬 2로 띄운 샌드백에 스킬 4 | 1~4타 저글, 5타 넉다운 |
| 중형 | 절단자에게 스킬 4, 그리고 `AgTargetGroggy` 뒤 다시 | 그로기 아닐 때 1~4타 몽타주 없음, 5타 움찔. 그로기 중 5타 넉다운 |
| 끌어당기지 않음, 도중에 들어온 적 | 셋째 타 전에 범위 밖 샌드백을 범위 안으로 옮김 | 옮긴 뒤의 타만 맞음. 맞지 않은 샌드백이 중심 쪽으로 움직이지 않음 |
| 플레이어 히트스톱 없음, 후딜에서 이어 감 | `WATCH_TAGS`에 `State.Hitstop`, 후딜에서 기본 공격 연타 | 소용돌이 타의 시각에 플레이어 `State.Hitstop`이 붙지 않음. 기본 공격이 나가는 동안 소용돌이 타가 계속 들어감 |
| 끊기면 안 생김, 생긴 뒤 계속 | 내려찍기 전 플레이어에게 `Event.Hit.Stagger`. 다른 회차는 둘째 타 뒤에 같은 이벤트 | 앞 회차는 샌드백 HP 변화 없음. 뒤 회차는 5타 모두 들어감 |
| UP 1회, MP 없음 | 스킬 4 전후 UP·MP | UP +15 한 번, MP는 소모 40만 |
| 궁극기 시간 정지 | 둘째 타 뒤 궁극기 | 시간 정지 동안 소용돌이 타가 없고, 정지가 끝난 뒤 남은 타가 들어감 |
| 플레이어가 맞지 않음 | 모든 회차 | 플레이어 HP 변화 없음 |

- 이 시나리오는 `Saved/ScenarioTest/Scenarios/player_skill4.py` 회귀 시나리오로 남깁니다.
- 충격파 회귀: 기존 회귀 시나리오는 충격파가 플레이어를 맞히는 장면을 보지 않으므로(`boss_patterns`는 플레이어 무적), 보스 스테이지에서 일회용 시나리오로 GroundSlam 충격파가 플레이어를 한 번 넉업시키는지, 페이즈 전환 밀어내기가 플레이어를 넉다운시키는지 봅니다. 끝나면 지웁니다.
- 회귀: `player_skill`(궁극기 AreaHit), `boss_patterns`.
- VFX는 고르신 뒤 테스트 창 화면을 GIF로 만들어 보여 드립니다. 기본 카메라(플레이어 뒤)에서 봅니다.

## 6. 문서 동기화 (4단계)

| 문서 | 고칠 곳 |
| - | - |
| [플레이어 캐릭터 - 린](<../../Docs/Design/Content/플레이어 캐릭터 - 린.md>) | '스킬 4', '연계'의 쓰러뜨리기, '적중에 의한 충전 횟수' |
| [시간 규칙](<../../Docs/Design/System/시간 규칙.md>) | '히트스톱'(소용돌이 적중 시 플레이어는 멈추지 않음), '궁극기 시간 정지'(멈추는 대상에 소용돌이) |
| [전투 시스템](<../../Docs/Design/System/전투 시스템.md>) | '대미지 숫자'의 캡슐 중심 예외에서 스킬 4를 뺌 |
| [아트 표현 사양](<../../Docs/Art/아트 표현 사양.md>) | '스킬 4 내려찍기 VFX'의 에셋(큐 대신 소용돌이 BP), 소용돌이 VFX 행 추가 |
| [데이터 로딩과 스키마](<../../Docs/Architecture/데이터 로딩과 스키마.md>) | 파일 표의 `SpawnActors.json` 설명, 가드 가능 여부 행(충격파는 `Hits[].bGuardable`), '생성 액터 정의'(충격파의 `Interval`·`Hits`, 나이아가라 파라미터, 외형 배율), 클래스 표에 빠져 있던 `AAgShockwave`, '스킬 4 내려찍기 VFX' 행 |
| [몽타주 타임라인과 구간 판정](<../../Docs/Architecture/몽타주 타임라인과 구간 판정.md>) | '몽타주에 남는 노티파이'의 예외 문장: AreaHit `Cue` 대신 판정하는 생성 액터가 판정 값을 나이아가라에 넘김 |
| [캐릭터 기반 구조와 GAS](<../../Docs/Architecture/캐릭터 기반 구조와 GAS.md>) | `UAgGameplayCueNotify_Burst` 행의 `MagnitudeNiagaraParameter` 문장 |
