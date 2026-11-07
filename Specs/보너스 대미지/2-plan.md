# 보너스 대미지 기술 설계 (Plan)

## 1. 영향 범위 및 아키텍처

| 대상 | 변경 |
| - | - |
| `UAgDamageExecution` | HP 배율에 보너스 배율(1 + 보너스 합)을 곱해 계산합니다. 판정은 파일 안 함수 `GetBonusDamageRatio`로 익명 네임스페이스에 둡니다. |
| `UAgCombatRules` | `GroggyBonusDamage`, `BackAttackBonusDamage`, `AerialBonusDamage` (float, 카테고리 `Bonus Damage`) 추가 |
| `DesignData/CombatRules.json` | `GroggyBonusDamage` 0.5, `BackAttackBonusDamage` 0.1, `AerialBonusDamage` 0.1 추가 |

- 대미지 처리는 GAS 객체(대미지 실행 계산) 안에 둔다는 [캐릭터 기반 구조와 GAS](<../../Docs/Architecture/캐릭터 기반 구조와 GAS.md>)의 배치를 따릅니다. 새 클래스, 태그, 어빌리티, 효과는 없고 클래스 구조도 바뀌지 않습니다.
- 수치는 모든 캐릭터에 공통인 전투 규칙이라 `CombatRules.json`에 둡니다. 로더는 속성 이름으로 읽으므로 로딩 코드는 바꾸지 않습니다.

## 2. 판정 구현

`Execute_Implementation`에서 극한 회피·무적으로 끝나는 경우를 지난 뒤 보너스 배율을 한 번 구하고, `HPMultiplier`에 곱해 패리·가드·일반 피격의 모든 HP 계산에 같은 값을 씁니다. PP 계산에는 곱하지 않습니다.

| 규칙 | 구현 |
| - | - |
| 플레이어의 공격만 | 컨텍스트 Instigator의 `GetCharacterData()->Team == EAgTeam::Player`. 반사탄은 `AAgProjectile::Reflect`에서 Instigator가 패리한 플레이어로 바뀌므로 자연히 포함됩니다. |
| 패리 대미지 제외 | 스펙 태그 `Effect.Damage.Parry` |
| 낙하 대미지 제외 | 낙하 대미지는 `UAgFallDamageMagnitude`를 쓰는 다른 효과라 이 실행 계산을 거치지 않습니다. |
| 처형 상태 제외 | 대상 태그 `State.Execution` |
| 그로기 | 대상 태그 `State.Groggy` |
| 에어리얼 | 대상 태그 `State.Airborne` (이동 모드가 떨어지는 중. 넉업·저글·점프·낙하를 모두 포함) |
| 백어택 | `UKismetAnimationLibrary::CalculateDirection(Causer 위치 − 대상 위치, 대상 회전)`의 절댓값이 135° 이상. 움찔의 등 모션 선택(`ResolveStaggerEvent`)과 같은 식이며, Causer가 투사체면 투사체 위치입니다. 두 줄이라 기존 두 곳(`ResolveStaggerEvent`, `RootMotion`의 `bKeepBack`)처럼 그 자리에 둡니다. |
| 적중 순간의 상태 | 대상 태그는 스펙에 캡처된 적용 순간의 태그를 씁니다. 그로기·넉업은 이 계산 뒤(어트리뷰트 세트와 결과 컴포넌트)에 시작되므로 그 타격에는 붙지 않습니다. |
| 반올림 | 기존 `CalculateDamage`가 배율을 모두 곱한 뒤 한 번 반올림합니다. |

- 가드 경로의 HP와 가드 SP 소모량에도 같은 배율이 들어가지만, 지금 가드하는 캐릭터는 플레이어뿐이라 플레이어의 공격이 가드되는 경우는 없습니다.

## 3. 인터페이스 & 함수 시그니처

```cpp
// AgDamageExecution.cpp 익명 네임스페이스
float GetBonusDamageRatio(const FGameplayEffectSpec& Spec, const AActor& Target, const FGameplayTagContainer& TargetTags, const UAgCombatRules& Rules);
```

```cpp
// AgCombatRules.h
UPROPERTY(Transient, VisibleAnywhere, Category = "Bonus Damage")
float GroggyBonusDamage = 0.f;

UPROPERTY(Transient, VisibleAnywhere, Category = "Bonus Damage")
float BackAttackBonusDamage = 0.f;

UPROPERTY(Transient, VisibleAnywhere, Category = "Bonus Damage")
float AerialBonusDamage = 0.f;
```

## 4. 문서 반영 (4단계)

- [전투 시스템](<../../Docs/Design/System/전투 시스템.md>): '대미지'에 '보너스 대미지' 절, 'HP 감소량 계산식'에 보너스 항
- [데이터 로딩과 스키마](<../../Docs/Architecture/데이터 로딩과 스키마.md>): `CombatRules.json` 행에 보너스 대미지 세 값
- [캐릭터 기반 구조와 GAS](<../../Docs/Architecture/캐릭터 기반 구조와 GAS.md>): `UAgDamageExecution` 행에 보너스 대미지

## 5. 검증

빌드 뒤 시나리오 테스트 창에서 샌드백·공격 샌드백으로 완료 조건 A1~A10을 확인합니다. 샌드백의 DEF가 0이라 기대값은 ATK × HP 배율 × (1 + 보너스 합)의 반올림입니다. A10은 테스트 창 메모리의 규칙 값만 바꿔 확인하고 `DesignData` JSON은 건드리지 않습니다.
