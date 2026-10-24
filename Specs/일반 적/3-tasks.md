# 일반 적 작업 태스크

[2-plan](<2-plan.md>)은 승인을 받았습니다. 빌드·에디터·테스트 창은 [AI 워크플로우](<../../Docs/AI 워크플로우.md>)의 '테스트 작업 수칙'과 에디터 작업 스킬을 따르고, 띄우기 전에 묻습니다.

## 코드

- [x] 1. 피아 식별: `EAgTeam`과 `AAgCharacterBase::Team`·`GetGenericTeamId`·`GetTeamAttitudeTowards`(태도 표), 플레이어(아군)·적(적군) 생성자 설정, 플레이어의 `GetGenericTeamId` 재정의 제거.
- [x] 2. 적중 판정: `WeaponHit`·`AgProjectile`의 적대 조건을 `Friendly`가 아님으로, `UAgTargetingFilterTask_Hostile::bIncludeNeutral` 추가.
- [x] 3. 적 데이터: `UAgBossData`를 `UAgEnemyData`로 바꾸고 `SightDistance`·`SightAngle` 추가, 에셋 매니저의 `Boss` 짝 변경, `UAgCombatRules::TargetLostTime`.
- [x] 4. 적 캐릭터: `AAgEnemyCharacter` 추가(팀, AI 컨트롤러·빙의·회전 설정, `AIStateTree`, `PatrolPoints`, 처음 자리와 방향 기록, 시야 기준점, 페이즈, 그로기 미루기, 그로기·처형 피격·페이즈 전환 끝의 PP 회복, `CanReturnToNonCombat`). `AAgBossCharacter`의 부모를 바꾸고 옮긴 코드를 뺌.
- [x] 5. 적 AI 컨트롤러: `AAgBossAIController`를 `AAgEnemyAIController`로 바꾸고 지각 컴포넌트와 시야·대미지 감각 설정, 알아차림·놓친 시각, 팀·태도 전달 추가. 락온 유지는 7번의 '적: 전투'로 옮김.
- [x] 6. 피격 알림: `UAgDamageResultGameplayEffectComponent`에서 공격이 닿았을 때 `UAISense_Damage::ReportDamageEvent`.
- [x] 7. StateTree 노드: `AgBossAITasks`·`AgBossAIConditions`를 `AgEnemyAITasks`·`AgEnemyAIConditions`로 바꾸고 모든 노드 이름 변경, '적: 전투'·'적: 정찰'·'적: 대상을 알아차림'·'적: 대상을 놓침' 추가, '적: 비전투'에 `ExitState` 추가. `DefaultEngine.ini`에 구조체 `StructRedirects` 추가.
- [x] 8. 다가가기 이동: '적: 다가가기'를 NavMesh 길찾기로 바꿈.
- [x] 9. 태그: `Ability.Boss.Pattern`을 `Ability.Enemy.Pattern`으로, `State.NonCombat`·페이즈 전환 태그 설명 갱신. `IsFacingFixed`에서 비전투 조건 제거.
- [x] 10. HUD: `UAgHUDWidget`에 `BossStatusPanel` 바인딩을 더하고 보스가 비전투인 동안 숨김.
- [x] 11. 에디터 코드 주석: `AgAssetToolset.h`의 에셋·노드 이름 예시를 새 이름으로.
- [x] 12. 빌드: 에디터 작업 스킬의 순서로 C++ 빌드와 에디터 재시작.

## 데이터

- [x] 13. `Characters.json`의 `Boss`에 `SightDistance` 2000, `SightAngle` 120.
- [x] 14. `CombatRules.json`에 `TargetLostTime` 5.
- [x] 15. `TargetingPresets.json`에 `Hit` 프리셋, `MontageTimelines.json`의 `AreaHit` 세 곳을 `Hit`으로.
- [x] 16. `Abilities.json`의 패턴 태그를 `Ability.Enemy.Pattern`으로.

## 에셋

- [x] 17. `ST_Boss`를 `ST_Enemy`로 이름을 바꾸고 `BP_Boss`의 `AIStateTree` 참조 확인. 노드가 새 이름으로 읽히는지 확인하고 스키마 클래스를 바꾼 뒤, 2-plan '전투와 비전투'의 상태 구성과 전환 순서로 고쳐 컴파일·저장. 저장 뒤 `StructRedirects` 제거. (구현: 리다이렉트로 옛 트리를 읽어 구성을 확인한 뒤 같은 노드로 `ST_Enemy`를 새로 만들고, 참조가 없어진 `ST_Boss`는 지웠습니다.)
- [x] 18. `WBP_HUD`: 보스 이름·HP 바·PP 바를 감싸는 `BossStatusPanel`.
- [x] 19. `LV_BossStage`에 `NavMeshBoundsVolume`. (정적 내비 메시를 에디터에서 굽고 저장)

## 검증

- [x] 20. `4-test.md`에 시나리오와 통과 조건을 먼저 적고 확인받은 뒤 실행. 이번에는 보스로 확인할 수 있는 완료 조건만 봅니다: 보스전 시작 즉시 전투와 그때부터 보이는 보스 체력바, 커스터의 시야 거리·정면 각도(보스를 다른 방향으로 둔 시나리오), 시야 밖 피격으로 전투 진입, 첫 전투에서 쿨다운 있는 패턴 선택, 놓쳐도 계속 전투, 페이즈와 페이즈 전환, 플레이어 사망 뒤 보스가 돌지 않고 대기, 사망한 보스가 남음. 회귀로 패턴·그로기·처형·스테이지 결과를 봅니다.
- [x] 21. 보스에 해당하지 않는 완료 조건(정찰, 5초 뒤 비전투 복귀, 처음 자리·방향 복귀, 다시 들어올 때 HP·PP·쿨다운 유지, 벽 뒤 플레이어)은 일반 적 작업에서 확인한다고 `4-test.md`에 적습니다. 샌드백 동작은 샌드백 작업에서 확인합니다.

2026-10-24 추가 검증은 `4-test.md` 4·5절에 기록합니다. T9·T10·T12·T13 통과 후 T11 방향 오차를 수정하고 세 방향 복귀 재검증도 통과했습니다. 샌드백 검증은 해당 작업으로 보류하며, 사람 확인과 정본 동기화는 아직 남아 있습니다.

- [x] 21a. 정찰 지점 없이 복귀한 뒤 처음 방향으로 정확하게 돌아서도록 수정하고 T11 재검증. 시선 기준점을 현재 위치로 변경했고, 처음 방향 0°·90°·-135° 모두 복귀 후 방향 오차 0.0000°·이동 속도 0으로 통과했습니다.

## 정본 동기화 (검증과 사람 확인 뒤)

- [ ] 22. 기획서: 1-spec '기존 기획서와의 관계' 표의 문서들과 '보스 - 커스터'의 시야 값.
- [ ] 23. 구현 문서: [캐릭터 기반 구조와 GAS](<../../Docs/Architecture/캐릭터 기반 구조와 GAS.md>)의 캐릭터 클래스 표·클래스 구조도·레벨 시작 순서도·효과 거는 객체 설명, [데이터 로딩과 스키마](<../../Docs/Architecture/데이터 로딩과 스키마.md>)의 데이터 클래스 표·`PatternSelection` 설명·파일 구성·'JSON 외부 데이터 위치'(정찰 지점), 에디터 작업 스킬의 `ST_Boss` 설명.
- [ ] 24. [용어집](<../../Docs/용어집.md>)에 새 기획 용어(시야, 정찰, 비전투 등)가 필요한지 확인하고 반영, 문서 인덱스의 적 시스템·보스 시스템 설명 갱신.
- [ ] 25. 일반 적 작업에서 21번 항목을 확인할 때까지 이 폴더를 지울지, 확인 항목을 일반 적 작업의 1-spec으로 넘기고 지울지 정한 뒤 `Specs/일반 적/` 정리.
