# 스킬 4종 확장 작업 태스크

- [x] 1. `Input.Skill1`~`Input.Skill4` 태그를 더하고 `Input.Skill`을 지웁니다. `AAgPlayerCharacter`의 입력 액션을 넷으로 바꿉니다.
- [x] 2. `FAgAbilityData::Icon`을 더하고, `Abilities.json`(`Skill1`~`Skill4`, 두 아이콘)과 `Characters.json`을 고칩니다.
- [x] 3. `UAgSkillSlot`을 만들고 `UAgHUDWidget`의 칸 코드를 칸 다섯 개로 옮깁니다.
- [x] 4. 빌드합니다. 헤더가 바뀌므로 에디터를 닫고 빌드한 뒤 다시 엽니다.
- [x] 5. 입력 에셋: `IA_Skill`을 복제해 `IA_Skill1`~`IA_Skill4` 생성 뒤 `IA_Skill` 삭제, `IMC_Default` 매핑, `BP_Player` 기본값 (리다이렉터 없음).
- [x] 6. `WBP_SkillSlot`을 만들고 `WBP_HUD`에 십자·키 표시·궁극기 칸을 배치합니다. `M_CooldownSweep`·`M_UPGauge`를 원형으로 고칩니다.
- [x] 7. `WBP_ControlsGuide`의 스킬 줄을 고칩니다.
- [x] 8. `runner.py`의 키 표와 `player_skill.py`를 고칩니다.
- [x] 9. 시나리오 테스트로 1-spec의 완료 조건을 확인하고 `4-test.md`에 기록합니다. HUD 배치는 게임 화면 캡처로 확인합니다. 회귀 시나리오 `player_skill.py`를 함께 돌립니다.
- [ ] 10. 사용자 플레이 확인을 받습니다.
- [ ] 11. 확인을 받으면 4단계(기획서·구현 문서·아트 문서 반영, Specs 폴더 삭제)를 진행합니다.
