# 스테이지 선택 검증 결과

## 1. 테스트 시나리오

- 방식: 에디터 PIE 창 하나(LV_FrontEnd에서 시작)를 에디터 파이썬으로 조종합니다. 버튼은 CommonUI 내부 버튼의 클릭을 보내 누르고, 드롭다운은 엔진 `SetSelectedOption`으로 고르며, 화면은 활성 상태(`IsActivated`)로 확인합니다.
- PIE에서는 Esc가 PIE를 끝내므로 Esc(시나리오 2·9의 Esc)와 방향키·마우스 조작(시나리오 9), 일시정지 메뉴(시나리오 7)는 확인하지 않습니다. 레벨을 새로 만드는 시나리오 10의 뒷부분도 확인하지 않습니다.
- 사망은 테스트용 콘솔 명령(`AgKillBoss`, `AgKillPlayer`)으로 만듭니다.

| 확인 | 해당 시나리오 | 통과 조건 |
| - | - | - |
| D1 | 1, 10 | Start를 누르면 레벨이 LV_FrontEnd 그대로이고, 스테이지 선택 화면이 활성, 타이틀 화면은 비활성입니다. 드롭다운 항목은 Boss Stage, Duel Corridor, Simulation Room 순서이고(레벨이 없는 Fortress Breach, Temp Dungeon 3은 없음), Boss Stage가 골라져 닫힌 칸에 그 이름이 보입니다. 처음 선택 대상은 드롭다운입니다. 화면 캡처에서 제목, 드롭다운, 그 아래 Start·Back 가로 배치가 보입니다. |
| D2 | 2 | Duel Corridor를 고른 뒤 Back을 누르면 스테이지 선택 화면이 비활성, 타이틀 화면이 활성입니다. 다시 Start를 누르면 항목이 3개 그대로(중복 없음)이고 Boss Stage가 골라져 있습니다. |
| D3 | 3, 6 | Boss Stage로 Start를 누르면 LV_BossStage가 열리고 보스가 하나, 전투 HUD가 활성입니다. `AgKillPlayer` 뒤 'GAME OVER'에서 Return to Title을 누르면 타이틀 화면만 활성입니다. |
| D4 | 4, 5, 6 | Duel Corridor로 Start를 누르면 LV_DuelCorridor가 열리고 플레이어가 빙의되어 있으며 전투 HUD가 활성, 마우스 커서가 꺼져 있습니다. 결과 흐름(Stage Clear·Restart·Game Over·Return to Title)은 1차 실행에서 확인했고 그 뒤 바뀐 코드가 없습니다. |
| D8 | 8 | Simulation Room으로 Start를 누르면 LV_SimulationRoom이 열리고 적 선택 화면이 활성입니다. 그 Start 버튼은 잠겨 있고 투명도 0.5입니다(시뮬레이션 룸 흐린 버튼 회귀 확인). |

## 2. 결과

### 1차 실행 (버튼 목록 화면, 2026-11-02)

드롭다운으로 바꾸기 전 화면에서 실행했습니다. 화면 확인은 2차 실행으로 대신하고, 레벨 흐름 결과만 남깁니다.

| 확인 | 결과 | 기록 |
| - | - | - |
| 보스전 진입 | 통과 | LV_BossStage, 보스 1, HUD 활성, 플레이어 빙의 |
| 보스전 Game Over → 타이틀 | 통과 | 'GAME OVER' 뒤 Return to Title → 타이틀만 활성 |
| 던전 진입 | 통과 | LV_DuelCorridor, 플레이어 빙의, HUD 활성, 커서 꺼짐 |
| 던전 Stage Clear → Restart | 통과 | 맵에 일반 적이 아직 배치되어 있지 않아 PIE 메모리에만 절단자를 `summon`으로 두고 확인. `AgKillBoss` 뒤 'STAGE CLEAR', 절단자 생존. Restart 뒤 같은 맵, 결과 화면 없음 |
| 던전 Game Over → 타이틀 | 통과 | 'GAME OVER' 뒤 Return to Title → 타이틀만 활성 |
| 흐린 버튼 | 실패 | 버튼이 위젯 상태 알림의 잠금 비트로 흐림을 정했으나 투명도가 1.00. 잠금 상태 등록 클래스는 CommonUI에 있고 UMG의 위젯 상태 설정이 그보다 먼저 만들어져 잠금 비트가 비어 있는 것으로 추정. 드롭다운으로 바꾸며 이 방식은 되돌림 |

### 2차 실행 (드롭다운 화면, 2026-11-02)

- 빌드: AgEditor Development 성공.
- 판정: 통과 (D1·D2·D3·D4·D8).
- 로그: 실행 중 `LogAg` 오류·경고 없음.

| 확인 | 결과 | 기록 |
| - | - | - |
| D1 | 통과 | Start 뒤 LV_FrontEnd 그대로, 스테이지 선택 활성·타이틀 비활성. 항목 키 0·1·4(Boss Stage, Duel Corridor, Simulation Room), 고른 항목 0, 닫힌 칸 'Boss Stage', 처음 선택 대상 드롭다운. 화면 캡처에서 제목, 드롭다운, Start·Back 가로 배치 확인 |
| D2 | 통과 | Duel Corridor를 고르고 Back → 타이틀 활성. 다시 열면 항목 0·1·4, 고른 항목 0 |
| D3 | 통과 | LV_BossStage, 보스 1, HUD 활성, 커서 꺼짐. 'GAME OVER' 뒤 Return to Title → 타이틀만 활성 |
| D4 | 통과 | LV_DuelCorridor, 플레이어 빙의, HUD 활성, 커서 꺼짐. Game Over 뒤 Return to Title → 타이틀만 활성 |
| D8 | 통과 | LV_SimulationRoom, 적 선택 화면 활성, Start 잠김·투명도 0.50 |

### 3차 실행 (드롭다운 공용화 뒤, 2026-11-03)

- 빌드: AgEditor Development 성공.
- 판정: 통과 (D1·D2·D8). D3·D4는 바뀐 코드가 없어 다시 돌리지 않았습니다.
- 로그: 실행 중 `LogAg` 오류·경고 없음.

| 확인 | 결과 | 기록 |
| - | - | - |
| D1 | 통과 | 스테이지 선택 활성·타이틀 비활성. 항목 키 0·1·4, 고른 항목 0, 닫힌 칸 'Boss Stage', 처음 선택 대상은 드롭다운의 콤보 상자. 화면 캡처에서 드롭다운 너비가 아래 버튼 줄과 같음 |
| D2 | 통과 | Duel Corridor를 고르고(닫힌 칸 'Duel Corridor') Back → 타이틀 활성. 다시 열면 항목 0·1·4, 고른 항목 0, 닫힌 칸 'Boss Stage' |
| D8 | 통과 | LV_SimulationRoom, 적 선택 화면 활성, Start 잠김·투명도 0.50 |

## 3. 회귀 확인

- 확인한 기존 기능과 결과: 시뮬레이션 룸 Start 흐림(D8 통과), 보스전·던전의 결과 화면과 Return to Title(통과).
- 사람 확인: 2026-11-02 프로그래머가 Standalone Game에서 확인했습니다. 버튼 줄이 몰려 보여 간격을 넓혔습니다(드롭다운과 버튼 줄 사이 96, 두 버튼 사이 48, 버튼 너비 228).
- 확인하지 않은 것: 새 레벨을 만들었을 때 목록에 나오는지(아직 만든 레벨이 없음), 패키지 게임에서 스테이지 맵이 쿡되는지.
