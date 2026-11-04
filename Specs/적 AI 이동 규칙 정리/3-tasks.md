# 적 AI 이동 규칙 정리 작업 태스크

- [x] 1. 컨트롤러의 후딜레이 시각과 함수, `AAgEnemyCharacter::GatherReadyPatternRanges`
- [x] 2. `FAgEnemyAITask_KeepDistance`, 후딜레이를 거는 곳(`Combat`·`RunPattern`·`WaitReaction`·`PhaseTransition`), `RunPattern`의 후딜레이 검사, `ResponseReady`의 후딜레이 조건
- [x] 3. 데이터: `WaitMovement` 이름 변경, `RetreatDistance` → `PreferredDistance` (C++, `Characters.json` 모든 적)
- [x] 4. 이름 '반응 패턴' → '반응형 패턴' (조건 표시 이름, 주석)
- [x] 5. 빌드, 에디터 재시작
- [x] 6. `ST_Enemy`: '패턴 후딜레이' → '간격 지키기'(Task 교체), '다가가기'·'움찔' 삭제, 패턴 실행·교전 전환 정리, '반응 패턴' → '반응형 패턴', 컴파일·저장
- [x] 7. 옛 Task(`PostPatternDelay`, `Approach`)와 조건(`Flinching`) 삭제, 다시 빌드, 에디터 재시작
- [x] 8. 시나리오 테스트로 Spec 3절 1~10 확인, `4-test.md`에 기록
- [ ] 9. 사용자 플레이 확인
- [ ] 10. 4단계: 기획서·구현 문서 반영, Specs 폴더 삭제
