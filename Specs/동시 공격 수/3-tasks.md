# 동시 공격 수 작업 태스크

- [x] 1. `CombatRules.json`에 `MaxAttackers` 2, `ReattackWaitTime` 2를 더하고 `UAgCombatRules`에 같은 속성을 둡니다.
- [x] 2. `UAgCombatSubsystem`을 만듭니다(목록 등록·해제와 변경 알림, `FindBoss`, `StartAttack`·`EndAttack`, `HasAttackTurn`).
- [x] 3. `FAgEnemyAITask_Combat`의 들어갈 때·나갈 때에 등록·해제를 연결합니다.
- [x] 4. `FAgEnemyAITask_KeepDistance::Tick`에 차례 확인을 넣습니다.
- [x] 5. `FAgEnemyAITask_RunPattern`의 `EnterState`에 차례 확인과 `StartAttack`, `ExitState`에 `EndAttack`을 넣습니다.
- [x] 6. HUD 보스 상태를 서브시스템 구독으로 바꾸고, `AAgGameState`의 보스와 `AAgGameMode`의 보스 찾기를 지웁니다.
- [x] 7. 빌드하고 에디터에서 데이터 재로딩·PIE 시작 시 오류 로그가 없는지 확인합니다.
- [x] 8. 1-spec 완료 조건 시나리오를 시뮬레이션 룸과 보스 스테이지에서 실행하고 `4-test.md`에 기록합니다.
- [x] 9. 회귀 시나리오(`enemy_wait_*`, `nameplate_boss`, `stage_boss_clear`, `sim_lineup`, `gunner_aim_cancel`, `boss_patterns`)를 다시 돌립니다.
- [ ] 10. 사용자 플레이 확인 뒤 2-plan '정본 반영'의 문서를 고치고 `Specs/동시 공격 수/`를 지웁니다.
