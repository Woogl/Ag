# UI

## 위젯 편집

- 위젯 애니메이션은 MCP와 파이썬으로 만들 수 없다. `AgAssetToolset.SetFadeAnimation`·`RemoveWidgetAnimation`을 쓴다.
- UMG를 MCP로 다룰 때:
  - `OverlaySlot`은 기본이 왼쪽 위 정렬이라 배경 이미지는 채우기로 바꾼다.
  - `ProgressBar`의 기본 스타일은 엔진 텍스처를 쓴다. 단색 막대는 `ResourceName`을 "None", `ImageType`을 NoImage로 둔다.
  - `GetWidgets`는 부모 클래스의 BindWidget 이름도 `widget: None`으로 보여 준다. 이름이 이미 쓰였는지는 `widget`이 있는지로 판단한다.
- 슬레이트는 틴트 알파가 0인 박스를 그리지 않는다. 채움이 투명한 둥근 박스는 외곽선도 보이지 않는다.
- `FKey::GetDisplayName()`은 에디터 언어를 따른다(한국어 에디터에서 '왼쪽 Shift').

## PIE에서 확인

- CommonUI는 에디터에서 PIE 뷰포트가 포커스를 가질 때만 입력 설정과 첫 포커스를 적용한다. 뒤에 있는 PIE에서 버튼 선택이 보이지 않으면 버그로 보기 전에 포커스부터 확인한다. 사람의 마우스가 떠 있는 PIE 창을 지나가도 버튼이 선택되니, 선택이 이상하면 `is_hovered()`와 커서 위치를 본다.
- CommonUI 버튼은 `<화면>.WidgetTree_0.<버튼>.WidgetTree_0.InternalRootButtonBase`의 `on_clicked.broadcast()`로 누른다. 화면은 `is_activated()`로 고른다. 이전 월드의 화면이 GameInstance에 남아 있다.
