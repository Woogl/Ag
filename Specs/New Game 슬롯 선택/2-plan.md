# New Game 슬롯 선택 기술 설계 (Plan)

## 1. 영향 범위 및 아키텍처

New Game 화면은 Load Game 화면과 슬롯 줄·포커스·뒤로 처리·확인 창 띄우기가 같고, 줄을 실행했을 때의 일과 처음 포커스만 다릅니다. 그래서 공통 부분을 부모 클래스로 올리고 두 화면을 그 자식으로 둡니다. 한 클래스에 화면 종류 플래그를 두는 방식은 쓰지 않습니다.

| 클래스 | 변경 | 내용 |
| - | - | - |
| `UAgSaveSlotScreen` (신규, Abstract) | `UAgMenuScreen` 자식 | 지금 `UAgLoadGameScreen`에 있는 공통 부분: 슬롯 줄을 슬롯 수만큼 한 번 만들기(`SlotRowClass`, `SlotList`), 켜질 때 저장 파일을 읽어 줄 다시 채우기, 포커스를 줄 슬롯(`FocusSlot`)과 첫 포커스 대상, Back 버튼과 뒤로 처리, 확인 창 띄우기(`ConfirmDialogClass`, 슬롯 이름 + 문구), 스테이지 선택 화면 열기(`StageSelectScreenClass`). 자식이 정하는 것은 가상 함수 둘: 줄을 실행했을 때(`HandleSlotPicked`), 처음 포커스할 슬롯(`FindDefaultFocusSlot`). |
| `UAgLoadGameScreen` | 부모를 `UAgSaveSlotScreen`으로 | 남는 것: 줄 실행 = 불러오기(지금과 같음), Delete와 삭제 확인(`DeleteMessage`), 처음 포커스 = 저장한 시각이 가장 늦은 저장된 슬롯. |
| `UAgNewGameScreen` (신규) | `UAgSaveSlotScreen` 자식 | 줄 실행: 빈 슬롯이면 바로, 아니면 덮어쓰기 확인 창(`OverwriteMessage`)의 Yes 뒤에 새 게임을 만들고 스테이지 선택 화면을 엶. 처음 포커스 = 번호가 가장 작은 빈 슬롯, 없으면 Slot 1. |
| `UAgSaveSlotRow` | 줄의 쓰임을 화면이 알려 줌 | `SetSlot`에 이 화면이 모든 슬롯을 고를 수 있는지를 받습니다(New Game 화면만 참). 참이면 빈 슬롯·읽을 수 없는 저장도 슬롯 버튼으로 보이고(스테이지 자리에 'Empty'·'Corrupted'), Delete 버튼은 접어(Collapsed) 줄이 화면 가운데에 옵니다. 거짓이면 지금과 같습니다. 버튼과 알림 이름을 쓰임에 맞게 바꿉니다: `LoadButton` → `SlotButton`, `OnLoadRequested` → `OnSlotPicked`, `GetLoadButton` → `GetSlotButton`. |
| `UAgSaveSubsystem` | `StartNewGame` | 슬롯 번호를 받아 그 슬롯에 새 게임을 만들고 저장합니다. 덮어쓸 때도 같은 함수이며, 엔진 `SaveGameToSlot`이 파일을 통째로 덮어쓰므로 지우기를 따로 하지 않습니다. 빈 슬롯을 찾던 코드는 지웁니다. |
| `UAgTitleScreen` | New Game | New Game은 New Game 화면(`NewGameScreenClass`)을 엽니다. New Game 버튼은 항상 보이므로, 빈 슬롯을 세던 코드와 그 버튼을 접던 코드를 지웁니다. `StartNewGame` 함수와, 쓸 곳이 Continue 하나만 남는 `StageSelectScreenClass`는 그대로 둡니다. |

- 스테이지 선택 화면의 Back은 지금처럼 그 화면을 연 화면(스택의 아래 화면)으로 돌아가므로 고칠 것이 없습니다. 돌아오면 `FocusSlot`이 고른 줄을 가리킵니다.
- 공통 규칙: [캐릭터 기반 구조와 GAS](<../../Docs/Architecture/캐릭터 기반 구조와 GAS.md>)의 UI 규칙(CommonUI 스택, Modal 레이어의 확인 창, `UAgButton`)을 그대로 따릅니다. 새 공통 규약은 없습니다.

## 2. GAS 및 데이터 설계

- 태그, 어빌리티, JSON 변경은 없습니다.
- 문구 표(`Content/StringTables/StringTable.csv`)에 `Menu.OverwriteSave,"이 저장 데이터를 덮어쓰시겠습니까?","Overwrite this save data?"`를 더합니다.

### 에셋

| 에셋 | 변경 |
| - | - |
| `WBP_NewGame` (신규) | `WBP_LoadGame`을 복제하고 부모를 `UAgNewGameScreen`으로 바꿉니다. 제목 글자는 'New Game'. 기본값: `SlotRowClass`, `StageSelectScreenClass`, `ConfirmDialogClass`는 `WBP_LoadGame`과 같은 에셋, `OverwriteMessage`는 문구 표의 `Menu.OverwriteSave`. |
| `WBP_LoadGame` | 부모 클래스는 그대로입니다. 부모로 올라간 속성은 이름이 같아 저장된 값이 그대로 읽히는지 확인합니다. |
| `WBP_SaveSlotRow` | 버튼 위젯 이름을 `LoadButton`에서 `SlotButton`으로 바꿉니다. |
| `WBP_Title` | `NewGameScreenClass` = `WBP_NewGame`. |

## 3. 인터페이스 & 함수 시그니처

```cpp
// UAgSaveSlotScreen
protected:
	virtual void HandleSlotPicked(int32 SlotIndex) PURE_VIRTUAL(UAgSaveSlotScreen::HandleSlotPicked, );
	virtual int32 FindDefaultFocusSlot(const TArray<FAgSaveSlotInfo>& Slots) const PURE_VIRTUAL(UAgSaveSlotScreen::FindDefaultFocusSlot, return INDEX_NONE;);
	/** 확인 창을 Modal 레이어에 엽니다. 창을 닫으면 그 슬롯의 줄이 선택됩니다. */
	void OpenConfirmDialog(int32 SlotIndex, const FText& Message, FSimpleDelegate OnConfirmed);
	/** 스테이지 선택 화면에서 Back으로 돌아오면 그 슬롯의 줄이 선택됩니다. */
	void OpenStageSelect(int32 SlotIndex);
	/** 저장 파일을 다시 읽어 줄을 채웁니다. 이 화면이 모든 슬롯을 고를 수 있는지(`bPickAnySlot`)를 줄에 넘깁니다. */
	void RefreshSlots();

	/** UAgNewGameScreen 생성자에서 true로 둡니다. */
	bool bPickAnySlot = false;

// UAgSaveSlotRow
	void SetSlot(int32 InSlotIndex, const FAgSaveSlotInfo& Info, bool bPickAnySlot);
	UWidget* GetSlotButton() const;
	FSimpleMulticastDelegate OnSlotPicked;

// UAgSaveSubsystem
	/** 그 슬롯에 새 게임을 만들어 바로 저장하고, 지금 진행으로 삼습니다. 저장된 슬롯이면 덮어씁니다. */
	void StartNewGame(int32 Slot);
```

- `bPickAnySlot`은 UPROPERTY가 아닌 C++ 값입니다. 화면 클래스가 정하는 규칙이라 에셋에서 바꿀 일이 없기 때문입니다.
