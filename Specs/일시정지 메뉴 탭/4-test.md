# 일시정지 메뉴 탭 검증 결과

## 1. 테스트 시나리오

- 테스트 창(`Scripts/ScenarioTest/run.py`의 창 띄우기와 원격 실행을 임시 도구로 써서 시나리오 없이 띄움), `LV_SimulationRoom?Enemies=,,Boss`. 적 AI 정지(`AgStopAI`)와 무적(`AgInvincible`) 뒤 확인.
- 메뉴 입력(Esc·Q·E·방향키·Enter)은 창에 키 메시지(`WM_KEYDOWN`/`WM_KEYUP`)를 보내 CommonUI가 실제 키로 받게 함. 게임 입력(궁극기 Q, 회복병 R)은 확인용 입력(`AgPressKey`/`AgReleaseKey`).
- 탭 이름과 칸의 좌클릭·마우스 올림은 버튼 내부 버튼의 `on_clicked`·`on_hovered`를 직접 불러 확인(창에 마우스 메시지를 보내면 사람의 실제 커서 위치가 쓰임).
- 여러 칸: 사용자 승인으로 `DesignData/Items.json`에 시험용 재화 아이템 30개를 잠깐 넣었다가 원래 바이트로 되돌림(해시 `bec1b871…` 일치).
- 상태는 원격 파이썬으로 위젯을 읽고, 모양은 `shot showui` 화면 캡처로 봄.

## 2. 결과

- 빌드: AgEditor Development 성공.
- 판정: 완료 조건 모두 통과(효과음 제외, 아래).

| 완료 조건 | 결과 |
| - | - |
| Esc로 열면 게임이 멈추고 System 탭에 Resume 선택, 탭 바에 Q·Inventory·System·E, System 밑줄 | 통과. 고른 탭만 밑줄이 보이고 탭 이름은 포커스를 받지 않음. 처음 열 때, Inventory에서 닫고 다시 열 때, System에서 닫고 다시 열 때 모두 CommonUI가 System 탭을 바로 켜고 Resume에 포커스를 줌(창 로그 `LogUIActionRouter`의 `Focused desired target ResumeButton`) |
| 열려 있는 동안 플레이어 상태·슬롯·보스 상태 숨김, 닫으면 다시 표시 | 통과(`StatusPanel` 숨김 → 다시 표시) |
| 시작 직후 Q로 바꾼 Inventory 탭의 첫 칸에 회복병 아이콘과 3, 상세 패널에 회복병·소비 아이템·보유·3 / 3·설명·큰 아이콘 | 통과 |
| 한 번 마신 뒤 칸 2, 상세 2 / 3. 모두 없애면 빈 그리드, 상세 패널 없음 | 통과(남은 둘은 `AgRemoveItem Potion 2`로 없앰) |
| 마우스를 올리면 선택이 바뀌지 않고, 좌클릭하면 그 칸을 고름 | 통과. 사람의 마우스가 지나갈 때 옅은 테두리만 생기는 것도 캡처에서 봄 |
| 방향키로 칸 사이 이동, 줄 끝 ←·→ 멈춤 | 통과. 7번째 칸에서 → 멈춤, 첫 칸에서 ← 멈춤, ↓ 한 줄씩, 5번째 줄로 내려가면 한 줄 스크롤, ↑로 돌아오면 스크롤 0 |
| System에서 Q로 Inventory(첫 칸), E로 System(Resume 선택), System에서 E로 Inventory, Inventory에서 Q로 System | 통과(끝에서 처음으로, 처음에서 끝으로 넘어감) |
| 탭 이름 좌클릭으로 탭 바꾸기 | 통과 |
| 두 탭에서 Esc로 닫고 이어 감, 다시 열면 System | 통과(빈 Inventory에서도 닫힘) |
| System 탭의 Resume·Restart·Return to Title·Quit | 통과(Enter로 Resume 닫힘, Restart는 시뮬레이션 룸 다시 열림·멈춤 풀림, Return to Title은 타이틀, Quit은 게임 종료) |
| System 탭에서 방향키는 버튼 사이로만 | 통과(양 끝에서 멈추고 탭 바로 가지 않음) |
| 메뉴를 닫은 전투 중 Q로 궁극기 | 통과(`State.Cutscene`과 궁극기 몽타주). Q는 `IA_Ultimate`과 `IA_PreviousTab` 둘 다에 매핑되어 있음 |

- 확인하지 못한 것: 효과음(테스트 창은 소리 없이 뜸). 탭 바꾸기 소리는 버튼 스타일의 '누를 때' 소리로 넣었고 엔진 코드(`SetSelectedInternal`)로만 확인함. 사람 손 플레이·PIE·패키지 안 함.
- 테스트 창의 함정: 메뉴를 열면 숨었던 커서를 CommonUI가 뷰포트 가운데로 옮기는데, 배경의 테스트 창에서는 사람의 실제 커서가 창 가운데로 옮겨지고, 그 자리의 버튼(창 크기에 따라 Return to Title)에 마우스 올림이 생겨 선택이 옮겨감. 사람이 커서를 창 위로 지나갈 때도 같음. 그래서 처음 선택은 창 로그의 포커스 기록과 커서가 창 밖에 있던 회차로 판정함.
- 고친 것: 기본 탭(System)을 메뉴가 켜진 뒤에 고르면, Inventory에서 닫고 다시 열 때 지난번 탭(Inventory)이 먼저 켜졌다가 바뀌어 Resume에 포커스가 남지 않음 → 탭 바가 화면에 붙을 때(메뉴가 켜지기 전) 고르도록 옮김.
- 고친 것: 처음 만든 그리드는 4줄을 넘어 스크롤 막대가 생기면 한 줄에 6칸이 됨 → 칸 영역을 막대 몫(두께 16 + 둘레 2×2)만큼 넓힘.
- 로그: 테스트 창의 `LogAg: Error` 없음.

## 3. 회귀 확인

- `Saved/ScenarioTest/Scenarios/item_potion.py`: 통과(13항목).
- `Saved/ScenarioTest/Scenarios/stage_boss_clear.py`: 통과(3항목).
