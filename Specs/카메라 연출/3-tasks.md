# 카메라 연출 작업 태스크

- [x] 1. `1-spec.md`에 `2-plan.md`의 '1-spec 수정 제안' 1~4 반영
- [x] 2. 삭제 전 값 읽기: `GC_CameraShake_*`의 `WorldInnerRadius`·`WorldOuterRadius`, `AM_Boss_GroundSlam`·`AM_Player_Ultimate`의 셰이크 큐 노티파이 시각. 포효 모션(`ARPG_Samurai_Anim_UE_Buff`)의 기합 순간 측정
- [x] 3. 신규 C++: `AgCameraEffect.h`(`EAgCameraSituation`, `FAgCameraEffect`), `UAgOffsetCameraShakePattern`(Update·Scrub, 이즈 인·아웃), `UAgExecutionCameraModifier`(구도·좌우 선택·충돌 스윕·Alpha 섞기, Priority 64), `UAgAnimNotify_PlayCameraShake`(월드 셰이크, 노티파이 이름)
- [x] 4. `AAgPlayerCharacter`: 셰이크 세기 필드와 `GetCombatCameraShake`를 `CameraEffects`·`PlayCameraEffect`·`PlayDamageCameraEffects`로 교체, 상대 쪽 회전(Tick, 시야 입력·락온·처형 카메라·시간 정지로 끝, 같은 프레임 가까운 상대), 처형 카메라(모디파이어 추가, 커밋 콜백 시작, 종료 콜백 끝, 시야 입력 무시), `NotifyEnemyAttackStarted`, 시간 정지 시작 때 `StopAllCameraShakes`
- [x] 5. `UAgDamageResultGameplayEffectComponent`: `PlayDamageCameraEffects` 호출과 극한 회피의 `PlayCameraEffect(PerfectDodge)`
- [x] 6. `AAgEnemyCharacter` 패턴 커밋 알림, `AAgGameMode` 보스 처치(슬로우모션, `PlayCameraEffect(BossKill)`), `UAgTimeSubsystem` 같은 프레임 슬로우모션 합치기
- [x] 7. 삭제: `AgCameraShake.h`·`.cpp`, 태그 `GameplayCue.CameraShake.*` 3개
- [x] 8. C++ 빌드 (에디터 재시작 필요: 새 클래스와 UPROPERTY 변경)
- [x] 9. 에셋: `CS_*` 7개(`2-plan.md` '연출 셰이크 에셋'), `CM_Execution`, `BP_Player`(`CameraEffects`, `ExecutionCameraClass`, `OffscreenAttackRange`), `BP_Boss` 확인, `AM_Boss_GroundSlam`·`AM_Player_Ultimate` 노티파이 교체, `AM_Boss_PhaseTransition` 노티파이 추가, 참조가 없어진 `GC_CameraShake_*` 삭제
- [x] 10. 수정 요청 반영(기울기 제거, 화면 밖 공격은 회전 대신 카메라 위치 이동, 처형 카메라 완화): `1-spec.md`·`2-plan.md` 수정, 패턴 `Roll` 삭제, 상대 쪽 이동, 처형 모디파이어 구도·시간, 빌드, `CS_Parry` 다시 저장, `CS_ExecutionStab`·`Blow` 시야각, `BP_Player`의 `OffscreenAttack`, 패리·처형·화면 밖 공격 다시 확인(실행 전 범위를 사람에게 확인)
- [x] 11. 회귀 확인: 기존 셰이크 상황(완료 조건 12), 락온 카메라의 회전과 해제, 궁극기 컷신과 마무리 — 락온 카메라만 남음(`4-test.md` '4')
- [x] 12. 확인: 완료 조건 1~14. 실행 전 범위를 사람에게 확인. — 테스트 창 범위는 끝남, 사람 확인 항목은 `4-test.md` '4'
- [x] 13. `4-test.md` 기록
- [x] 14. 4단계: 기획서 반영(`1-spec.md` '2.8'), 구현 문서 반영(`2-plan.md` '4단계 구현 문서 반영'), 인덱스 갱신, `Specs/카메라 연출/` 삭제
- [x] 15. 수정 요청 반영(극한 회피 회전만 남김, 보스 처치 회전 제거, 패리 블렌드 0 / 0.4, 화면 밖 공격 삭제): `1-spec.md`·`2-plan.md` 수정, `BP_Player`(`PerfectDodge` 회전 켬, `BossKill` 회전 끔, `OffscreenAttack` 항목 삭제), `CS_Parry` 시간, 코드에서 `OffscreenAttack`·상대 쪽 이동·적의 패턴 커밋 알림·`OffscreenAttackRange` 삭제, 빌드, `BP_Player` 다시 저장
