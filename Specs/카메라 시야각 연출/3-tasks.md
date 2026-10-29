# 카메라 시야각 연출 작업 태스크

- [x] 1. `UAgFieldOfViewCameraModifier`와 `FAgFieldOfViewEffect` 추가
- [x] 2. `FAgCameraEffect`에 `FieldOfView` 추가, `PlayCameraEffect`에서 재생
- [x] 3. 플레이어 캐릭터: 모디파이어 붙이기, `SprintFieldOfView`와 `State.Sprinting`·`Input.Move` 태그 이벤트, 궁극기 시작 시 `StopAll`
- [x] 4. 노티파이 이름을 `UAgAnimNotify_PlayCameraEffect`로 바꾸고 `FieldOfView` 추가, 임시 `[CoreRedirects]` 추가
- [x] 5. 에디터를 닫고 전체 빌드, 에디터 다시 켜기
- [x] 6. 7개 `CS_*`의 시야각 값을 `BP_Player`의 `CameraEffects`와 `AM_Boss_PhaseTransition` 노티파이로 옮기고, `CameraShakes`에서 그 에셋을 빼기. 패리 성공의 `CS_ParryShake`를 `CS_Medium`으로 바꾸기. `SprintFieldOfView` 값 넣기. 저장
- [x] 7. `AM_Boss_GroundSlam`, `AM_Player_Ultimate`도 다시 저장하고, 세 몽타주에 옛 노티파이 이름이 없는지 확인
- [x] 8. 7개 `CS_*` 에셋, `CS_ParryShake`, `UAgOffsetCameraShakePattern` 삭제, 임시 리다이렉트 삭제, 다시 빌드
- [x] 9. 남은 참조가 없는지 확인 (`grep -a`)
- [ ] 10. 사용자 플레이 확인 (1-spec '완료 조건')
- [ ] 11. 정본 동기화(2-plan '정본 동기화') 후 작업 폴더 삭제
