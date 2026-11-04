# GroundSlam 충격파 작업 태스크

- [x] 1. `FAgJsonReader`: 액터 클래스 객체를 `RF_ArchetypeObject`로 생성
- [x] 2. `UAgProjectileSet` → `UAgSpawnActorSet`(ID별 액터 템플릿 맵, `Data/AgSpawnActorData.h`), `FAgProjectileDefinition`·`FAgProjectileParams` 삭제, 에셋 매니저 파일 목록·조회 함수 변경
- [x] 3. `IAgAbilitySpawnedActor` 신규, `SpawnProjectile` Task → `SpawnActor`(템플릿 지연 생성 + 인터페이스 호출)
- [x] 4. `AAgProjectile`: `Launch` 삭제, `Speed`·`Range`·`Damage` 속성, 인터페이스 구현, 비행 방향을 정면에서 읽기
- [x] 5. `AAgShockwave` 신규 (지면 배치, 반경 구간 판정, 외형 배율, 소멸)
- [x] 6. `Projectiles.json` → `SpawnActors.json`(액터 템플릿 형식, `GroundSlamShockwave` 추가), `Abilities.json`의 SpawnProjectile 항목 4개 이름 변경과 `GroundSlam` `End` 섹션 교체
- [x] 7. 빌드, 에디터 재시작, JSON 로드 오류 없음 확인
- [x] 8. `BP_GroundSlamShockwave` 생성 (기존 메시로 반경 1m 고리 외형)
- [x] 9. 시나리오 테스트 (Spec 3의 완료 조건, 회귀: 검기 적중·관통·패리 반사·벽 소멸·사거리, 탄환 조준 발사) 후 `4-test.md` 기록
- [ ] 10. 사람 확인 뒤 문서 반영 (2-plan '문서 반영'), `Specs/GroundSlam 충격파/` 삭제
