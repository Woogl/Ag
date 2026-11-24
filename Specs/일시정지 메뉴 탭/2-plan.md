# 일시정지 메뉴 탭 기술 설계 (Plan)

## 1. 영향 범위 및 아키텍처

탭 바와 내용 전환은 CommonUI의 표준 부품을 그대로 씁니다. 탭 목록은 `UCommonTabListWidgetBase`(Q·E 입력, 버튼 그룹의 단일 선택, 끝에서 처음으로 넘어가기), 내용은 `UCommonActivatableWidgetSwitcher`(메뉴가 켜져 있을 때 고른 탭의 내용만 활성화)입니다. 탭 하나를 더하는 일은 내용 위젯 블루프린트 하나와 일시정지 메뉴 기본값의 탭 배열 한 줄입니다.

| 클래스 | 부모 | 변경 | 역할 |
| - | - | - | - |
| `UAgPauseMenu` | `UAgMenuScreen` | 변경 | 일시정지 메뉴 화면. 켜지면 게임을 멈춤, 꺼지면 게임을 이어 감, 뒤로 처리(`bIsBackHandler`). 버튼 네 개는 `UAgSystemTab`으로 옮기고 탭 목록(`TabList`)과 내용 전환기(`TabContent`)를 가짐 |
| `UAgTabList` | `UCommonTabListWidgetBase` | 추가 | 탭 바. 탭 배열(`Tabs`: 탭 이름 + 내용 위젯 클래스)로 내용 위젯을 한 번 만들어 연결된 전환기에 넣고, 화면에 붙을 때마다 탭을 등록하고(엔진이 화면에서 빠질 때 탭을 모두 지우므로) 기본 탭(`DefaultTabIndex`, 일시정지 메뉴는 System)을 고름. 메뉴가 켜진 뒤에 탭을 바꾸면 지난번 탭이 먼저 켜져 CommonUI의 첫 포커스가 그쪽에 남으므로, 메뉴가 켜지기 전인 이때 고름. 탭 버튼을 가로 상자에 놓고 이름을 넣음. 양 끝 키 아이콘(`UAgActionWidget`)에 엔진 속성 `PreviousTabEnhancedInputAction`·`NextTabEnhancedInputAction`을 넘김 |
| `UAgTabButton` | `UCommonButtonBase` | 추가 | 탭 이름 버튼. 글자 모양은 버튼 스타일의 보통·마우스 올림·선택 글자 스타일, 선택되면 밑줄 이미지를 보이고 풀리면 숨김. 방향키 포커스를 받지 않음(`bIsFocusable` 끔) |
| `UAgInventoryTab` | `UCommonActivatableWidget` | 추가 | Inventory 탭 내용. 켜질 때마다 인벤토리와 아이템 데이터로 그리드(`ItemGrid`, CommonUI `UCommonTileView`)의 항목을 다시 채우고 첫 칸을 고름(메뉴가 열린 동안 게임이 멈춰 보유 수가 바뀌지 않으므로 바뀜 알림은 받지 않음). 그리드의 선택 변경 알림(`OnItemSelectionChanged`)으로 상세 패널을 갱신. 비면 상세 패널을 숨김. 처음 포커스는 그리드 |
| `UAgInventoryEntry` | `UObject` | 추가 | 그리드 항목 하나의 데이터(아이템 ID, 보유 수). 엔진 리스트 뷰가 항목을 `UObject`로 받기 때문에 둠 |
| `UAgInventorySlot` | `UCommonButtonBase`, `IUserObjectListEntry` | 추가 | 그리드 칸. 항목을 받으면(`NativeOnListItemObjectSet`) 아이콘과 보유 수를 그림. CommonUI 타일 뷰는 버튼 항목을 `SCommonButtonTableRow`로 감싸 버튼의 선택과 리스트의 선택을 맞추므로, 테두리·바탕·효과음은 버튼 스타일(`ButtonStyle_InventorySlot`)이 맡고, 좌클릭은 고르기, 마우스 올림은 올림 모양만 냄(선택은 좌클릭과 방향키) |
| `UAgItemDetail` | `UCommonUserWidget` | 추가 | 상세 패널. 아이템 ID와 보유 수로 이름·카테고리 이름(카테고리 → 문구 표 항목 맵 `CategoryNames`, 위젯 기본값)·'보유 수 / 최대 보유 수'·아이콘·설명을 그림(`SetItem`) |
| `UAgSystemTab` | `UCommonActivatableWidget` | 추가 | System 탭 내용. Resume(자신을 담은 `UAgPauseMenu`를 끔, 엔진 `UCommonUILibrary::FindParentWidgetOfType`), Restart·Return to Title·Quit(게임 모드). 처음 포커스는 Resume |

- HUD 숨김: `UAgUILayout::PushScreen`·`AAgPlayerController::PushScreen`이 스택이 만든(또는 다시 쓰는) 화면을 돌려줍니다. `UAgHUDWidget::OpenPauseMenu`가 그 화면을 약한 포인터로 들고 화면의 켜짐·꺼짐 알림(`OnActivated`·`OnDeactivated`, 다시 쓰는 인스턴스라 먼저 `RemoveAll`)에 `UpdateStatusPanel`을 겁니다. `UpdateStatusPanel`의 숨김 조건(컷신, 스테이지 결과)에 '일시정지 메뉴가 켜져 있음'을 더합니다. `StatusPanel`이 플레이어 상태·슬롯·보스 상태를 담고 있어 지금 숨김과 같은 범위입니다.
- 입력: `IA_PreviousTab`(Q)·`IA_NextTab`(E)을 `IMC_UI`에 더합니다. 두 액션은 `bConsumeInput`을 끕니다. Enhanced Input은 매핑을 다시 만들 때 우선순위가 높은 맥락의 소비 액션이 쓴 키를 낮은 맥락에서 지우므로(`RebuildControlMappings`), 소비하면 Q의 궁극기가 사라지고, 반대로 `IMC_Default`가 위면 CommonUI가 Q를 탭 액션의 키로 찾지 못합니다. 그래서 `AAgPlayerController`가 `IMC_UI`를 `IMC_Default`(우선순위 0)보다 높은 1로 겁니다. 게임 입력 모드에서는 탭 액션을 듣는 위젯이 없어 Q는 궁극기만 냅니다.
- 키 아이콘: `T_Key_E`를 다른 키 아이콘과 같은 방식(Kenney 원본 가공)으로 만들어 `BP_KeyboardMouseControllerData`에 등록합니다.
- 그리드: `UCommonTileView`의 칸 크기는 144 × 176(칸 128 × 160 + 칸 사이 16, 엔진이 칸의 오른쪽·아래에 붙임)이고, 그리드 영역은 7칸 너비에 세로 스크롤 막대 몫(두께 16 + 둘레 2 × 2)을 더한 1028 × 704입니다. 막대 몫이 없으면 4줄을 넘을 때 한 줄이 6칸이 됩니다. 상세 패널은 마지막 칸 오른쪽 끝에서 48px이 되도록 그리드 영역 뒤 12px에 둡니다. UMG 타일 뷰는 줄 끝의 ←·→가 다음 줄로 넘어가지 않는 것(`bWrapHorizontalNavigation` 꺼짐)이 기본입니다(UI 조작의 '끝에서 멈춤'). 선택 방식은 하나(`Single`), 스크롤은 엔진 타일 뷰 그대로(휠, 방향키로 고른 칸 보이기).
- 아이콘: 칸과 상세 패널 모두 소프트 참조 아이콘을 HUD 칸과 같이 CommonUI `UCommonLazyImage::SetBrushFromLazyTexture`로 비동기 로드해 그립니다.
- 위젯 블루프린트: `WBP_PauseMenu`(탭 바 + 전환기로 다시 구성), `WBP_TabList`, `WBP_TabButton`(+ 버튼 스타일 `ButtonStyle_Tab`), `WBP_InventoryTab`, `WBP_InventorySlot`(+ `ButtonStyle_InventorySlot`), `WBP_ItemDetail`, `WBP_SystemTab`. 탭 배열과 기본 탭은 `WBP_PauseMenu`의 `TabList` 값, 탭 액션과 탭 버튼 클래스는 `WBP_TabList` 기본값, 칸 위젯 클래스는 `WBP_InventoryTab`의 `ItemGrid` 값에 둡니다(AGENTS.md 2번: 실행 구조의 클래스·에셋 참조는 쓰는 객체의 에셋 기본값). 탭 이름, '보유', 카테고리 이름은 문구 표 항목을 가리킵니다.
- 효과음: 탭 버튼과 칸의 버튼 스타일(`ButtonStyle_Default`를 복제)에서 실행 효과음을 '클릭' 소리가 아니라 '누를 때' 소리(`PressedSlateSound`)로 둡니다. 엔진은 Q·E로 탭을 고를 때(`SetIsSelected(true, true)` → `SetSelectedInternal`) '누를 때' 소리만 내기 때문이며, 좌클릭에서 두 번 나지 않게 '클릭' 소리는 비웁니다. 방향키로 칸을 고를 때는 엔진이 소리 없이 고르며, 이는 다른 버튼의 방향키 선택과 같습니다. 메뉴를 열 때(탭 바가 화면에 붙을 때) 기본 탭을 고르는 것은 피드백 없이 고릅니다.
- 글자 스타일: 상세 패널의 흐린 카테고리 이름과 '보유'에 `TS_MediumDim`·`TS_SmallDim`을 더합니다(`TS_LargeDim`과 같은 색).
- 배치: `WBP_PauseMenu`의 내용 전환기는 화면 전체를 채우고 탭 바를 그 위(화면 위쪽 여백 48px)에 겹칩니다. System 탭의 버튼이 지금처럼 화면 가운데에 놓이고, Inventory 탭은 탭 바 아래 72px(화면 위에서 172px)부터 놓입니다.
- 기존 공통 규칙: 위젯 Tick 없음(`DisableNativeTick`), 메뉴 입력 모드는 `UAgMenuScreen` 그대로, 기본 방침의 '위젯 갱신'(이벤트로만 갱신)을 지킵니다.
- MVVM: 목록 화면이 처음 생기지만, 켜질 때 한 번 채우고 고른 칸만 상세 패널에 넘기는 화면이라 쓰지 않습니다. 5.8의 MVVM 플러그인도 아직 Beta입니다.

## 2. GAS 및 데이터 설계

- GameplayTag·어빌리티·이펙트 변경 없음.
- 아이템 데이터 변경 없음. 상세 패널이 쓰는 `FAgItemData`의 `DisplayName`·`Description`·`Icon`(`TSoftObjectPtr<UTexture2D>`, 게임 시작 때 읽지 않음)과 `Items.json`·문구 표의 회복병 값은 아이템 작업이 이미 넣었습니다.
- `Content/StringTables/StringTable.csv`: `ItemCategory.Consumable`(소비 아이템), `ItemCategory.Currency`(재화 아이템), `Inventory.Count`(보유), `Menu.Inventory`(Inventory), `Menu.System`(System).
- 영어 번역: 영어 지원 작업의 형식대로 문구 표의 `en` 칼럼에 함께 적습니다(`키,"한국어","English"` 한 줄, 파일 맨 앞 BOM 유지). 원문은 한국어 모드에서 보일 글자이며, 메뉴 글자 둘은 두 언어에서 같은 영어(Inventory, System), 카테고리 이름은 용어집 영문 표기(Consumable, Currency), '보유'는 Owned입니다.
- 목록 순서: `UAgAssetManager::GetItems()`(JSON의 순서를 지키는 `TMap`)를 카테고리 값 순서로 안정 정렬합니다(`Algo::StableSortBy`).

## 3. 인터페이스 & 함수 시그니처

```cpp
// UI/AgTabList.h
USTRUCT()
struct FAgTab
{
	UPROPERTY(EditAnywhere) FText Label;
	UPROPERTY(EditAnywhere) TSubclassOf<UCommonActivatableWidget> Content;
};

UCLASS(Abstract, meta = (DisableNativeTick))
class UAgTabList : public UCommonTabListWidgetBase
{
public:
protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void HandleTabCreation_Implementation(FName TabNameID, UCommonButtonBase* TabButton) override;
	UPROPERTY(EditAnywhere) TArray<FAgTab> Tabs;
	UPROPERTY(EditAnywhere) int32 DefaultTabIndex = 0;
	UPROPERTY(EditAnywhere) TSubclassOf<UAgTabButton> TabButtonClass;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UAgActionWidget> PreviousTabAction;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UHorizontalBox> TabButtonBox;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UAgActionWidget> NextTabAction;
private:
	UPROPERTY(Transient) TArray<TObjectPtr<UCommonActivatableWidget>> ContentWidgets;
};

// UI/AgTabButton.h
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgTabButton : public UCommonButtonBase
{
public:
	UAgTabButton(const FObjectInitializer& ObjectInitializer);
	void SetLabel(const FText& Label);
protected:
	virtual void NativeOnCurrentTextStyleChanged() override;
	virtual void NativeOnSelected(bool bBroadcast) override;
	virtual void NativeOnDeselected(bool bBroadcast) override;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCommonTextBlock> LabelText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UImage> Underline;
};

// UI/AgUILayout.h, Game/AgPlayerController.h
UCommonActivatableWidget* PushScreen(EAgUILayer Layer, TSubclassOf<UCommonActivatableWidget> ScreenClass);

// UI/AgHUDWidget.h
private:
	TWeakObjectPtr<UCommonActivatableWidget> PauseMenu;

// UI/AgPauseMenu.h (버튼 네 개 대신)
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UAgTabList> TabList;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCommonActivatableWidgetSwitcher> TabContent;

// UI/AgInventoryTab.h
UCLASS()
class UAgInventoryEntry : public UObject
{
public:
	FName ItemId;
	int32 Count = 0;
};

UCLASS(Abstract, meta = (DisableNativeTick))
class UAgInventoryTab : public UCommonActivatableWidget
{
protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCommonTileView> ItemGrid;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UAgItemDetail> ItemDetail;
private:
	void HandleSelectionChanged(UObject* Item);
};

// UI/AgInventorySlot.h
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgInventorySlot : public UCommonButtonBase, public IUserObjectListEntry
{
protected:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCommonLazyImage> Icon;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCommonTextBlock> CountText;
};

// UI/AgItemDetail.h
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgItemDetail : public UCommonUserWidget
{
public:
	void SetItem(FName ItemId, int32 Count);
protected:
	UPROPERTY(EditAnywhere) TMap<EAgItemCategory, FText> CategoryNames;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCommonTextBlock> NameText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCommonTextBlock> CategoryText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCommonTextBlock> CountText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCommonLazyImage> Icon;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCommonTextBlock> DescriptionText;
};

// UI/AgSystemTab.h
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgSystemTab : public UCommonActivatableWidget
{
protected:
	virtual void NativeOnInitialized() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UAgButton> ResumeButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UAgButton> RestartButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UAgButton> ReturnToTitleButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UAgButton> QuitButton;
private:
	void ClosePauseMenu();
};
```

## 4. 문서 반영 (4단계)

- 기획서: 1-spec 2.8의 표.
- `Docs/Architecture/캐릭터 기반 구조와 GAS.md`: 게임 모드 절의 표에 새 클래스 여덟 개, 다이어그램에 일시정지 메뉴 관계, '키 아이콘' 설명에 `IMC_UI` 우선순위와 탭 액션의 소비 끔.
- `Docs/Art/Attachments/일시정지 메뉴 화면 구성.svg`를 두 탭 그림으로 바꿈.
