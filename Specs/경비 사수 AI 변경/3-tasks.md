# 경비 사수 AI 변경 작업 태스크

- [x] 1. `UAgCharacterData`의 `PostPatternDelayMovement`·`RetreatDistance`와 패턴 후딜레이 Task 움직임
- [x] 2. `AAgEnemyCharacter`의 `bCanWalkOffLedges` false, 적 BP 기본값이 덮어쓰지 않는지 확인 (`uea find`로 에셋 1641개 모두 읽음, 덮어쓴 곳 없음)
- [x] 3. `AAgProjectile` 예측 조준(`AimTarget`, `bHorizontal`, 익명 `PredictLocation`)
- [x] 4. Aim Task·어빌리티 조준점·SpawnActor 조준점 분기 삭제, `AimLost` 검사 정리
- [x] 5. `AAgProjectile`의 `bDestroyOnHit`·`OnDamageApplied`와 피격 결과 컴포넌트의 호출
- [x] 6. JSON: `Characters.json` `Gunner`, `Abilities.json` `PistolShot`, `SpawnActors.json` `PistolBullet`·`SwordWave`
- [x] 7. 빌드, 에디터 재시작, JSON 다시 읽기 오류 없음 확인
- [x] 8. 시나리오 테스트로 Spec 3절 1~9 확인, `4-test.md`에 기록
- [ ] 9. 사용자 플레이 확인
- [ ] 10. 4단계: 기획서·구현 문서 반영, Specs 폴더 삭제
