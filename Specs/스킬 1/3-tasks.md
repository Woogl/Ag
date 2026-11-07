# 스킬 1 작업 태스크

- [x] 1. 태그: `State.Hidden`, `GameplayCue.Teleport`, `GameplayCue.BlinkRushThrust` 네이티브 선언
- [x] 2. `AAgCharacterBase`: `State.Hidden` 태그 이벤트로 액터 숨기기·보이기
- [x] 3. 대상 필터: `UAgTargetingFilterTask_LockOnTarget`, 시야 필터의 `Socket` 생략 시 캡슐 중심 가림 검사
- [x] 4. Transition: `RequiredTags`
- [x] 5. `UAgAbilityTask_Teleport`·`FAgTeleportParams`: 대상 판정, 나타날 자리 찾기, 순간이동, 공중 높이 유지와 복원, 나타나는 큐
- [x] 6. 적중 카메라: `EAgCameraSituation::BlinkRushHit`, 적중 큐에서 재생, `Camera.json`
- [x] 7. 빌드
- [x] 8. 에셋: `AM_Player_Skill1`(`Attack09`, `Default`·`AirEnd`, 큐 노티파이), `GC_Teleport`·`NS_Teleport`, `GC_BlinkRushThrust`·`NS_BlinkRushThrust`, `M_TeleportRift`·`M_TeleportInk`
- [x] 9. 데이터: `Effects.json` `Hidden`, `TargetingPresets.json` `Teleport`, `Abilities.json` `Skill1`
- [x] 10. 시나리오 테스트(2-plan 4절)와 시각 조정, 연출 캡처, `4-test.md` 기록
- [ ] 11. 사용자 플레이 확인
- [ ] 12. 4단계: 기획서·아트·구현 문서 반영, `Specs/스킬 1` 삭제
