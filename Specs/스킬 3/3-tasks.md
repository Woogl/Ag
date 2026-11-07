# 스킬 3 장풍 작업 태스크

- [x] 1. C++: `Ability.Action.Skill.1` ~ `.4` 태그, Transition `bCommitAbility`, SpawnActor `Roll`, 투사체 Roll 유지, `UAgGameplayAbility::GetCharges`, `UAgSkillSlot` 충전 숫자, `UAgHUDWidget` 부여·다시 읽기 때 충전 다시 그리기
- [x] 2. 빌드 (헤더가 바뀌므로 에디터를 닫고 빌드한 뒤 다시 엽니다. 다른 세션이 에디터를 쓰는지 먼저 확인)
- [x] 3. 에셋: `AM_Player_Skill3` (ACS2 `Combo01_1`~`01_3`, 섹션 3개·링크 없음, `RateScale` 1.2, 무기 궤적 노티파이)
- [x] 4. 에셋: `BP_PlayerSwordWave` (`BP_SwordWave` 복제, 흰색 반투명)
- [x] 5. 에셋: `WBP_SkillSlot`에 `ChargeText`
- [x] 6. 데이터: `Abilities.json` `Skill3`, `Skill1`·`Skill2`·`Skill4`의 `AbilityTags`, `HitReward` 스킬 줄 나누기, `SpawnActors.json` `PlayerSwordWave`. 고친 뒤 스킬 1·4 세션에 바뀐 태그를 알림
- [x] 7. 장풍 시각·기울기 확인: 몽타주에서 칼끝이 앞을 지나는 순간과 장풍 생성 시각, 칼 궤적과 장풍 기울기(부호)를 화면으로 맞춤
- [x] 8. 시나리오 테스트(완료 조건 13개)와 회귀(기본 공격 적중 보상, 스킬 2 UP 15, 보스 검기), `4-test.md` 기록
