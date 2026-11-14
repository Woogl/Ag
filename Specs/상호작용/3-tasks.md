# 상호작용 작업 태스크

- [x] 1. `Specs/성문 상호작용/` 폴더 삭제 (이 Spec에 합침)
- [x] 2. 태그·채널·데이터: `Input.Execute` → `Input.Interact`, `ECC_AgInteraction` 채널, `FAgAbilityData::DisplayName`, `Abilities.json`의 `Execution` 수정, 시나리오 실행기의 키 표
- [x] 3. `IAgInteractable` 인터페이스와 `AgInteractionRange` 충돌 프로파일 추가
- [x] 4. `UAgInteractionPrompt`와 `UAgInteractionComponent` 추가 (대상 고르기, 실행, 안내 갱신)
- [x] 5. `AAgPlayerCharacter`: 컴포넌트 추가, `InteractAction`, `RunInput`의 상호작용 분기, 선입력 제외 태그
- [x] 6. `AAgEnemyCharacter`: 처형 안내 컴포넌트와 관련 코드 삭제
- [x] 7. `AAgGate` 추가
- [x] 8. 빌드
- [x] 9. 에셋: `IA_Execute` → `IA_Interact`, `WBP_ExecutionPrompt` → `WBP_InteractionPrompt`(부모 클래스 변경, 키 표시 + 행동 이름), `BP_Player` 기본값(입력 액션, 위젯 클래스), 적 BP의 남은 처형 안내 참조 정리, 조작 안내 위젯의 F 줄, `DefaultEngine.ini`의 임시 `[CoreRedirects]` 삭제
- [x] 10. 에셋: `BP_Gate` 제작
- [ ] 10-1. `LV_FortressBreach` 문틀 사이에 `BP_Gate` 배치 (기획자가 진행)
- [x] 11. 시나리오 테스트: 처형 회귀(`player_execution.py`, `groggy.py`), 성문(닫힘·안내·열림·한 번만·처형 우선), 결과를 `4-test.md`에 기록 (Restart와 실제 탄환은 실행하지 않음)
- [ ] 12. 정본 동기화: 기획서(플레이어 시스템, 린, UI 시스템과 HUD 화면 구성 그림, 요새 돌파, 용어집), 아키텍처 문서(입력 결정, 클래스 구조, 어빌리티 항목), `Specs/상호작용/` 삭제
