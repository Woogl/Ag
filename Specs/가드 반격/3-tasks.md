# 가드 반격 작업 태스크

- [x] 1. `Attack_HeavyB`의 베기 시각(내려베기 시작, 올려베기 끝)과 전진 구간을 잰다.
- [x] 2. 태그 `Ability.Action.GuardCounter`, `State.GuardCounterChance`와 `EAgCameraSituation::GuardCounterHit`을 더한다.
- [x] 3. 피격 결과 처리 컴포넌트에서 패리·가드 결과에 `GuardCounterChance`를 건다.
- [x] 4. 플레이어 캐릭터의 기회 제거 대상, 가드 브레이크 제거, 기본 공격 키 처리, 적중 카메라 연출을 고친다.
- [x] 5. C++ 빌드 후 에디터에서 `AM_Player_GuardCounter`를 만들고 `BP_Player`의 `CameraEffects`에 `GuardCounterHit`을 넣는다.
- [x] 6. `Effects.json`, `Abilities.json`, `Characters.json`, `MontageTimelines.json`에 가드 반격을 더하고 회피 반격 값을 바꾼다.
- [x] 7. 1-spec 완료 조건을 확인한다. (사람이 직접 플레이해 확인)
