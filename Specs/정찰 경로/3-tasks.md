# 정찰 경로 작업 태스크

- [x] 1. `AAgPatrolRoute`: 스플라인 구성, 점 조회, 지면 스냅, `FindWalkPath`, `SetPointLocations`, 편집 뒤 확인·맵 검사 경고
- [x] 2. `UAgPatrolPreviewComponent`: 경로·적 주인별 표시 데이터
- [x] 3. `AAgEnemyCharacter`: `PatrolRoute`로 교체, `PatrolPreview`, 경로 지정·이동 뒤 확인과 맵 검사 경고
- [x] 4. `FAgEnemyAITask_Patrol`: 경로에서 점 읽기, 갈 수 없는 점 건너뛰기, 한 바퀴 실패 시 처음 자리 대기, 경고 로그
- [x] 5. `FAgPatrolVisualizer`와 `AgEditor` 등록, `Ag.Build.cs` 의존성
- [x] 6. 전체 빌드, 에디터 재시작
- [x] 7. 에디터 확인(열린 LV_BossStage에 임시 배치 후 삭제, 저장 안 함): 배치·스냅·표시·경고·맵 검사·`SetPointLocations`
- [x] 8. 동작 확인(테스트 창): 공유 경로, 갈 수 없는 점 건너뛰기, 모두 실패 시 대기, 경로 없는 적·MOV 0 적 회귀
- [x] 9. `4-test.md` 기록
- [x] 10. 에디터를 닫고 전체 빌드(바닥 고르기·번호 글꼴 수정)
- [ ] 11. 사람 확인: 뷰포트에서 점 Alt 드래그·끌어 놓기 스냅
