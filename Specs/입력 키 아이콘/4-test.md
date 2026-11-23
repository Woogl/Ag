# 입력 키 아이콘 검증 결과

## 1. 테스트 시나리오

- 테스트 창(별도 게임 프로세스)을 띄워 창에 파이썬 명령을 보내 확인했습니다. 시나리오 실행기는 플레이어 폰이 없는 타이틀 맵을 열지 못해, 같은 원격 실행 포트로 직접 명령을 보냈습니다(임시 도구, 저장소에 두지 않음).
- Esc는 확인용 키 명령(`AgPressKey`)이 플레이어 컨트롤러에 바로 들어가 CommonUI를 거치지 않으므로, 테스트 창에 Windows 키 메시지(`WM_KEYDOWN`/`WM_KEYUP`, Esc)를 보내 사람의 키와 같은 경로로 넣었습니다. 창의 포커스는 가져오지 않습니다.
- 회귀: `python -I Scripts/ScenarioTest/run.py Saved/ScenarioTest/Scenarios/player_execution.py`, `interaction_priority.py`

## 2. 결과

- 빌드: AgEditor Development 성공.
- 타이틀(`LV_FrontEnd`, 1920×1080)
  - [통과] 팝업을 열기 전 `IA_Move` 키 없음, `IA_Back` = Escape(`IMC_UI`가 늘 걸림)
  - [통과] Controls를 열면 `IA_Move` = W·A·S·D, 조작 안내 15개 아이콘이 모두 보임: 이동 `T_Key_WASD`(156×36), 시야 마우스 이동, 기본 공격·가드·락온 좌·우·휠 클릭, 회피 Shift, 점프 Space, 스킬 1~4, Q, F, R, 일시정지 Esc
  - [통과] Confirm으로 닫으면 `IA_Move` 키가 다시 없어지고 `IA_Back`은 남음. 다시 열면 15개 모두 보임
  - [통과] Controls를 열고 Esc → 팝업만 닫힘. Play로 스테이지 선택을 열고 Esc → 닫히고 타이틀로 돌아옴
- 시뮬레이션 룸(1920×1080)
  - [통과] HUD 스킬 1~4 → `T_Key_One`~`Four`, 회복병 → `T_Key_R`, 36px로 보임
  - [통과] 보스 그로기(`AgTargetGroggy`) 뒤 처형 안내에 `T_Key_F`와 '처형'이 보임(48px 칸)
  - [통과] Esc → 일시정지 메뉴가 열리고 게임이 멈춤, 다시 Esc → 닫히고 게임이 이어짐
- 로그: 테스트 창 `LogAg: Error` 0건, CommonUI 바인딩 오류 없음.
- 그림: 조작 안내 팝업과 HUD·처형 안내 캡처(`Saved/Screenshots/WindowsEditor/ScreenShot01587.png`, `ScreenShot01589.png`). 밝은 회색 바닥 위 처형 안내의 F, HUD의 키캡 글자가 읽힘.

## 3. 회귀 확인

- `player_execution.py` 통과(그로기 → 처형 → 처형 피격 사망, 처형 카메라).
- `interaction_priority.py` 통과(상호작용 키로 처형 실행, 처형 뒤 성문 안내, 다시 눌러 성문 열림).
- 사람 손 플레이와 PIE는 하지 않았습니다.
