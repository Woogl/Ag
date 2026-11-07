# 스킬 연계 기술 설계 (Plan)

## 1. 영향 범위 및 아키텍처

| 대상 | 변경 |
| - | - |
| `AgGameplayTags` | `State.Invincible.Downed`(넉업·넉다운으로 누워 있는 동안의 무적)를 네이티브로 선언합니다. |
| `UAgDamageExecution` | 무적 검사(`IsInvincible`)에서, 대상의 무적이 누워 있는 동안의 무적뿐이면(`State.Invincible`을 직접 가진 효과가 없음) 대상이 플레이어 팀일 때만 무적으로 봅니다. 적은 누워 있어도 일반 처리로 넘어갑니다. |
| `AAgEnemyCharacter::CanLosePP` | `State.Invincible`을 정확히 가졌을 때만 PP 감소를 막습니다. 누워 있는 동안의 무적은 적을 지키지 않으므로, 누운 적이 맞으면 PP도 줄어듭니다. |
| `UAgAbilityTask_Teleport` | 나타날 자리를 찾는 기준 방향을 대상의 등 뒤(대상 정면의 반대)에서 '순간이동하는 캐릭터 → 대상' 방향(대상 너머)으로 바꿉니다. 둘레를 도는 순서와 자리 조건은 그대로이고, 두 캐릭터가 겹쳐 방향이 없으면 캐릭터 정면을 씁니다. |
| `AAgCharacterBase` | 무기 채널 캡슐 응답을 '넉업으로 떠 있음'에서 '넉업·넉다운 피격 반응 중'으로 넓힙니다. 처리 함수 `HandleAirborneKnockUpChanged`를 `HandleWeaponCapsuleTagChanged`로 바꾸고, `State.HitReaction.KnockUp`·`State.HitReaction.KnockDown` 태그 이벤트에 등록합니다. 누운 몸은 바닥에 붙어 무기 피직스 바디를 비껴가므로 떠 있는 대상과 같이 캡슐로 맞힙니다. 누워 있는 플레이어는 캡슐에 닿아도 대미지 실행이 막습니다. |

- 누워 있는 동안의 무적을 `State.Invincible`의 하위 태그로 두어, 부모 태그를 보는 기존 검사는 그대로 무적으로 다룹니다. 플레이어만 지키는 예외는 대미지 실행과 PP 감소 조건 두 곳에만 둡니다. 플레이어는 PP를 잃지 않으므로 PP 조건은 적에게만 해당합니다.
- 피격 반응은 기존 규칙(`AgDamageResultGameplayEffectComponent`)을 그대로 씁니다. 누운 대상에게 움찔은 반응 이벤트 없이 흔들림만, 넉업은 넉업 어빌리티를 다시 발동해 누운 자리에서 띄웁니다.
- `Docs/Architecture/` 공통 규칙: 태그는 네이티브 선언, 기획 값은 JSON 정본, 몽타주 안의 규칙 시각은 `Abilities.json` 시간표를 따릅니다.

## 2. GAS 및 데이터 설계

| 파일 | 변경 |
| - | - |
| `Effects.json` | `DownedInvincible` (Infinite, `State.Invincible.Downed`) |
| `Abilities.json` `KnockUp`·`Juggle`(`Land`)·`KnockDown` | 누워 있는 구간의 `Invincible` → `DownedInvincible` |
| `Abilities.json` `Skill4` | `HitReaction: KnockUp`, Busy 끝을 내려찍은 직후로 |
| `Abilities.json` `Skill1` | 사라지기까지 0.12 → 0.08초에 맞춰 그 뒤 시각을 0.04초 당김, 찌르기 판정 끝과 Busy 끝을 0.45초로 맞춤, 나타난 뒤의 RootMotion 구간 `bWarpTranslation: true` |
| `Abilities.json` `Skill3` `Attack2` | Busy 끝과 Transition 시작을 0.43 → 0.32 (몽타주 위치) |
| `Abilities.json` `Skill2` | Busy 끝을 올려베기 판정이 끝난 직후로, RootMotion 구간을 0 ~ 0.2초와 0.2 ~ 0.3167초(`TargetingRange` 400)로 나눔 |

몽타주 변경 (몽타주 재생 위치 기준):

| 몽타주 | 변경 |
| - | - |
| `AM_Player_Skill1` | 첫 조각(`Attack09` 0 ~ 0.2초)을 0 ~ 0.12초 1.5배속(몽타주 0 ~ 0.08초)과 0.12 ~ 0.2초 1배속(보이지 않는 0.08초)으로 나눕니다. 노티파이(사라지는 순간의 큐, 찌르기 큐)를 0.04초 당깁니다. |
| `AM_Player_Skill` | 칼 VFX 노티파이 상태의 끝을 새 후딜 시작에 맞춥니다(기획서 '동작 시작부터 후딜 시작까지'). |

## 3. 인터페이스 & 함수 시그니처

```cpp
// AAgCharacterBase
void HandleWeaponCapsuleTagChanged(const FGameplayTag Tag, int32 NewCount);
```
