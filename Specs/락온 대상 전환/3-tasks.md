# 락온 대상 전환 작업 태스크

- [x] 1. `AgTargeting::FindSideTarget`을 더합니다.
- [x] 2. `UAgGameplayAbility::SetActionTarget`을 더합니다.
- [x] 3. `AgTargeting::ProjectToGameScreen`을 더하고 `AgTargetingFilterTask_OnScreen`·`AgTargetingSortTask_ScreenCenter`가 쓰게 합니다.
- [x] 4. `WaitTargetLost`를 `TrackTarget`으로 바꾸고, 지금 대상 읽기와 사망 시 다시 고르기를 넣습니다. `Abilities.json`의 두 락온 어빌리티 Task 이름을 함께 고칩니다.
- [x] 5. `AAgPlayerCameraManager::ApplyLookInput`에 카메라 상태별 처리와 튕김 판정(`UpdateLockOnFlick`, 판정 값은 상수 `LockOnFlickTime`·`LockOnFlickAngle`)을 넣습니다.
- [x] 6. `AAgPlayerCharacter::SwitchLockOnTarget`(락온 어빌리티의 대상을 직접 바꿈)과 확인용 입력 `DevLookInput`을 더합니다.
- [x] 7. 빌드합니다. 헤더가 바뀌므로 에디터를 닫고 빌드한 뒤 다시 엽니다.
- [x] 8. 시나리오 테스트로 1-spec의 완료 조건을 확인하고 `4-test.md`에 기록합니다. 기존 회귀 시나리오 `look_at_sandbags.py`, `player_execution.py`, `enemy_turn_order.py`를 함께 돌립니다.
- [ ] 9. 사용자 플레이 확인을 받습니다. 튕김 판정 값은 이때 조정합니다. 판정 각도는 플레이 확인에서 15°가 너무 민감해 30°로 올렸습니다.
- [ ] 10. 확인을 받으면 4단계(기획서·구현 문서 반영, Specs 폴더 삭제)를 진행합니다.
