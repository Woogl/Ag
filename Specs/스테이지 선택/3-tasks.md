# 스테이지 선택 작업 태스크

- [x] 1. `AAgGameMode`: `FAgStage`, `Stages`, `GetStages`, `IsStageAvailable`, `OpenStage(int32)`. `StageMap` 삭제
- [x] 2. `UAgTitleScreen`의 Start를 스테이지 선택 화면 열기로 변경, `BP_FrontEndGameMode`의 `Stages` 5개, `WBP_Title`의 `StageSelectScreenClass`
- [x] 3. `Config/DefaultGame.ini`의 `MapsToCook`을 `LV_FrontEnd`만 남김
- [x] 4. 버튼 목록 방식에서 바꾼 것 되돌리기: `UAgButton`·`UAgEnemySelectScreen` 코드와 `WBP_Button`·`ButtonStyle_Default`·`WBP_EnemySelect`를 HEAD 상태로
- [x] 5. `UAgStageSelectScreen`을 드롭다운으로 다시 작성, `UAgStageOptionWidget` 추가
- [x] 6. C++ 빌드와 에디터 재시작
- [x] 7. 에셋: `WBP_StageSelect`를 드롭다운 + Start·Back 줄로 다시 구성(드롭다운 모양은 `WBP_EnemySlot`에서 옮김), `WBP_StageOption`(`WBP_EnemyOption`과 같은 모양)
- [x] 8. 검증: PIE로 D1·D2·D3·D4·D8 통과(`4-test.md`). Esc·방향키·마우스는 사람이 확인
- [x] 9. 드롭다운 공용화(시뮬레이션 룸 태스크 18): `WBP_StageSelect`의 드롭다운을 `WBP_Dropdown`(`StageDropdown`)으로, `UAgStageOptionWidget`·`WBP_StageOption` 삭제
- [x] 10. 공용화 뒤 드롭다운 PIE 확인(D1·D2·D8, `4-test.md`)
