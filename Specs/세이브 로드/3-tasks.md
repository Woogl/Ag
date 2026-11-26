# 세이브&로드 작업 태스크

- [x] 1. 저장 내용과 서브시스템: `UAgSaveGame`, `UAgSaveSubsystem`(슬롯 없는 진행, 슬롯 읽기, 새 게임·불러오기·지우기·내려놓기, 바꾸는 함수와 파일 쓰기, 플레이 시간, 종료할 때 저장). 망가진 저장 파일에서 엔진 읽기 함수의 동작 확인(Plan '4')
- [x] 2. 인벤토리: 시작 값을 지금 진행에서 읽기, `Refill`, 바뀔 때 저장. `AAgPlayerCharacter::Rest`는 `Refill` 호출로
- [x] 3. 체크포인트: `StartPoint`와 `GetStartTransform`, 휴식할 때 저장. `BP_Checkpoint`에서 `StartPoint` 자리 확인
- [x] 4. 성문: 열 때 저장, `BeginPlay`에서 열린 상태 되살리기
- [x] 5. 게임 모드: `RestartPlayer` 재정의(체크포인트에서 시작), `OpenStage`의 마지막 스테이지, `OpenFrontEnd`의 `Unload`, Stage Clear 때 던전 진행 지우기, `ShouldRestartOnGameOver`(시뮬레이션 룸 false)
- [x] 6. 사망 뒤 다시 시작: `UAgBattleResultScreen`의 자동 다시 시작 모드, `WBP_BattleResult`의 `RestartDelayAnimation`
- [x] 7. UI 레이어: `EAgUILayer::Modal`과 `WBP_UILayout`의 `ModalStack`
- [x] 8. 확인 창: `UAgConfirmDialog`, `WBP_ConfirmDialog`, 문구 표 `Menu.DeleteSave`
- [x] 9. Load Game 화면: `UAgLoadGameScreen`, `UAgSaveSlotRow`, `UAgSaveSlotButton`과 WBP 3개
- [x] 10. 타이틀 화면: Continue·New Game·Load Game 버튼과 표시 조건, 첫 포커스. `WBP_Title` 버튼 배치
- [x] 11. 스테이지 선택 화면: 처음 고른 스테이지, Back·Esc에서 `Unload`
- [x] 12. 빌드, 포맷 검사(`Scripts/FormatCpp.ps1`)
- [x] 13. 검증: 임시 사본의 테스트 창에서 Spec '3. 완료 조건' 항목별 확인, 기존 회귀 시나리오 확인, `4-test.md` 기록
- [x] 14. 사본에서 만든 위젯 블루프린트 7개(`WBP_UILayout`, `WBP_Title`, `WBP_BattleResult`, `WBP_ConfirmDialog`, `WBP_SaveSlotButton`, `WBP_SaveSlotRow`, `WBP_LoadGame`)를 작업 트리의 `Content/UI`로 들여놓고, 공유 에디터를 닫은 뒤 작업 트리를 빌드해 다시 켜기. 소스와 에셋이 함께 들어가야 타이틀 화면이 맞물립니다.
- [ ] 15. 정본 동기화: Spec '2.10'의 기획서와 Plan '6'의 아키텍처 문서·스킬 반영, 시안 SVG를 `Docs/Art/Attachments`로 옮기기, 인덱스 갱신, `Specs/세이브 로드` 삭제
