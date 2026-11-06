# 전투 중 대상 바라보기 작업 태스크

- [x] 1. 승인된 1-spec 수정 제안(2-plan 4절)을 1-spec에 반영
- [x] 2. `SK_Mannequin`의 `neck_01`·`head` 기준 자세에서 얼굴 앞 축을 재서 `Look At` 노드의 `LookAt_Axis` 결정
- [x] 3. `UAgCombatRules`에 `LookAtTargeting` 추가, `CombatRules.json`에 값 추가, 읽는 순서 조정
- [x] 4. `UAgAnimInstance`에 `UpdateLookAt`·`FindLookAtTarget`·`IsLookAtBlocked`와 속성 추가, 빌드
- [x] 5. `ABP_Character`에 `Look At` 노드 두 개(`neck_01` 알파 배율 0.5, `head`)를 상하 조준 뒤·`AdditiveSlot` 앞에 넣고 `LookAtLocation`·`LookAtAlpha` 연결, 한도 50°·보간 0, 컴파일·저장
- [x] 6. 시나리오 테스트 작성·실행(2-plan 5절), 회귀 시나리오 실행, `4-test.md` 기록
- [x] 7. 사용자 플레이 확인 (보이는 모습, 한도·따라잡는 시간): 따라잡는 시간 0.5초, 대기·이동 중에만 쳐다보도록 바꾼 뒤 "이제 딱 좋네요"
- [x] 8. 4단계: 전투 시스템, ABP와 로코모션·데이터 로딩과 스키마, 인덱스 반영, Specs 폴더 삭제
