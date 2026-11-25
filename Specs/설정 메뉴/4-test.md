# 설정 메뉴 검증 결과

## 1. 테스트 시나리오

- 타이틀, 일시정지 메뉴, 적 선택 화면은 시나리오 실행기가 다루지 않으므로, `Scripts/ScenarioTest/run.py`의 창 띄우기·원격 실행 함수를 가져다 쓰는 임시 드라이버(스크래치패드, 저장소 밖)로 확인했습니다. 키는 테스트 창에 `WM_KEYDOWN`·`WM_KEYUP`을 보내 CommonUI 메뉴에 넣었고, 화면은 `shot showui`로 찍었습니다.
- 테스트 창이 바꾼 창 모드·해상도·언어는 에디터와 같은 `Saved/Config/WindowsEditor/GameUserSettings.ini`에 저장되므로, 실행 전에 백업하고 끝나면 되돌렸습니다. 중간에 한 번 테스트 창이 바꾼 값이 백업에 들어가, 마지막에 처음 값(800 × 450, `FullscreenMode=1`, 언어 줄 없음)으로 직접 되돌렸습니다.
- 회귀: `python -I Scripts/ScenarioTest/run.py Saved/ScenarioTest/Scenarios/item_potion.py`.

## 2. 결과

- 빌드: 임시 사본 빌드 성공, 작업 트리 빌드 세 번 성공(구현, 아래 수정 두 가지 뒤, 표준 해상도·Borderless 삭제 뒤). 새 에디터 로그에 `LogAg` 오류 없음.
- 확인 중에 찾아 고친 것:
  - 분류 목록에서 →가 드롭다운으로 가지 않음. 내비게이션은 포커스를 받는 위젯으로만 옮기는데 대상이 드롭다운을 감싼 사용자 위젯이었음 → 그 안의 콤보 상자까지 따라 내려가게 고침.
  - 일시정지 메뉴에서 다른 탭에 갔다가 Settings로 돌아오면 Controls가 골라지지만 포커스는 이전 분류(Display)에 남음. CommonUI가 켜짐 알림 때 첫 포커스를 정하는데 그 뒤에 분류를 되돌렸기 때문 → 부모 함수 전에 고르게 고침.
  - 분류 이름 사이 간격이 0. 위젯 블루프린트 값(테두리 여백과 자식 칸 여백이 같은 값) → 12px씩으로 고침.

| 완료 조건 | 판정 | 근거 |
| - | - | - |
| 타이틀 버튼이 Play, Settings, Quit | 통과 | 버튼 목록 `PlayButtonBox`, `SettingsButtonBox`, `QuitButtonBox` |
| 타이틀 Settings → Controls를 고른 Settings 화면, Back·Esc로 닫으면 Settings 버튼 선택 | 통과 | 열 때 분류 0, 분류 이름 포커스, 타이틀 화면은 꺼짐. Esc 뒤 타이틀의 Settings 버튼 포커스 |
| 일시정지 메뉴 탭 Inventory, System, Settings, 열 때 System, Q·E로 오가고 끝에서 넘어감 | 통과 | 탭 3개, 열 때 1, E → 2, Q → 1, 2에서 E → 0 |
| Settings 탭을 표시할 때마다 Controls | 통과 | Display로 바꾸고 System에 갔다 오면 분류 0, 포커스 Controls(수정 뒤) |
| 좌클릭·↑·↓로 분류가 바뀜, 내용 칸과 Back 위치 그대로 | 통과(↑·↓, 위치) | ↓로 0→1→2, 마지막 분류에서 ↓는 멈춤. 분류마다 찍은 화면에서 Back과 내용 칸 위치가 같음. 좌클릭과 마우스 올림은 엔진 버튼 동작이라 따로 누르지 않음 |
| Controls에 조작 안내와 같은 줄, 키 아이콘 | 통과 | 타이틀 Settings 화면과 일시정지 Settings 탭 화면 모두 12줄과 키 아이콘 |
| Window Mode가 Fullscreen, Windowed 둘이고 고르면 적용 | 일부 통과 | 목록 `Fullscreen`, `Windowed`(Borderless 뺀 뒤). Windowed 적용은 Borderless를 빼기 전 빌드에서 확인(창 크기가 바뀜). Fullscreen 적용은 사람의 화면 전체를 덮어 실행하지 않음 |
| Resolution 목록이 표와 규칙대로, 적용, UI 크기 맞춤 | 통과(Windowed), 계산(Fullscreen) | 2560 × 1440 모니터에서 Windowed 목록 8개(1920 × 1200, 1920 × 1080, 1680 × 1050, 1600 × 900, 1440 × 900, 1366 × 768, 1280 × 800, 1280 × 720). Fullscreen은 표와 엔진의 모니터 지원 해상도를 맞춰 9개(모니터 해상도 포함)로 계산만 함. 고른 해상도 적용(창 1280 × 720)은 표준 목록 전 빌드에서 확인 |
| 창 모드를 바꾸면 목록에 없는 해상도는 가장 큰 것으로 | 통과 | Windowed로 바꾸자 그 목록의 가장 큰 해상도(표준 목록 전 빌드) |
| 다시 실행하면 바꾼 값, 처음은 Fullscreen·모니터 해상도 | 일부 통과 | 해상도 명령줄 없이 다시 띄우자 저장한 Windowed 1280 × 720으로 시작. 처음 실행(저장값 없음)은 사람의 화면을 Fullscreen으로 덮어 실행하지 않음. 엔진이 `Config/DefaultGameUserSettings.ini`의 값을 읽는 길(`PreloadResolutionSettings`의 `Version` 확인)은 소스로 확인 |
| 언어를 바꾸면 바로 적용 | 통과(타이틀) | Language에서 English를 고르자 현재 언어 `en`. 일시정지 메뉴에서의 언어 바꾸기는 실행하지 않음(같은 위젯) |
| 드롭다운 목록이 열려 있을 때 Esc는 목록만 닫음 | 확인 못 함 | 그 단계를 넣은 실행이 다른 이유로 중간에 끊김 |
| 스테이지 선택·Settings·적 선택 화면이 같은 양식, 타이틀은 레벨, 전투 결과 UI 그대로 | 통과(세 화면) | 세 화면과 일시정지 Settings 탭 화면에서 검은 배경, 같은 자리의 밑줄 제목, 같은 내용 높이. 전투 결과 UI는 고치지 않음 |

- 로그: 테스트 창 로그의 `LogUIActionRouter` 줄로 첫 포커스 대상을 확인했습니다(수정 전 `Focused desired target WBP_CategoryButton_C_1`, 수정 뒤 Controls). `-game`의 `LogPython: Error`(엔진 툴셋 `init_unreal`)는 테스트 창의 알려진 잡음입니다.
- 테스트 환경의 잡음: 메뉴를 열면 CommonUI가 사람의 실제 커서를 창 가운데로 옮기고, 키를 빠르게 연달아 보내면 포커스가 엉뚱한 버튼으로 가는 실행이 있었습니다(한 번은 Back → 타이틀 → Quit까지 이어져 창이 정상 종료됨). 키 사이를 0.4초 이상 두면 같은 흐름이 기대대로 동작했습니다. 해상도를 고른 직후 포커스가 드롭다운에 남는지는 이 잡음 때문에 확정하지 못했습니다. 엔진은 실행 중 해상도 변경에서 포커스를 옮기지 않습니다(`SetAllUserFocusToGameViewport`는 처음 창을 띄울 때만 부름).

## 3. 회귀 확인

- `item_potion`(HUD의 회복병 칸과 보유 수 포함): 통과 13/13.
- 타이틀 → Play → 스테이지 선택 → Start: `LV_FortressBreach`가 열리고 HUD가 켜짐.
- 시뮬레이션 룸: 구성 옵션으로 들어가 HUD와 일시정지 메뉴 확인, 옵션 없이 들어가 적 선택 화면 표시. 적을 골라 Start하는 흐름은 실행하지 않음(이 작업에서 바꾸지 않은 코드).
- 사람 손 플레이, PIE, 패키지는 하지 않음.
