# 움찔 부위별 반응 작업 태스크

- [x] 1. 5.8에서 시퀀스 좌우 반전을 굽는 방법을 확인하고, 쌍검 `Hit2` 반전 시퀀스(`A_Flinch_ArmLeft`)를 만든다.
- [x] 2. 쌍검 `Hit1`·`Hit2`·`Hit4`·`Hit5`·반전 `Hit2`·`Hit_knockdown`의 루트 모션을 켜고, `Hit_Airborne`은 끈 상태인지 확인한다.
- [x] 3. `AM_Flinch`(섹션 5개), `AM_FlinchAir`, `AM_KnockDown`을 만든다.
- [x] 4. `AgGameplayTags`에 `Event.Hit.Stagger` 하위 태그 5개를 선언한다.
- [x] 5. `UAgGameplayAbility`에 `TriggerEventTag`를 더하고, `FAgPlayMontageParams`에 `EventTag` 조건을 더한다.
- [x] 6. 피격 결과 처리에 맞은 곳 판정(`ResolveStaggerEvent`)을 더한다.
- [x] 7. `Abilities.json`의 `Flinch`, `Characters.json`의 린·커스터 항목, `MontageTimelines.json`의 넉다운 키를 바꾼다.
- [x] 8. `AM_Player_Flinch`·`AM_Player_KnockDown`·`AM_Boss_KnockDown`의 참조를 확인하고 지운다.
- [x] 9. 적 StateTree의 움찔 종료 조건이 강인도와 무관한지 확인한다.
- [x] 9-1. `AM_KnockUp`의 기상 끝부분을 쌍검 `Hit_knockdown` 끝부분(`A_KnockUp_Stand`)으로 바꾸고, `AM_Flinch` 재생 배율을 0.55초에 맞춘다.
- [ ] 10. 빌드하고, 1-spec 완료 조건의 시나리오 테스트를 준비해 실행 전에 확인을 받는다. 결과는 `4-test.md`에 적는다. (빌드 완료)
- [ ] 11. 확인을 받은 뒤 2-plan 4절의 문서를 동기화하고 `Specs/움찔 부위별 반응/`을 지운다.
