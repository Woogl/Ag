# 스킬 4 소용돌이 작업 태스크

## 코드

- [x] 1. `AAgShockwave`에 파동마다 공격 데이터(`Hits`)와 파동 사이 간격(`Interval`), 나이아가라 파라미터, VFX가 끝난 뒤 사라지기를 더하고, 외형 메시가 있을 때만 액터 배율을 맞추게 합니다. (2-plan 1.2, 4.1)
- [x] 2. AreaHit `Cue`, Burst 큐의 `MagnitudeNiagaraParameter`, `GameplayCue.PunishingEdge` 태그를 지웁니다. (2-plan 1.3, 4.2)
- [x] 3. 공유 에디터를 쓰는 다른 세션과 맞춘 뒤 에디터를 닫고 `AgEditor`를 빌드합니다.

## 에셋과 데이터

- [x] 4. `SpawnActors.json`의 `GroundSlamShockwave`·`PhaseTransitionShockwave`의 `Damage`를 `Hits` 한 칸으로 옮기고, `PunishingEdgeVortex`를 더합니다. `Abilities.json` `Skill4`의 AreaHit을 SpawnActor로 바꿉니다. (2-plan 2절)
- [x] 5. `BP_PunishingEdgeVortex`(부모 `AAgShockwave`)를 만들고 `NS_PunishingEdge` 나이아가라 컴포넌트를 둡니다. `GC_PunishingEdge`를 지웁니다.

## 검증

- [x] 6. `player_skill4.py`를 2-plan 5절의 기준으로 쓰고 시뮬레이션 룸에서 실행합니다.
- [x] 7. 일회용 시나리오로 GroundSlam 충격파와 페이즈 전환 밀어내기가 플레이어를 맞히는지 보고 지웁니다. 회귀 시나리오 `player_skill`, `boss_patterns`를 실행합니다.
- [x] 8. 결과와 로그를 `4-test.md`에 적습니다.

## VFX

- [x] 9. 소용돌이 VFX 레퍼런스 화면을 모으고, 실제 게임 화면 위에 GIF 후보 4~6개를 만들어 보여 드립니다.
- [x] 10. 고르신 후보로 `NS_PunishingEdgeVortex`와 머티리얼을 만들어 `BP_PunishingEdgeVortex`에 더합니다. `User.Radius`·`User.Duration`을 읽고, 끝이 있는 시스템으로 만듭니다.
- [x] 11. 테스트 창의 기본 카메라 화면을 GIF로 만들어 보여 드립니다.

## 마무리 (4단계)

- [ ] 12. 플레이 확인을 받은 뒤 2-plan 6절의 문서를 고치고 `Specs/스킬 4 소용돌이/`를 지웁니다.
