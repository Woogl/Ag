# 영어 지원 작업 태스크

- [x] 1. 문구 표에 `en` 칼럼과 Spec 2.5의 영어 27개
- [x] 2. `UAgLanguageSubsystem` (문구 표 언어 칼럼을 번역으로 등록, 시작 언어, 언어 목록, 지금 언어, 바꾸기·저장, PIE 미리 보기)
- [x] 3. `UAgSettingsPopup` C++와 `UAgTitleScreen`의 Settings 버튼·팝업 클래스
- [x] 4. 빌드, 에디터 재시작
- [x] 5. `WBP_SettingsPopup` 제작, `WBP_Title`에 Settings 버튼, `SettingsPopupClass` 지정, 저장
- [x] 6. `DefaultGame.ini` 패키징: `InternationalizationPreset` EFIGSCJK, `CulturesToStage` ko·en
- [x] 7. 테스트 창 확인: 시작 언어(한국어 윈도우 → 한국어), 영어로 바꾼 뒤 27개가 영어, JSON 이름이 바로 바뀜, 한국어로 되돌림, 저장한 언어로 다시 시작, 번역이 없는 언어(fr) → 영어, 저장값 정리
- [ ] 8. PIE 확인(사용자에게 묻고): 타이틀 버튼 4개, Settings 팝업 표시·선택·닫기, 영어로 바꾼 뒤 조작 안내 팝업 영어, 에디터 UI 언어가 바뀌지 않음
- [ ] 9. 패키지 확인: 문구 표를 읽고 두 언어로 바뀜
- [ ] 10. `4-test.md` 기록
- [ ] 11. 정본 동기화: 게임 개요(지원 언어·시작 언어), UI 시스템(타이틀 버튼, Settings 팝업 절, UI 레이어 Modal, UI 조작), 데이터 로딩과 스키마('문구'에 언어 칼럼, '패키징'에 언어), 캐릭터 기반 구조와 GAS(서브시스템·팝업 클래스) → `Specs/영어 지원` 삭제
