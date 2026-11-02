# 근거리 일반 적 — 절단자 기술 설계 (Plan)

## 1. 영향 범위 및 아키텍처

새 캐릭터 클래스 없이 `BP_Cutter`가 `AAgEnemyCharacter`를 바로 상속합니다. 절단자의 규칙은 모두 데이터(JSON)와 에셋으로 표현하고, 코드는 대미지 숫자 표시 조건만 바꿉니다.

| 대상 | 변경 |
| - | - |
| `AAgEnemyCharacter` | `ShowsDamageNumbers()` 가상 함수를 더합니다. 기본은 거짓이고 `AAgBossCharacter`·`AAgSandbagCharacter`가 참으로 재정의합니다. [전투 시스템](<../../Docs/Design/System/전투 시스템.md>) '대미지 숫자'가 보스와 샌드백만 표시하므로 일반 적은 표시하지 않습니다. |
| `UAgGameplayCueNotify_DamageFloater` | 맞은 대상이 적이면 `ShowsDamageNumbers()`가 참일 때만 띄웁니다. |

- 그대로 맞는 기존 동작(변경 없음): 강인도 중형(보스와 같은 규칙: 움찔은 부위만 흔들림, 넉업·넉다운은 움찔로 패턴 중단), 시야·정찰·전투 전환·패턴 후딜레이의 좌우 걷기·피격 뒤 전환(`ST_Enemy`), 2연 베기의 첫 타 패리 뒤 계속(`Parried`는 PP만 깎음), 그로기·처형 피격 뒤 PP 회복과 처형 사망 모션(`AAgEnemyCharacter`), 락온 마커·처형 안내·네임플레이트, 극한 회피(공격자 종류를 보지 않음), 일반 적 사망(게임 모드는 보스·플레이어 사망만 결과와 처치 연출에 씀).
- 각 타의 추적 구간은 RootMotion 구간입니다. 이동 목표는 구간 시작에 한 번 정하고 회전만 구간 끝까지 대상을 따라갑니다(기존 공통 규칙). 몽타주 재생 중에는 구간 밖에서 돌지 않으므로 방향 확정 뒤 따라가지 않는 규칙도 그대로 맞습니다.
- [기본 방침](<../../Docs/Architecture/기본 방침.md>): 새 Task·태그·클래스가 없습니다.

## 2. GAS 및 데이터 설계

### JSON

| 파일 | 항목 |
| - | - |
| `Characters.json` | `Cutter`: 표시 이름 절단자, 중형, HP 600, PP 400, ATK 120, DEF 0, MOV 250, 어빌리티(패턴 3종, `Flinch`·`KnockDown`·`KnockUp`·`Juggle`·`Groggy`·`Executed`·`Parried`·`EnemyLockOn`), `PostPatternDelayTime` 0.6 ~ 1.0초, `SightDistance` 1200, `SightAngle` 120, 시작 효과 `CutterPPRegen`, 몽타주 |
| `Abilities.json` | `SingleSlash`(쿨다운 없음, 거리 0 ~ 200cm, 가중치 6), `DoubleSlash`(쿨다운 4초, 0 ~ 200cm, 가중치 3), `HeavyCleave`(쿨다운 8초, 0 ~ 230cm, 가중치 1). 셋 다 `Ability.Enemy.Pattern`, 발동 차단 태그는 보스 패턴과 같고, 타겟팅은 `Enemy` 1200cm·`TargetSpacing` 120cm |
| `Effects.json` | `CutterPPRegen`: `PPRegen`과 같고 초당 25 |
| `MontageTimelines.json` | 타마다 RootMotion 추적 구간(판정 0.15초 전까지, 최소 간격까지 다가가며 회전), 그 뒤 판정 시작 이후까지 `Direction: Back` RootMotion 구간(거리 0, 회전 없음: 모션의 남은 내딛기를 제자리에서 해 추가로 다가가지 않음), WeaponHit 판정 구간. 판정 값: `SingleSlash` HP 1.0 움찔 가드 가능, `DoubleSlash` 두 타 HP 0.8 움찔 가드 가능, `HeavyCleave` HP 1.5 넉다운 가드 불가. PP 배율은 0(기본값이라 적지 않음). 시각은 Spec '모션 조정 시 확정할 항목'의 확정값입니다. |

- 어빌리티 ID는 Spec의 패턴 ID를 그대로 씁니다. 캐릭터 ID `Cutter`와 에셋 이름의 `Cutter`는 가칭 '절단자'에서 왔습니다.

### 에셋

| 에셋 | 내용 |
| - | - |
| `AM_Cutter_SingleSlash` | Samurai `Attack_Combo3` |
| `AM_Cutter_DoubleSlash` | 섹션 `Attack1`(Samurai `Attack_Combo1`) → BlendTime 0.2 Transition(`Combo1` 0.3초, 연결 시점) → `Attack2`(`Attack_Combo2`). 적 패턴의 이음매 블렌드 규칙을 따릅니다. `Attack1`은 전환 시각 뒤로도 모션을 남겨 전환 전에 몽타주가 끝나지 않게 합니다. |
| `AM_Cutter_HeavyCleave` | Samurai `Attack_Combo4` |
| 공격 몽타주 공통 | Rate Scale 1. 모션이 원래 멈칫하는 자세를 느린 조각으로 늘려 선딜을 만들고, 무기 궤적과 공격 예고(패리 예고, `HeavyCleave`는 판정 0.5초 전부터 가드 불가 예고) 노티파이를 둡니다. 콤보 중간 자세에서 시작하는 모션은 몽타주 블렌드 인으로 대기 자세와 잇습니다. |
| 공용 재사용 | 그로기 `AM_Boss_Groggy`(이미 Samurai `Collide_To_Stun` → `Stun` → `Stun_To_Idle1`), 피격 `AM_Flinch`·`AM_FlinchAir`·`AM_KnockDown`·`AM_KnockUp`, 처형 피격·사망 `AM_Boss_Executed`·`AM_Boss_ExecutedDeath`(Spec이 고른 Warrior 처형 쌍) |
| `BS_CutterMove` | `BS_BossMove`를 복제해 Halberd 모션을 같은 이름의 Samurai 모션(`Idle1`, 8방향 `Walk_*_IPC`, 2.53m/s)으로 바꾼 이동 블렌드 스페이스. |
| `ABP_Cutter` | `ABP_Character`의 자식. 이동 `BS_CutterMove`, 제자리 회전 Samurai `Turn_45`·`Turn_90`(플레이어와 같음), 그로기 자세 Samurai `Stun`, 공중 자세 `MM_Fall_Loop`(플레이어와 같음). 그래프는 바꾸지 않습니다. |
| `BP_Cutter` | 부모 `AAgEnemyCharacter`. `SKM_Manny_Simple`, 메시 배율 1.05(애니메이션에서 Manny와 린(Quinn)의 머리 높이가 같아 1.05배가 됨, 캡슐은 보스처럼 기본 크기). 무기는 `BP_Player`의 카타나 구성(`Weapon` 태그 메시, `WeaponCollision`)을 옮깁니다. `ABP_Cutter`, 캐릭터 데이터 ID `Cutter`, `AIStateTree` `ST_Enemy`, 회전 속도 360°, 락온 마커·처형 안내·네임플레이트 위젯 클래스는 `BP_Boss`·`BP_Sandbag`과 같습니다. |

### 모션 맞춤

Spec '모션 조정 시 확정할 항목'을 구현 전에 확정합니다.

1. 네 모션의 손 속도·루트 이동을 프레임마다 재 베기 구간과 멈칫하는 자세를 찾고, 조각별 재생 속도로 타이밍 목표에 맞춥니다.
2. 대기에서 각 패턴 시작 자세로의 블렌드 인, 2연 베기 이음매 자세 차이를 확인합니다.
3. 공격 전진: 지금 RootMotion은 최대 이동 거리 없이 목표 간격(1.2m)까지 워핑하므로, 발동 거리 상한에서 전진량은 0.8m(`HeavyCleave` 1.1m)입니다. Spec의 '각 타 최대 0.4m'와 실제 도달 범위를 측정값과 함께 보여 드리고, 전진 제한이 필요하면 그때 RootMotion 최대 거리 재도입 여부를 정합니다.
4. 패턴별 움직이는 GIF와 측정값을 보여 드리고, 확인을 받으면 Spec의 표와 본문을 확정값으로 고친 뒤 타임라인에 넣습니다.

## 3. 인터페이스 & 함수 시그니처

```cpp
// AgEnemyCharacter.h
/** 플레이어의 공격에 맞았을 때 대미지 숫자를 띄웁니다. */
virtual bool ShowsDamageNumbers() const
{
	return false;
}

// AgBossCharacter.h, AgSandbagCharacter.h
virtual bool ShowsDamageNumbers() const override
{
	return true;
}
```
