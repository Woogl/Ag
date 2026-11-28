# New Game 슬롯 선택 작업 태스크

- [x] 1. `UAgSaveSubsystem::StartNewGame(int32 Slot)`로 바꾸고 빈 슬롯 찾기를 지웁니다.
- [x] 2. `UAgSaveSlotRow`: `SlotButton`·`OnSlotPicked`·`GetSlotButton`으로 이름을 바꾸고, `SetSlot`에 `bPickAnySlot`을 받습니다.
- [x] 3. `UAgSaveSlotScreen`을 만들고 `UAgLoadGameScreen`의 공통 부분을 옮깁니다. `UAgLoadGameScreen`은 불러오기·Delete·처음 포커스만 남깁니다.
- [x] 4. `UAgNewGameScreen`을 만듭니다(줄 실행, 덮어쓰기 확인, 처음 포커스).
- [x] 5. `UAgTitleScreen`: New Game은 New Game 화면을 열고, New Game 버튼을 접던 코드를 지웁니다.
- [x] 6. 포맷 검사, 에디터를 닫고 전체 빌드, 에디터를 다시 켭니다.
- [x] 7. 문구 표에 `Menu.OverwriteSave`를 더합니다.
- [x] 8. `WBP_SaveSlotRow`의 버튼 이름을 `SlotButton`으로 바꾸고, `WBP_NewGame`을 만들어 기본값을 채우고, `WBP_Title`에 `NewGameScreenClass`를 넣습니다. `WBP_LoadGame`의 기본값이 그대로인지 확인합니다. 모두 저장합니다.
- [x] 9. 테스트 창에서 Spec의 완료 조건 시나리오 1~6과 Load Game 회귀(불러오기, 삭제, Back)를 확인합니다. 저장 슬롯은 테스트 전에 백업하고 끝나면 되돌립니다. 결과를 `4-test.md`에 적습니다.
- [ ] 10. 확인을 마치면 `Docs/Design`(게임 플로우, UI 시스템), `Docs/Architecture`(클래스 표와 클래스 구조), 문서 인덱스에 반영하고 `Specs/New Game 슬롯 선택/`을 지웁니다.
