# 적 네임플레이트 작업 태스크

- [x] 1. 마네킹 `head` 뼈에서 머리 꼭대기까지의 뼈 기준 오프셋을 잽니다(읽기 전용 측정). → +X 약 18cm
- [x] 2. `UAgStatBar::ClearTrail`, `UAgNameplateWidget`을 만들고 `UAgHUDWidget`의 보스 HP·PP 표시를 옮깁니다.
- [x] 3. `UAgNameplateComponent`를 만듭니다.
- [x] 4. `AAgEnemyCharacter`에 `Nameplate`를 더하고 표시 조건을 구현합니다. `ExecutionPrompt`를 락온 지점 뼈로 옮깁니다. 보스·샌드백 가상 함수를 재정의합니다.
- [x] 5. 빌드합니다(헤더 변경이라 에디터를 닫고 빌드한 뒤 다시 엽니다).
- [x] 6. 에디터에서 `WBP_BossStatus`, `WBP_Nameplate`를 만들고 `WBP_HUD`, `WBP_ExecutionPrompt`, `BP_Sandbag`을 고칩니다.
- [x] 7. 확인 범위를 승인받은 뒤 테스트하고 `4-test.md`에 기록합니다.
- [ ] 8. 사용자 확인 뒤 `UI 시스템` 기획서(처형 안내 위치와 화면 구성 그림 포함)와 `캐릭터 기반 구조와 GAS`의 클래스 구조를 갱신하고 `Specs/적 네임플레이트/`를 삭제합니다.
