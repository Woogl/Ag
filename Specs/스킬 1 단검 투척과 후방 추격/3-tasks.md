# 스킬 1 단검 투척과 후방 추격 작업 태스크

- [x] 1. 투척 모션 후보 GIF를 나란히 만들어 보여 드리고 고름 (Dual Wield `Attack_Combo1` 왼손 오버핸드, 0.29초에 놓음)
- [x] 2. C++: `AAgProjectile` `bPredictAim`, `Teleport` 이벤트 대기·등 뒤 탐색·`MaxAngle`, `Event.Teleported` 태그
- [x] 3. 빌드, 에디터 재시작
- [x] 4. `BP_Dagger`(판정 캡슐, 줄인 `Sword` 메시)
- [x] 5. `AM_Player_Skill1`을 `Throw`·`Slash` 섹션으로 다시 짜고 사라지는 큐 노티파이 삭제, `BlendOutTriggerTime` 0, `Attack_Combo1` 루트 모션 켬
- [x] 6. 데이터: `Skill1`, `HitReward` 스킬 1 행, `SpawnActors.json` `Dagger`, 프리셋 `Teleport` → `Throw`
- [x] 7. 테스트 창 시나리오: 완료 조건 7개
- [x] 8. 회귀: 투사체·WeaponHit·스킬 시나리오
- [x] 9. `4-test.md` 기록
- [x] 9-1. 단검 궤적: 후보 GIF 네 개 → 'B 먹선 + 흰 심' → `NS_DaggerTrail`·`M_DaggerTrail`을 `BP_Dagger`에 붙이고 테스트 창 화면 GIF로 확인
- [ ] 10. 정본 동기화(린 '스킬 1'·UP 충전 표·연계, 아트 표현 사양(단검 외형·궤적 행 포함), 데이터 로딩과 스키마의 Task·스킬 1 행, 몽타주 타임라인의 Teleport 문장, Dual Wield 조사의 루트 모션 칸, 모션 GIF) 후 `Specs` 폴더 삭제 — 확인 뒤
