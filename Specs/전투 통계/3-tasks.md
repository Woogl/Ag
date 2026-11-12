# 전투 통계 작업 태스크

- [x] 1. `UAgCombatStatsSubsystem`과 기록 구조체(`EAgCombatResult`, `FAgCombatEnemyStats`, `FAgCombatStats`, 파일 구조체): 도전 횟수, 파일 경로, JSON 저장
- [x] 2. `UAgCombatSubsystem::GetCombatants`
- [x] 3. `UAgCombatStatsComponent`: 시작·끝·결과 판정, 상대한 적, 패리·극한 회피·피격 횟수, 전투 시간, 결과창용 기록
- [x] 4. `AAgPlayerCharacter`에 컴포넌트 추가, `AAgGameMode`의 `InitGame`·`RestartStage` 연결
- [x] 5. `UAgBattleResultScreen`: 시뮬레이션 룸일 때 통계 표 채우기·접기, 이름 묶기와 시간 표기
- [x] 6. 빌드
- [x] 7. `WBP_BattleResult`: `Content` 세로 상자로 다시 배치하고 `CombatStatsTable` 추가, 저장
- [x] 8. 테스트 창 검증: 처치·사망·이탈 기록, 도전 횟수(사망 Restart, 일시정지 Restart, 새 진입), 횟수 세기(패리·극한 회피·피격·가드 제외), 일시정지 시간 제외, 중단 미저장, 결과창 표시, 보스 스테이지 Stage Clear와 표 없음
- [x] 9. `4-test.md` 작성
- [x] 10. `Docs/Architecture/캐릭터 기반 구조와 GAS.md` 클래스 표·그림 갱신
- [ ] 11. 사람 확인 뒤: 게임 플로우 '전투 통계', UI 시스템 결과창, 결과창 그림, 문서 인덱스 반영, `Specs/전투 통계/` 삭제
