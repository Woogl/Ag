# 어빌리티 데이터 구조 테스트 기록

- 빌드와 데이터: 2026-11-03 03:09 빌드(이 작업의 C++ 전부 포함), 새 형식 JSON, `BP_Player`의 `JumpMontage`·`GuardFlinchMontage`.
- 방법: JSON 재로딩만 에디터 PIE에서, 나머지는 [시나리오 테스트](<../../Scripts/ScenarioTest/README.md>) 창에서 확인했습니다. 보스 스테이지와 시뮬레이션 룸(절단자·경비 사수·공격 샌드백)을 쓰고, 적 AI를 멈춘 뒤 전제는 테스트용 콘솔 명령(`AgPattern`, `AgGroggy`, `AgSetHP`, `AgFillMPUP`, `AgGod`)과 게임플레이 이벤트로 만들었습니다.
- 모든 창에서 `LogAg: Error`가 없었습니다. 에디터 시작 때 JSON 읽기 오류·경고도 없었습니다.

## 1-spec 3절 완료 조건

| 조건 | 결과 | 확인한 것 |
| - | - | - |
| 항목 하나에 모든 정의, `MontageTimelines.json`·캐릭터 `Montages` 없음 | 통과 | 변환 결과(어빌리티 39개, 타임라인 전부 이동), 두 정의가 없는 채로 읽기 오류 없음 |
| 입력 태그와 HUD | 통과 | 모든 행동 키가 `InputTag`로 발동. HUD 스킬 칸 비용 40, 쿨다운 중 남은 시간 표시, MP 20이면 칸이 흐려짐(0.5), UP 소모 뒤 궁극기 칸이 흐려짐 |
| 기본 공격 콤보 | 통과 | 1→2→3→4→5→1타, 가드 반격 뒤 첫 타가 Attack2 |
| 점프, 점프 공격, 착지 | 통과 | 점프 Start→Land(`BP_Player` 기본값 몽타주), 점프 공격 Start→Fall→Land |
| 회피, 재회피, 달리기 | 통과 | 입력 없음 Backstep, 이동 입력 Forward, 후딜 중 재회피, 극한 회피 구간 0.20초(두 섹션), 무적 0.32초, 회피 키를 누른 채 후딜 뒤 달리기(6.18~7.56초) |
| 극한 회피, 회피 반격 | 통과 | 보스 DoubleStrike 타격을 극한 회피, 보상 MP·UP 각 +10, 슬로우모션 배율 0.3, 회피 반격 기회와 회피 반격. 방향 앞·오른쪽·뒤·왼쪽·입력 없음 → Forward·Right·Backward·Left·Backward |
| 가드, 패리, 가드 반응, 가드 반격 | 통과 | 누르는 동안 가드, 공격·회피 후딜 뒤 누르고 있던 가드, 패리 모션 0.36초 뒤 가드 자세(가드는 유지), 가드 움찔(`BP_Player` 기본값 몽타주), 넉다운 타격의 가드 밀림, SP 0의 가드 브레이크, 가드 반격 |
| 스킬·궁극기, 처형, 락온 | 통과 | 스킬 MP 100→60, 쿨다운 중 발동 안 함, 쿨다운 뒤 60→20. 궁극기 UP 100→0, 시간 정지와 컷신 뒤 Judgment→Draw. 처형 카메라 3.54~7.46초. 락온 켜고 끔 |
| 움찔, 넉업, 저글, 넉다운 | 통과 | 움찔 부위 다섯(Back·Head·ArmRight·ArmLeft·Body), 공중 움찔 `AM_FlinchAir`, 지상 넉업 Rise, 공중 넉업 AirHit, 저글 AirHit, 착지 뒤 Land→GetUp→Stand, 넉다운 |
| 그로기, 처형 피격, 사망 모션, 페이즈 전환 | 통과 | 움찔 중 그로기 → 그로기 모션(Start→Loop), 넉다운 중 그로기는 모션 없음, 비전투 중 PP 0은 그로기 아님, 그로기 4.97초 뒤 PP 1500, 처형 피격 뒤 처형 중 사망 모션 `AM_Boss_ExecutedDeath`, HP 40%에서 페이즈 전환 |
| 적 패턴 | 통과 | 보스 8개(DashThrust의 Hold→Recover 포함), 절단자 3개, 경비 사수 PistolShot(Aim→Fire→Reload, 조준 구간), 공격 샌드백 |
| JSON 재로딩 | 통과 | 테스트 창에는 에디터 전용 재로딩(`UAgDataReloadSubsystem`)이 없어 에디터 PIE에서 확인. 회피가 Backstep·SP 20으로 발동한 뒤, `Abilities.json`에서 회피 비용을 SP 10으로, 입력 없을 때 시작 섹션을 Forward로 바꿔 저장하자 다음 회피가 Forward·SP 10을 씀. 원래 바이트로 되돌리자 다시 Backstep·SP 20 |
| JSON만으로 새 어빌리티 | 통과 | `Abilities.json`에 보스 패턴 `TestPattern`(`AM_Boss_Sweep`, 넉다운 타격 타임라인)을, `Characters.json` 보스 목록에 그 ID를 더함. `AgPattern TestPattern`으로 보스가 그 몽타주를 재생했고, 원래 Sweep의 넉업이 아니라 새 타임라인의 넉다운으로 플레이어가 쓰러짐(HP 1000→933). 확인 뒤 두 파일을 원래 바이트로 되돌림 |

## 2-plan 5절 순서가 바뀐 어빌리티

| 어빌리티 | 결과 | 확인한 것 |
| - | - | - |
| Execution | 통과 | 플레이어 처형과 보스 처형 피격 몽타주가 같은 프레임(3.536초)에 시작 |
| KnockUp, Juggle | 통과 | 지상 넉업이 AirHit가 아닌 Rise로 시작, 공중 넉업과 저글은 AirHit |
| GroundSlam | 통과 | Start→Loop→End, 착지 뒤 End |

## 실패와 다시 실행한 것

- 넉업·저글: 두 번째 이벤트가 정점(띄운 뒤 약 0.27초) 직후에 들어가 Rise→Fall→AirHit로 기록되어 기준(Rise 바로 뒤 AirHit)에 걸렸습니다. 기록에서 이벤트 다음 프레임에 AirHit가 시작된 것을 확인했고, 기준은 그대로 두고 이벤트만 정점 전으로 당겨 다시 실행해 통과했습니다.
- 그로기·적 패턴: 시나리오에서 파이썬 이름을 잘못 써서 실행 실패 → 고쳐 다시 실행했습니다.
- 경비 사수: 콘솔 명령은 가장 가까운 적을 고르는데, 사수에서 3m 지점에서는 다른 적이 더 가까워 패턴이 시작되지 않았습니다(배치 문제) → 락온으로 사수를 고르고 다시 실행했습니다.

기록은 `Saved/ScenarioTest/slot*_20261103-*.json`에 있습니다.
