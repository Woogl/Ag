# 일시정지 메뉴 탭 작업 태스크

- [x] 1. 문구 표 항목 다섯 개(카테고리 이름 둘, '보유', 탭 이름 둘)(`en` 칼럼에 영어 번역 함께)
- [x] 2. C++: `UAgTabList`, `UAgTabButton`, `UAgInventoryTab`, `UAgInventoryEntry`, `UAgInventorySlot`, `UAgItemDetail`, `UAgSystemTab` 추가, `UAgPauseMenu` 변경, `PushScreen`이 화면을 돌려줌, HUD가 일시정지 메뉴가 켜진 동안 `StatusPanel` 숨김, `AAgPlayerController`의 `IMC_UI` 우선순위 1
- [x] 3. 빌드(에디터를 닫고 전체 빌드, 다시 켬)
- [x] 4. `T_Key_E` 가공·가져오기, 컨트롤러 데이터 등록
- [x] 5. `IA_PreviousTab`·`IA_NextTab`(소비 끔), `IMC_UI`에 Q·E 매핑
- [x] 6. 위젯 블루프린트: `ButtonStyle_Tab`, `WBP_TabButton`, `WBP_TabList`, `ButtonStyle_InventorySlot`, `WBP_InventorySlot`, `WBP_ItemDetail`, `WBP_InventoryTab`, `WBP_SystemTab`, `WBP_PauseMenu` 다시 구성, 흐린 글자 스타일 `TS_MediumDim`·`TS_SmallDim`
- [x] 7. 테스트 창에서 완료 조건 확인(LV_SimulationRoom 또는 미니던전), 화면 캡처로 그림과 비교, 시험용 재화 아이템(테스트 창 사본 JSON에만)으로 여러 칸·스크롤·방향키, 회귀(궁극기 Q, 기존 회귀 시나리오 중 일시정지·회복병 관련)
- [ ] 8. 4-test 기록, 정본 반영(1-spec 2.8, 2-plan 4절), `Specs/일시정지 메뉴 탭` 삭제
