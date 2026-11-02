# 시뮬레이션 룸 작업 태스크

- [x] 1. `AAgGameMode`: `IsStageCleared` 분리, 보스 처치 연출 조건을 '결과가 정해지기 전의 보스 사망'으로
- [x] 2. `AAgEnemyAIController::NoticeTarget` 추가
- [x] 3. `AAgEnemySpawnPoint`와 `AAgSimulationRoomGameMode` 구현 (배치 지점 수집·번호 검사, 주소 옵션 읽기, 지점 스폰·전투 진입, 클리어 조건, 적 선택 화면 열기, `StartLineup`)
- [x] 4. `UAgButton::SetButtonText`, 위젯 클래스 `UAgEnemySelectScreen` 구현 (Start·Return to Title, Esc 미등록)
- [x] 5. C++ 빌드 (새 클래스와 `BindWidget` 추가라 에디터를 닫고 빌드)
- [x] 6. 보관본 에셋 되살리기: `BP_SimulationRoomGameMode`, `WBP_EnemySelect`, `LV_SimulationRoom`(외부 액터 포함)
- [x] 7. 에디터: `EnemyClasses`를 `BP_Boss`, `BP_Cutter`, `BP_Gunner`, `BP_Sandbag`, `BP_AttackSandbag` 순으로, 게임 모드 기본값(HUD·결과·선택 화면 클래스, 폰·컨트롤러)이 `BP_StageGameMode`와 맞는지, 목록 버튼 높이가 다른 메뉴 버튼과 같은지 확인
- [x] 8. `Config/DefaultGame.ini`의 `MapsToCook`에 `LV_SimulationRoom` 추가
- [x] 9. 에디터 시작·맵 로드 로그에 `LogAg` 오류 없음 확인
- [x] 10. 확인 범위를 승인받은 뒤 테스트: 보스전 회귀(Stage Clear·Game Over), 시뮬레이션 룸 완료 조건 1~9 (9는 타이틀 버튼이 없어 맵을 직접 다시 열어 확인)
- [x] 11. `4-test.md` 기록
- [x] 12. 사용자 플레이 의견 반영: `1-spec.md` 적 선택 화면을 자리별 드롭다운으로, 배치 자리 번호를 왼쪽부터로
- [x] 13. C++: 게임 모드의 자리별 구성(`CanPlace`, `FindEnemyClass`, `GetSlotCount`, 주소 옵션 빈 칸), `UAgEnemySlotWidget`·`UAgEnemyOptionWidget` 추가, `UAgEnemySelectScreen`을 자리 목록으로, 쓰지 않게 된 `UAgButton::SetButtonText` 삭제
- [x] 14. C++ 빌드
- [x] 15. 에셋: `WBP_EnemyOption`, `WBP_EnemySlot`(드롭다운 스타일), `WBP_EnemySelect` 다시 구성, `LV_SimulationRoom` 배치 지점 `Slot`을 왼쪽부터 1 ~ 5로, 목록 버튼 때문에 바꿨던 `WBP_Button`·`WBP_PauseMenu`·`WBP_BattleResult`를 되돌림
- [x] 16. PIE 확인(완료 조건 1 ~ 6·9, 첫 프레임 배치, 화면 캡처), `4-test.md` 갱신
- [x] 17. 프로그래머 플레이 확인
- [x] 18. 드롭다운 공용화(스테이지 선택과 함께): `UAgEnemySlotWidget`·`UAgEnemyOptionWidget`을 `UAgDropdown`·`UAgDropdownOptionWidget`으로, `WBP_EnemySlot`·`WBP_StageOption`을 `WBP_Dropdown`·`WBP_DropdownOption`으로 이름 변경, `WBP_EnemyOption` 삭제, 빈 자리 글자를 `WBP_EnemySelect`의 `EmptyText`로
- [x] 19. 공용화 뒤 드롭다운 PIE 확인(완료 조건 1 ~ 4, `4-test.md`)
- [ ] 20. 기획자 확인 뒤 4단계: 기획서(`1-spec.md` '2.6')·구현 문서(`2-plan.md` '4단계 구현 문서 반영') 반영, 인덱스 갱신, `Specs/시뮬레이션 룸/` 삭제

- 타이틀에서 들어오는 버튼은 스테이지 선택 Spec 작업입니다.
