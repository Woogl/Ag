# 설정 메뉴 작업 태스크

- [x] 1. C++: `UAgTabList`(내용 타입, 버튼 상자, 키 아이콘 선택, `SelectDefaultTab`), `UAgTabButton`(`SelectionMark`).
- [x] 2. C++: `UAgSettingsPanel`, `UAgSettingsScreen`, `UAgDisplaySettings`, `UAgLanguageSettings`, `UAgMenuTitle` 추가.
- [x] 3. C++: `UAgTitleScreen` 정리, `UAgControlsPopup`·`UAgSettingsPopup` 삭제, `EAgUILayer::Modal`·`ModalStack` 삭제.
- [x] 4. 빌드(에디터를 닫고 작업 트리 빌드, 다른 세션에 알림).
- [x] 5. 에셋: `WBP_TabButton`의 `SelectionMark` 이름, `WBP_MenuTitle`, `WBP_CategoryButton`, `WBP_CategoryList`, `WBP_DisplaySettings`, `WBP_LanguageSettings`, `WBP_SettingsPanel`, `WBP_SettingsScreen` 만들기.
- [x] 6. 에셋: `WBP_PauseMenu` 탭 추가, `WBP_Title` 정리, `WBP_StageSelect`·`WBP_EnemySelect` 공통 양식, `WBP_UILayout`의 `ModalStack` 삭제, `WBP_ControlsPopup`·`WBP_SettingsPopup` 삭제, 문구 표 4줄.
- [x] 7. 테스트 창 검증: 타이틀 Settings 화면(열기, 분류, Back·Esc, 키 아이콘), 일시정지 메뉴 Settings 탭(Q·E, 열 때마다 Controls), 창 모드·해상도 적용과 다시 실행, 언어, 스테이지 선택·적 선택 화면의 배경·제목·위치(스크린숏). 테스트 창이 바꾼 `GameUserSettings.ini`는 끝나면 되돌림.
- [x] 8. 회귀: 일시정지 메뉴·아이템 시나리오(`item_potion`), 타이틀 → 스테이지 진입, 시뮬레이션 룸 적 선택 화면 표시(적을 골라 Start하는 흐름은 실행하지 않음).
- [x] 9. `4-test.md` 기록.
