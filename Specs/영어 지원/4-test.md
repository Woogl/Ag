# 영어 지원 검증 결과

## 1. 테스트 시나리오

- 테스트 창(`python -I Scripts/ScenarioTest/run.py <시나리오.py>`, 시나리오는 작업 폴더 밖의 임시 파일)
  - 시작 언어와 문구: 시작 언어를 읽고, 문구 표 27개를 한국어로 읽은 뒤 영어로 바꿔 다시 읽고, 한국어로 되돌립니다. JSON에서 읽은 캐릭터 이름도 함께 봅니다.
  - 저장: 한 창에서 영어로 바꿔 저장하고, 다시 띄운 창의 시작 언어를 읽습니다.
  - 번역이 없는 언어: 설정 파일의 저장 언어를 fr로 바꿔 띄운 창의 시작 언어를 읽습니다.
- PIE(`LV_FrontEnd`): 타이틀 → Settings → 드롭다운에서 English 선택(콤보 박스 선택 이벤트) → Confirm → Controls. 단계마다 `shot showui`.
- 패키지(Development): `-ExecCmds="Localization.DumpLiveTable -Namespace=StringTable"`로 인자 없이 한 번, `-culture=en`으로 한 번 띄워 로그의 표시 문구를 봅니다.

## 2. 결과

- 빌드: 에디터·패키지 성공. 패키지 첫 빌드는 에디터 전용 미리 보기 함수를 `WITH_EDITOR` 밖에서 불러 실패했고, 고친 뒤 성공했습니다.
- 판정:
  - [x] 한국어 윈도우, 고른 적 없음 → 한국어로 시작(테스트 창 `ko`, 패키지 '린').
  - [x] 번역이 없는 언어 → 영어로 시작(테스트 창, 저장 언어 fr → `en`). 실제 영어 윈도우에서는 확인하지 못했습니다(이 PC는 한국어 윈도우).
  - [x] 타이틀 버튼이 위에서부터 Play, Controls, Settings, Quit.
  - [x] Settings 팝업: 제목 'Settings', 'Language', '한국어' 드롭다운, Confirm. 뒤의 타이틀은 어둡습니다.
  - [x] English를 고르면 드롭다운이 'English'로 바뀌고, Controls 팝업의 조작 이름이 Move, Basic Attack, Guard / Parry … Lock-On / Release, Pause로 보입니다.
  - [x] 문구 표 27개가 영어로 바뀌고 한국어로 되돌아옵니다. JSON에서 읽은 이름도 바로 바뀝니다(린 → Rin).
  - [x] 영어로 저장 → 다시 띄운 창이 영어로 시작.
  - [x] 패키지: 인자 없이 한국어, `-culture=en`으로 영어(Rin, Lock-On / Release, Drink to restore HP.).
  - [x] PIE에서 언어를 바꿔도 에디터 UI 언어는 한국어 그대로이고, PIE가 끝나면 게임 문구도 한국어로 돌아옵니다. 저장값을 남기지 않습니다.
  - [ ] 드롭다운 목록을 마우스·키보드로 열어 고르기, Esc로 팝업·목록 닫기: 사람이 조작해 확인해야 합니다(도구가 콤보 박스 목록을 열지 못함). 닫기·Esc 처리는 조작 안내 팝업과 같은 코드(`bIsBackHandler`, Confirm)입니다.
- 로그: 테스트 창·패키지 로그에 `LogAg: Error` 없음.

## 3. 회귀 확인

- 한국어 모드의 문구 표 27개가 이전과 같습니다(테스트 창).
- 테스트가 쓴 `Saved/Config/WindowsEditor/GameUserSettings.ini`의 언어 값을 지웠고, PIE 확인 전에 끈 백그라운드 CPU 제한을 다시 켰습니다.
