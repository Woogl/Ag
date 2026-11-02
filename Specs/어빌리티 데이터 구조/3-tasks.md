# 어빌리티 데이터 구조 작업 태스크

- [x] 1. 프로그래머의 설계 승인을 받습니다.
- [x] 2. 구현 시점의 작업 트리와 JSON을 다시 확인하고, 기획자가 진행 중인 변경을 반영합니다. 전환 시점은 사용자가 기획자에게 알립니다.
- [x] 3. 데이터 구조체를 바꿉니다: `FAgAbilityData`의 필드, `FAgAbilityMontage`, `FAgMontageStart`, 엔진 `FAbilityTriggerData`, `HasSameGrantSettings`, `FAgTransitionParams`의 `TagRemoved`·`ToSection`.
- [x] 4. `UAgAbilityTask_PlayMontage`가 `Montage` 사본으로 시작 섹션을 고르고 타임라인을 읽게 하고, `UAgGameplayAbility`가 발동 Task를 시작한 뒤 재생 Task를 시작하게 합니다. `FAgPlayMontageParams`, `FAgMontageTimeline`, `UAgMontageTimelineData`를 지우고 에셋 매니저의 등록을 뺍니다.
- [x] 5. 캐릭터 데이터의 `Montages`·`FindMontage`를 지우고, 점프·가드 움찔 몽타주를 캐릭터 기본값으로, 사망 모션을 정의의 에셋으로 바꿉니다.
- [x] 6. 적 캐릭터가 `Event.Groggy`를 받아 그로기 효과를 걸게 하고, 그로기가 끝날 때의 어빌리티 취소를 지웁니다. 효과 정의에 `ApplicationIgnoredTags`를 더합니다.
- [x] 7. 입력 계층과 HUD를 `InputTag`로 바꿉니다(2-plan 6절).
- [x] 8. 스크래치패드 스크립트로 `Abilities.json`·`Characters.json`·`Effects.json`을 새 형식으로 바꾸고, 2-plan 3절의 특수 케이스를 고칩니다. `MontageTimelines.json`을 지웁니다.
- [x] 9. 빌드합니다. 헤더 변경이라 에디터를 닫고 빌드한 뒤 다시 열고, JSON 읽기 오류가 없는지 확인합니다(`FAbilityTriggerData`의 `TriggerSource` 포함).
- [x] 10. 에디터에서 `BP_Player`의 `JumpMontage`, `GuardFlinchMontage`를 정합니다.
- [x] 11. 확인 범위를 승인받은 뒤 1-spec 3절과 2-plan 5절을 테스트하고 `4-test.md`에 기록합니다.
- [x] 12. 사용자 확인 뒤 2-plan 9절의 문서(기획서 시간 규칙 포함)를 갱신하고 `Specs/어빌리티 데이터 구조/`를 삭제합니다.
