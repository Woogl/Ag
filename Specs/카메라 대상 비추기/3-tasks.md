# 카메라 대상 비추기 작업 태스크

- [x] 1. `AAgPlayerCameraManager`: `StartLookAt`·`StopLookAt`, `PreviousCameraTarget`으로 이름 변경과 `UpdateLockOnCamera`의 대상 타입 확장, Tick의 대상 선택, `ProcessViewRotation` 입력 차단, `UpdateCameraTurn` 중단 조건, 컷신·초기화 정리
- [x] 2. `UAgAnimNotifyState_CameraLookAt` 추가
- [x] 3. 빌드와 에디터 재시작 (새 UCLASS라 Live Coding으로 끝나지 않음)
- [x] 4. `AM_Boss_PhaseTransition`에 노티파이 상태를 0.0001초부터 몽타주 끝까지 추가
- [x] 5. 시나리오 테스트: 등진 상태의 페이즈 전환, 비추는 중 마우스 입력, 보스 4m 안 넉다운, 보스 락온 중 전환, 비추는 중 락온, 회귀(극한 회피 상대 쪽 회전, 처형 카메라, 락온 대상 전환)
- [x] 6. `4-test.md` 기록
- [x] 7. `AM_Boss_PhaseTransition`에 포효 시작 줌인 노티파이 추가, 시야각 변화 시나리오 테스트와 `4-test.md` 기록
- [x] 8. 보스 쪽 당기기: `UAgLookAtCameraModifier`·`FAgCameraLookAt` 추가, 카메라 매니저와 노티파이 상태 변경, 빌드와 시나리오 테스트, `4-test.md` 기록
- [ ] 9. 4단계 문서 반영과 `Specs/카메라 대상 비추기/` 삭제
