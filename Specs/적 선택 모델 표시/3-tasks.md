# 적 선택 모델 표시 작업 태스크

- [x] 1. `AAgCharacterPreview` 추가: 캡처·조명 컴포넌트, 렌더 타깃 생성, `ShowCharacter`(AI 없이 스폰, 라이팅 채널 1, 캡처 전용 표시, 멈춘 동안 틱, 메시 범위로 정면 꽉 찬 거리)
- [x] 2. `AAgSimulationRoomGameMode`: `EnemyPreviewClass`, `GetEnemyPreview`(적 종류마다 하나, 레벨 위 1km에 20m 간격)
- [x] 3. `UAgDropdown`의 `SelectedWidgetClass`와 닫힌 칸·목록 항목 생성 분리, `UAgDropdownOptionWidget::SetOption`
- [x] 4. `UAgEnemyCard` 추가: 키로 적 클래스와 렌더 타깃을 받아 모델 또는 '+' 표시
- [x] 5. 빌드와 에디터 재시작
- [x] 6. 에셋: `BP_EnemyPreview`, `WBP_EnemyCard`, `WBP_EnemySlot` 추가, `WBP_EnemySelect` 항목 클래스, `BP_SimulationRoomGameMode` 기본값
- [x] 7. 시나리오 테스트와 `4-test.md` 기록
- [ ] 8. 사람의 플레이 확인(방향키·Esc·열린 목록 모양) 뒤 4단계 문서 반영
