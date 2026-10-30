# 제자리 회전 작업 태스크

- [x] 1. `UAgAnimInstance`: `RootYawOffset`·`RootYawOffsetSmoothingTime`·`LastActorYaw`를 추가하고, `UpdateTurnInPlace`·`AdvanceTurn`·`GetTurnOffset`을 2-plan '매 프레임 계산' 1~5, 7대로 바꿉니다. `PendingTurnYaw`·`IsTurningInPlace`·`ConsumeTurnYaw`는 지웁니다.
- [x] 2. `UAgCharacterMovementComponent::PhysicsRotation`: 애님 인스턴스 연동을 지우고 사망·방향 고정 검사만 남깁니다.
- [x] 3. 빌드: 헤더가 바뀌므로 에디터를 닫고 빌드합니다(Live Coding 아님).
- [x] 4. `ABP_Character`: 제자리 회전 층 뒤, 그로기 전환 앞에 `Rotate Root Bone`을 넣고 `Yaw` 핀을 `RootYawOffset`에 연결합니다.
- [x] 5. 검증(1차): 시나리오 테스트 2개, 결과는 `4-test.md`.
- [x] 6. `UAgAnimInstance`: `TurnScale`을 지워 회전 모션의 회전량을 그대로 쓰고, `UpperBodyTwist`·`MaxUpperBodyTwist`·`UpperBodyTwistSmoothingTime`을 더해 2-plan '매 프레임 계산' 6을 구현합니다.
- [x] 7. 빌드: 헤더가 바뀌므로 에디터를 함께 쓰는 세션에 알리고 에디터를 닫은 뒤 빌드합니다.
- [x] 8. 에셋: 회전 시퀀스 6개의 Force Root Lock을 켜고, `BS_PlayerMove`를 만들어 저장합니다(열고 dirty 후 저장). 누가 PIE를 실행 중이면 하지 않습니다.
- [x] 9. `ABP_Character`: 상체 비틀기 노드(공간 변환 2개, `Transform (Modify) Bone` 3개)를 넣고 회전 모션 목록 기본값을 비웁니다. `ABP_Player`·`ABP_Boss`의 기본값을 2-plan 표대로 바꿉니다. 세 ABP를 컴파일·저장합니다.
- [ ] 10. 검증(2차): 1차 시나리오에 상체 비틀기, 배율 없는 회전, 다리 자세 연결 항목을 더해 기준을 정하고, 사람에게 확인받은 뒤 실행합니다. 결과를 `4-test.md`에 더합니다.
- [ ] 11. 사람이 결과를 확인하면 2-plan '정본 동기화 대상'의 문서를 고치고 `Specs/제자리 회전/`을 지웁니다.
