# 가드 반격 기술 설계 (Plan)

## 1. 영향 범위 및 아키텍처

회피 반격의 구조를 그대로 따릅니다. 새 클래스와 새 Task는 없습니다.

| 대상 | 변경 |
| - | - |
| `UAgDamageResultGameplayEffectComponent` | 피격 결과가 패리 또는 가드이고 막은 쪽이 살아 있으면, 반응 처리(패리 이벤트, 가드 반응) 뒤에 막은 쪽에 효과 `GuardCounterChance`를 겁니다. 효과 ID는 `GuardFlinch` 몽타주 ID처럼 익명 네임스페이스의 상수로 둡니다. |
| `AAgPlayerCharacter::HandleAbilityActivated` | 기회를 지우는 대상을 `State.DodgeCounterChance`와 `State.GuardCounterChance` 둘로 넓히고, 지우는 어빌리티에 가드 브레이크(`Ability.Reaction.HitReaction.GuardBreak`)를 더합니다. |
| `AAgPlayerCharacter::RunInput` | 기본 공격 키 처리에서 회피 반격 기회 다음에 가드 반격 기회를 확인해 `Ability.Action.GuardCounter`를 발동합니다. |
| `AAgPlayerCharacter::PlayDamageCameraEffects` | 가드 반격 적중이면 `EAgCameraSituation::GuardCounterHit`을 재생합니다. |
| `EAgCameraSituation` | `GuardCounterHit`(가드 반격 적중)을 더합니다. `BP_Player`의 `CameraEffects`에 회피 반격 적중과 같은 값으로 넣습니다. |

- 기회가 사라지는 규칙은 지금 코드 구조에서 그대로 성립합니다. 가드를 새로 시작하면 `Ability.Action.Guard`가 발동해 지워집니다. 패리 모션 후딜이나 가드 밀림 끝에 가드 자세로 돌아갈 때는 가드 어빌리티가 계속 켜져 있어 새로 발동하지 않으므로, 기회가 남습니다.
- 패리는 `Event.Parry`로 패리 어빌리티(`Ability.Action`)가 발동하며 기존 기회를 지웁니다. 컴포넌트는 이벤트를 보낸 뒤에 효과를 걸므로 새 기회가 남습니다.
- 패리 모션과 가드 밀림 중에 누른 기본 공격 키는 지금의 선입력이 처리합니다. 후딜 또는 모션 끝에 `TryRunPendingInput`이 기억한 입력을 먼저 실행하고, 그 뒤에 가드로 돌아갑니다.
- [캐릭터 기반 구조와 GAS](<../../Docs/Architecture/캐릭터 기반 구조와 GAS.md>) '효과를 거는 객체'에 "가드 반격 기회는 피격 결과 처리 컴포넌트가 겁니다"를, [데이터 로딩과 스키마](<../../Docs/Architecture/데이터 로딩과 스키마.md>) '효과에 두는 값'의 목록에 가드 반격 기회를 더합니다. 클래스 관계는 바뀌지 않습니다.

## 2. GAS 및 데이터 설계

### 태그 (`AgGameplayTags`)

| 태그 | 주석 |
| - | - |
| `Ability.Action.GuardCounter` | |
| `State.GuardCounterChance` | 가드 반격 기회 |

### JSON

| 파일 | 변경 |
| - | - |
| `Effects.json` | `GuardCounterChance`: `HasDuration`, `Duration` 1, `GrantedTags` `State.GuardCounterChance` |
| `Abilities.json` | `GuardCounter`: `DodgeCounter`와 같은 형식이고, `RequiredTags`만 `State.GuardCounterChance`, 몽타주는 `GuardCounter`입니다. |
| `Characters.json` | 플레이어 `Abilities`에 `GuardCounter`, `Montages`에 `GuardCounter` → `AM_Player_GuardCounter` |
| `MontageTimelines.json` | `AM_Player_GuardCounter`: RootMotion(0 ~ 판정 시작, `TargetingRange` 800, Slerp, 이동 워핑), Invincible(0 ~ 판정 끝), Busy(0 ~ 판정 끝 + 0.08), WeaponHit(내려베기 시작 ~ 올려베기 끝, `HitReward` MP 8, HP·PP 배율 1.5, `Flinch`, 가드 가능) |
| `MontageTimelines.json` | `AM_Player_DodgeCounter`: HP·PP 배율 1.5, `TargetingRange` 800 |

- 가드 반격 시각은 `Attack_Combo4`의 베는 시점 0.43 ~ 0.48초를 씁니다.

### 에셋

| 에셋 | 변경 |
| - | - |
| `AM_Player_GuardCounter` | `AM_Player_DodgeCounter`를 복제하고 시퀀스를 `ARPG_Samurai_Anim_UE_Attack_Combo4`로 바꿉니다. 블렌드 인은 회피 반격과 같게 둡니다. 무기 궤적은 판정 0.1초 전부터 판정 끝 0.05초 뒤까지입니다. || `BP_Player` | `CameraEffects`에 `GuardCounterHit`을 넣고, 값은 `DodgeCounterHit`과 같게 합니다. |

## 3. 인터페이스 & 함수 시그니처

새로 추가하거나 바꾸는 함수 선언은 없습니다. 기존 함수의 본문만 바꿉니다.
