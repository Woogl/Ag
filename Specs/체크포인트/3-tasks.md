# 체크포인트 작업 태스크

설계는 [2-plan](<2-plan.md>)을 따릅니다.

- [x] 1. **Spec 보완:** [1-spec](<1-spec.md>)에 레벨 시작의 로딩 화면 규칙(적이 모두 등장할 때까지 화면을 가리고 조작을 받지 않음)과 완료 조건을 더합니다.
- [x] 2. **미리 읽기:** `FAgJsonReader`의 에셋 경로 모으기, `UAgJsonData::GatherAssetPaths`와 `UAgAbilitySet`의 재정의(그 캐릭터의 어빌리티 항목과 생성 액터 정의), `UAgAssetManager::PreloadData`.
- [x] 3. **스포너 (게임):** `AAgEnemySpawner`의 배치 값, 적 클래스 → 어빌리티 에셋 순서의 비동기 읽기, `Spawn`·`DestroyEnemy`·`OnSpawnFinished`, `EndPlay`의 취소와 정리. `AAgEnemyCharacter::SetPlacement`. 이미 메모리에 있는 에셋을 요청했을 때 완료 콜백이 불리는 시점을 엔진 소스로 확인합니다.
- [x] 4. **스포너 (에디터):** 적 외형 미리 보기 메시, 시야·정찰 경로 미리 보기 컴포넌트와 정찰·그룹 배치 검사를 적에서 스포너로 옮김, 레벨에 직접 놓인 적 경고.
- [x] 5. **게임 모드:** `StartPlay`에서 생성을 기다리는 스포너가 있으면 로딩 화면을 열고 모두 끝나면 닫음(스포너 수와 걸린 시간 로그), `LoadingScreenClass`, `RespawnEnemies`. 전투 서브시스템이 빈 그룹을 지움.
- [x] 6. **휴식:** `AAgPlayerCharacter::Rest`, `FAgItemData::bRefill`, `AAgCheckpoint`.
- [x] 7. **빌드:** 다른 작업이 에디터를 쓰지 않는지 확인하고 전체 빌드 뒤 에디터를 다시 켭니다.
- [x] 8. **데이터와 에셋:** `Items.json`의 `Potion`에 `bRefill`, 문구 표에 `Interaction.Rest`, `BP_Checkpoint`(임시 메시, 범위 구 반지름 2m, 안내 지점 1.2m), `WBP_LoadingScreen`, `BP_StageGameMode`의 `LoadingScreenClass`.
- [x] 9. **레벨 변환:** `LV_FortressBreach`의 적을 같은 자리·방향·정찰 경로·그룹의 스포너로 바꾸고 저장합니다. 체크포인트는 레벨에 놓지 않고(자리는 레벨 기획서에서 정함) 테스트 창에서 소환해 확인합니다. 작업 트리에서만 바꿨으며, 올리기 전에 기획자에게 알립니다.
- [x] 10. **시나리오 테스트:** spec의 완료 조건과 2-plan 5장의 항목(락온 중인 대상, 투사체와 충격파의 주인, 전투 중 목록과 HUD 보스 상태, 스킬 버튼의 쿨다운 표시, 미리 읽기 뒤 새로 읽히는 패키지). 결과를 `4-test.md`에 적습니다.
- [x] 11. **회귀 시나리오:** 시작하자마자 적을 찾는 시나리오를 기다리도록 고치고 전체를 실행합니다.
- [x] 12. **패키지 빌드:** 스포너의 소프트 참조만으로 적 블루프린트가 쿡되는지 확인하고, 2-plan 6장의 조건으로 After를 재어 Before와 나란히 `4-test.md`에 적습니다.
- [ ] 13. **정본 동기화 (프로그래머와 기획자 확인 뒤):** 2-plan 7장의 문서와 레벨 작업 스킬·에디터 작업 스킬의 '정찰 경로'를 고치고 `Specs/체크포인트/`를 지웁니다.
