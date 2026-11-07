# 스킬 4 작업 태스크

- [x] 1. `AgGameplayTags`에 `GameplayCue.Skill4Slam`을 선언합니다.
- [x] 2. `FAgAreaHitParams`에 `Cue`를 더하고, `UAgAbilityTask_AreaHit::Activate`가 판정 전에 발밑에서 큐를 실행하게 합니다(`RawMagnitude` = `Area.Range`, `Normal` = 정면 수평 방향).
- [x] 3. `UAgGameplayCueNotify_Burst`에 `MagnitudeNiagaraParameter`를 더해, 값이 있으면 생성한 나이아가라에 `RawMagnitude`를 넣습니다.
- [x] 4. 빌드하고 에디터를 다시 띄웁니다(스킬 1·3 세션과 합쳐 스킬 1 세션이 실행).
- [x] 5. `AM_Player_Skill4`를 만듭니다(`Attack12`, `DefaultSlot`, `RateScale` 1.2, 휩쓸기 0.28~0.47초·내려찍기 0.86~1.08초에 `GameplayCue.WeaponTrail` 상태 노티파이).
- [x] 6. `NS_Skill4Slam`과 전용 머티리얼(`M_Skill4Shockwave`, `M_Skill4Glow`, `M_Skill4Dust`)을 만듭니다.
- [x] 7. `GC_Skill4Slam`을 만듭니다(부모 `UAgGameplayCueNotify_Burst`, `NS_Skill4Slam` 붙이지 않고 재생, `MagnitudeNiagaraParameter` = `User.Radius`).
- [x] 8. `Abilities.json` `Skill4`를 설계대로 바꿉니다.
- [x] 9. 시나리오 테스트로 완료 조건을 확인하고 `4-test.md`에 적습니다.
- [ ] 10. VFX 모양과 템포를 사용자 플레이로 확인합니다.
- [ ] 11. 4단계: 기획서와 구현 문서를 반영하고 `Specs/스킬 4/`를 지웁니다.
