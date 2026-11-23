# 입력 키 아이콘 작업 태스크

- [x] 1. Kenney 아이콘 18장 + W·A·S·D 묶음 가공(뚫린 부분 채움, 외곽선) → `Art/UI/InputIcons`
- [x] 2. `UAgActionWidget`, CommonUI 설정(`DefaultGame.ini`, `Windows/WindowsGame.ini`), `DeveloperSettings` 의존성
- [x] 3. Esc를 엔진 뒤로 액션으로: `IA_Back`·`IMC_UI`·`BP_UIInputData`, 플레이어 컨트롤러 `UIInputMapping`, 메뉴 `bIsBackHandler`, HUD 바인딩, `UI.Action.Escape` 삭제
- [x] 4. 빌드, 에디터 다시 켜기
- [x] 5. 텍스처 가져오기, `BP_KeyboardMouseControllerData`(키 18개 + 키 묶음)
- [x] 6. `WBP_HUD`·`WBP_InteractionPrompt` 키 표시 교체
- [x] 7. 문구 12개, `WBP_ControlsGuide` 다시 구성, `WBP_ControlsPopup` `InputMapping`, `TS_ControlsKey` 삭제
- [x] 8. 테스트 창 확인 → `4-test.md`
- [ ] 9. 정본 반영 후 `Specs/입력 키 아이콘` 삭제
