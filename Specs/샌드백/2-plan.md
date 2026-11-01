# 샌드백 기술 설계 (Plan)

## 1. 영향 범위 및 아키텍처

| 대상 | 변경 |
| - | - |
| `AAgSandbagCharacter` (신규, `AAgEnemyCharacter` 상속) | 중립 진영(`EAgTeam::Neutral`). 사망 이벤트를 받으면 사망 대신 `HPRefill`로 HP를 MaxHP로 되돌립니다. 자기 공격(`Ability.Enemy.Pattern`)이 도는 동안 엔진 `SetAnimRootMotionTranslationScale(0)`으로 모션의 이동을 끕니다(회전과 피격 반응의 이동은 그대로). 그 밖의 적 공통 기능(PP·그로기·처형 피격 뒤 PP 회복·패턴 고르기·락온 마커·처형 안내)은 그대로 물려받습니다. |
| `AAgCharacterBase::GetTeamAttitudeTowards` | 적군과 중립이 서로를 `Neutral`로 봅니다(서로 노리지 않음). `Friendly`가 아니므로 공격 판정에 들어오면 맞습니다. 아군과 중립은 서로 `Hostile`이라 플레이어의 락온·공격 방향·처형 대상에 샌드백이 들어가고, 공격 샌드백은 플레이어만 노립니다. |
| `EAgEffect::HPRefill` (신규) | `PPRefill`과 같은 형태의 즉시 효과. 대상의 MaxHP를 스냅샷 없이 읽어 HP에 덮어씁니다. |

- 그대로 맞는 기존 동작(변경 없음): 대미지 숫자(로컬 플레이어가 공격자일 때만 표시), MP·UP 충전(피해 Task의 `HitReward`), 대미지 결과 처리의 반응(사망 태그가 없으므로 HP 회복 뒤에도 그 타격의 반응·그로기 판정 유지), 처형 피격 뒤 PP 회복(`AAgEnemyCharacter`), 적 AI의 중립 피격 시 전투 미진입(`IsTargetNoticed`), 스테이지 결과(사망 알림이 없음).
- 플레이어가 샌드백을 처형할 때 처형 사망 모션은 재생하지 않습니다. 사망 처리 자체를 하지 않으므로 `AAgEnemyCharacter::HandleDeathEvent`의 사망 모션 분기에 닿지 않습니다.
- 공격 샌드백의 패턴은 내려찍기 하나라 패턴 고르기(`GatherPatterns`)를 바꾸지 않습니다.
- [기본 방침](<../../Docs/Architecture/기본 방침.md>): 규칙은 주인 객체(샌드백 캐릭터)가 맡고, 어빌리티·Task는 범용 그대로 씁니다. 새 Task·태그는 없습니다.

## 2. GAS 및 데이터 설계

### StateTree `ST_Sandbag` (공격 샌드백 전용)

기존 적 Task·조건만 씁니다. 샌드백은 `AIStateTree`를 비워 AI가 아무것도 하지 않습니다.

```
Root
├─ 전투                Task: 적: 전투 (락온·바라보기)
│  │  전환(우선): 적: 플레이어 사망 → 플레이어 사망 대기
│  ├─ 교전             전환(우선): 적: 피격 반응 중 → 피격 반응
│  │  ├─ 공격 대기      Task: 적: 패턴 후딜레이 (PostPatternDelayTime 2 ~ 2초, MOV 0이라 걷지 않음) → 완료 시 공격
│  │  └─ 공격          Task: 적: 패턴 실행 → 성공 시 공격 대기, 실패(1.5m 밖) 시 공격
│  └─ 피격 반응         Task: 적: 피격 반응 대기 → 완료 시 공격 대기
└─ 플레이어 사망 대기   Task: 적: 비전투
```

- 배치 직후 첫 상태가 공격 대기라 배치 2초 뒤부터 공격합니다. 움찔을 포함한 모든 피격 반응·그로기·처형 피격 뒤에도 공격 대기를 거칩니다.
- 플레이어 사망 시 비전투 Task가 진행 중인 패턴을 취소하고 `State.NonCombat`으로 새 패턴을 막으며, 전투 Task가 끝나며 바라보기와 락온을 풉니다.

### JSON

| 파일 | 항목 |
| - | - |
| `Characters.json` | `Sandbag`(소형, HP 10000, PP 1000, ATK 100, 피격 반응·그로기·처형 피격 어빌리티, `SandbagPPRegen`), `AttackSandbag`(같은 값 + `SandbagSlam`·`Parried`·`EnemyLockOn`, `PostPatternDelayTime` 2 ~ 2초) |
| `Abilities.json` | `SandbagSlam`: `Ability.Enemy.Pattern`, 발동 차단 태그는 보스 패턴과 같음, 타겟팅 `Enemy` 300cm, `PatternSelection` 거리 0 ~ 150cm·가중치 1 |
| `Effects.json` | `SandbagPPRegen`: `PPRegen`과 같고 초당 100 |
| `MontageTimelines.json` | `AM_Sandbag_Slam`: 회전 추적 0 ~ 0.9초(이동 보정 없음), 판정 0.95 ~ 1.05초(HP 1.0, 움찔, 가드 가능). 구간은 보스 DoubleStrike 2타와 같습니다. |

- 어빌리티 ID는 전역이라 근거리 일반 적의 공격과 겹치지 않게 `Sandbag` 접두사를 붙입니다.

### 에셋

| 에셋 | 내용 |
| - | - |
| `AM_Sandbag_Slam` | Halberd `Attack_Combo2` 단독 몽타주. 보스 DoubleStrike 2타의 조각·Rate Scale과 노티파이(무기 궤적, 패리 예고)를 옮깁니다. 콤보 중간에서 시작하는 모션이라 대기 자세와의 차이는 몽타주 블렌드 인으로 메웁니다. |
| `ST_Sandbag` | 위 StateTree |
| `BP_Sandbag`, `BP_AttackSandbag` | `BP_Sandbag`은 부모 `AAgSandbagCharacter`, 마네킹 Manny 플레이어 크기(엔진 기본 캡슐, 배율 1), `ABP_Boss`, 이동 설정은 보스·플레이어와 같음(중력 배율 1.75, 회전 속도 360°). `BP_AttackSandbag`은 `BP_Sandbag`의 자식으로 보스의 할버드 컴포넌트(`Weapon`, `WeaponCollision`), 캐릭터 데이터 ID, `ST_Sandbag`만 더합니다. |
| 샌드백 머티리얼 인스턴스 | `MI_Sandbag_01`·`MI_Sandbag_02`. Manny 머티리얼 인스턴스를 복제해 `Paint Tint`를 모래색으로 바꾸고, 몸통 쪽은 머리·다리 쪽을 부모로 둬 색 하나로 맞춥니다. 두 샌드백이 같이 씁니다. |

## 3. 인터페이스 & 함수 시그니처

```cpp
// AgSandbagCharacter.h
UCLASS(Abstract)
class AAgSandbagCharacter : public AAgEnemyCharacter
{
public:
	AAgSandbagCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void HandleDeathEvent(const FGameplayEventData* Payload) override;

private:
	void HandlePatternActivated(UGameplayAbility* Ability);
	void HandlePatternEnded(const FAbilityEndedData& Data);
};

// AgEffectData.h
enum class EAgEffect : uint8 { ..., PPRefill, HPRefill };
```
