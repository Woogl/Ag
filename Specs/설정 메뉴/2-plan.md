# 설정 메뉴 기술 설계 (Plan)

## 1. 영향 범위 및 아키텍처

### 1.1 구성

| 화면 | 구성 |
| - | - |
| Settings 내용 | `UAgSettingsPanel : UCommonActivatableWidget`(`WBP_SettingsPanel`). 분류 목록은 탭 바와 같은 `UAgTabList`의 두 번째 위젯 블루프린트(`WBP_CategoryList`), 분류 내용은 엔진 `UCommonAnimatedSwitcher`(`CategoryContent`, 전환 0초)에 넣습니다. |
| 일시정지 메뉴의 Settings 탭 | `WBP_PauseMenu`의 `TabList`에 `WBP_SettingsPanel`을 내용 화면으로 하는 탭 한 줄. 탭 내용 전환기가 켜고 끕니다. |
| 타이틀의 Settings 화면 | `UAgSettingsScreen : UAgMenuScreen`(`WBP_SettingsScreen`)을 Menu 레이어에 엽니다. 안에 둔 `WBP_SettingsPanel` 인스턴스는 엔진 속성 `bAutoActivate`로 화면과 함께 켜지고, 화면이 스택에서 빠질 때 엔진이 끕니다. |
| 메뉴 화면 공통 양식 | 제목(글자 + 밑줄)은 새 `UAgMenuTitle : UCommonUserWidget`(`WBP_MenuTitle`) 하나를 Settings·스테이지 선택·적 선택 화면이 함께 씁니다. 제목 글자는 인스턴스 값 `Text`이고, 모양은 이 위젯 블루프린트 한 곳에 있습니다. 검은 배경은 각 화면의 전체 화면 이미지(`WBP_PauseMenu`의 `Background`와 같음)이고, 내용 위치는 각 화면의 배치 값입니다. |

- 분류 내용(Controls, Display, Language)은 활성화 위젯이 아닌 사용자 위젯입니다. 활성화 위젯이면 CommonUI가 가장 안쪽의 켜진 위젯(분류 내용)의 포커스 대상, 곧 첫 드롭다운에 포커스를 주어, 열 때 분류 이름이 선택되지 않습니다.
  - Controls는 지금의 `WBP_ControlsGuide`를 왼쪽 위에 둔 감싸개 `WBP_ControlsSettings`(C++ 없음)입니다. 전환기는 내용을 칸 크기로 늘리므로, 세 분류 모두 루트(오버레이) 안에서 내용을 왼쪽 위에 붙입니다.
  - Display는 새 `UAgDisplaySettings`(`WBP_DisplaySettings`), Language는 새 `UAgLanguageSettings`(`WBP_LanguageSettings`)입니다.
- 창 모드와 해상도의 주인은 엔진 `UGameUserSettings`입니다. Display 분류가 직접 읽고 바꾸며, `ApplyResolutionSettings(false)`로 적용하고 `SaveSettings()`로 저장(`GameUserSettings.ini`)합니다. 바뀌지 않은 다른 설정(수직 동기화, 품질)까지 다시 적용하는 `ApplySettings`는 쓰지 않습니다. 다음 실행에서는 엔진이 시작할 때 그 값으로 창을 엽니다(`PreloadResolutionSettings`).
  - 처음 실행의 Fullscreen, 모니터 해상도는 새 설정 파일 `Config/DefaultGameUserSettings.ini`(`FullscreenMode=0`, `bUseDesktopResolution=True`)가 정합니다. 엔진 기본값(`GetDefaultWindowMode`, 테두리 없는 창)은 정적 함수라 하위 클래스로 바꿀 수 없고, 엔진은 이 파일의 `Version`이 엔진 설정 버전(5)과 같을 때만 값을 읽습니다. 엔진을 올려 버전이 바뀌면 이 숫자도 맞춥니다.
  - 고를 수 있는 창 모드는 `WBP_DisplaySettings` 기본값 `WindowModeNames`에 있는 모드(Fullscreen, Windowed)이고, 해상도 후보는 기본값 `StandardResolutions`(Spec 2.8의 표)입니다. Fullscreen은 후보 가운데 엔진 `UKismetSystemLibrary::GetSupportedFullscreenResolutions`에 있는 것과 모니터 해상도, Windowed는 후보 가운데 모니터(`UGameUserSettings::GetDesktopResolution`)보다 가로·세로가 작은 것입니다(없으면 모니터 해상도).
  - 예전에 테두리 없는 창을 저장한 설정은 창 모드 드롭다운에 첫 항목(Fullscreen)으로 보이고 해상도 목록은 모니터 해상도 하나입니다. 개발자 PC의 저장값에만 있는 경우라 따로 옮기지 않습니다.
  - Lyra처럼 `UGameUserSettings` 하위 클래스나 설정 서브시스템은 두지 않습니다. 값과 저장을 엔진이 갖고, 쓰는 곳이 Display 분류 하나입니다.
- 언어의 주인은 지금처럼 `UAgLanguageSubsystem`입니다. `UAgLanguageSettings`가 `UAgSettingsPopup`의 언어 드롭다운 코드를 옮겨 씁니다.
- PIE에서는 엔진이 해상도 요청을 처리하지 않습니다(`UGameEngine`만 처리하고 에디터 엔진은 처리하지 않음). 창이 바뀌지 않고 에디터의 `GameUserSettings.ini`에 저장만 됩니다. 창 모드·해상도 확인은 테스트 창(독립 실행 게임)에서 합니다.

### 1.2 분류 고르기

- 분류 이름 버튼(`WBP_CategoryButton`, `UAgTabButton`)은 위젯 블루프린트 기본값에서 포커스를 받고(`Is Focusable`) 포커스를 받으면 선택되게(`bShouldSelectUponReceivingFocus`) 둡니다. ↑·↓로 포커스가 옮겨 가면 엔진이 그 버튼을 선택하고, 탭 목록이 내용 전환기를 그 분류로 바꿉니다.
- 엔진 버튼은 마우스를 올려도 포커스를 옮기지 않으므로(포커스를 옮기는 것은 우리 `UAgButton`의 동작) 마우스 올림으로는 고르지 않습니다.
- 효과음은 탭 이름과 같은 버튼 스타일(`ButtonStyle_Tab`)입니다. 포커스로 선택될 때 엔진이 효과음을 내지 않으므로(`SetIsSelected(true, false)`) 방향키로 고를 때는 소리가 없습니다. Inventory 칸과 같습니다.
- 위쪽 탭 이름(`WBP_TabButton`)은 지금처럼 C++ 기본값으로 포커스를 받지 않습니다.
- 방향키 →와 ←: 분류가 바뀔 때마다 `UAgSettingsPanel`이 분류 목록의 →를 그 분류 내용의 포커스 대상으로, 내용 칸의 ←를 고른 분류 버튼으로 명시합니다(`SetNavigationRuleExplicit`, 없으면 `Stop`). 기하 탐색에 맡기면 ←가 같은 높이의 다른 분류에 닿아 분류가 바뀝니다. 엔진 내비게이션은 포커스를 받는 위젯으로만 옮기므로, 포커스 대상은 사용자 위젯의 Desired Focus Widget을 따라 내려가 실제 콤보 상자까지 찾습니다(분류 내용 → 첫 드롭다운 → 그 안의 `ComboBox`).
- 분류는 패널이 켜질 때마다(`NativeOnActivated`) 기본 분류(Controls)로 되돌립니다. 일시정지 메뉴의 탭 내용은 메뉴를 열 때 한 번 만들어져(`NativeConstruct`) 다른 탭에 갔다 와도 다시 만들어지지 않기 때문입니다. CommonUI가 켜짐 알림(부모 함수) 때 첫 포커스 대상을 정하므로, 기본 분류는 부모 함수를 부르기 전에 고릅니다. 뒤에 고르면 첫 포커스가 이전 분류 버튼에 남습니다(테스트 창에서 확인).

### 1.3 내용 칸 크기

- 엔진 위젯 전환기는 켜진 자식의 크기만 따르므로(`SWidgetSwitcher::ComputeDesiredSize`), `WBP_SettingsPanel`에서 전환기를 크기 상자(`USizeBox`)로 감싸고 두 언어에서 가장 큰 분류(Controls)에 맞춘 고정 크기를 줍니다. 크기는 위젯 블루프린트 값입니다.
- 대안은 패널이 켜질 때 세 분류의 크기를 재어(`ForceLayoutPrepass`) 상자에 넣는 코드입니다. 줄이 늘거나 언어가 바뀌어도 맞지만 코드가 늘어 고정 크기를 골랐습니다. 조작 안내 줄이 늘면 상자 크기도 함께 고칩니다.

### 1.4 클래스 변경

| 클래스 | 변경 |
| - | - |
| `UAgTabList` | `FAgTab::Content`를 `TSubclassOf<UUserWidget>`로 넓힘(분류 내용은 사용자 위젯). `TabButtonBox`를 `UPanelWidget`으로(가로·세로 상자 모두). `PreviousTabAction`·`NextTabAction`을 `BindWidgetOptional`로(분류 목록에는 키 아이콘이 없음). 기본 탭 고르기를 `SelectDefaultTab()`으로 공개해 `NativeConstruct`와 패널이 함께 씀. 분류 목록 인스턴스에는 탭 바꾸기 입력 액션을 두지 않음. |
| `UAgTabButton` | 선택 표시 위젯 `Underline`을 `SelectionMark`로 이름 바꿈. 분류에서는 세로 막대라 모양이 아닌 역할로 부름. |
| `UAgSettingsPanel` (새) | `CategoryList`(`UAgTabList`)를 `CategoryContent`에 연결, 켜질 때 기본 분류, 포커스 대상은 고른 분류 버튼, 분류가 바뀌면 →·← 내비게이션 명시. |
| `UAgSettingsScreen` (새) | 타이틀의 Settings 화면. 뒤로 처리(`bIsBackHandler`), Back 버튼은 화면 끄기, 포커스 대상은 패널의 포커스 대상. 위젯 블루프린트의 `InputMapping`은 `IMC_Default`(타이틀에는 게임 매핑이 없어, 걸지 않으면 Controls의 키 아이콘이 키를 찾지 못함. 지금 두 팝업과 같은 이유). 패널에는 걸지 않음: 일시정지 메뉴에서 탭이 꺼질 때 게임 매핑까지 빠짐. |
| `UAgDisplaySettings` (새) | 창 모드·해상도 드롭다운. 만들어질 때(`NativeConstruct`)와 바꾼 뒤 지금 값으로 다시 채움. 고를 수 있는 창 모드와 그 글자는 위젯 블루프린트 기본값 `WindowModeNames`, 해상도 후보는 `StandardResolutions`. |
| `UAgLanguageSettings` (새) | `UAgSettingsPopup`의 언어 드롭다운 코드를 옮김. 만들어질 때 채움. |
| `UAgMenuTitle` (새) | 메뉴 화면 제목. 인스턴스 값 `Text`를 디자이너에서도 보이게 `NativePreConstruct`에서 글자에 넣음(`UAgButton`의 `ButtonText`와 같은 방식). 위젯 블루프린트 그래프를 쓰지 않는 규칙 때문에 C++로 둠. |
| `UAgTitleScreen` | `ControlsButton`, `ControlsPopupClass`, `SettingsPopupClass`, `OpenControls` 삭제. `SettingsScreenClass`(`UAgSettingsScreen`)를 Menu 레이어로 엶. 돌아오면 지금의 `bAutoRestoreFocus`가 Settings 버튼을 다시 고름. |
| `UAgControlsPopup`, `UAgSettingsPopup` | 삭제. |
| `UAgUILayout`, `EAgUILayer` | 쓰는 화면이 없어진 `Modal`과 `ModalStack` 삭제. |

### 1.5 위젯 블루프린트와 문구

| 에셋 | 변경 |
| - | - |
| `WBP_TabButton` | `Underline` 위젯 이름을 `SelectionMark`로. |
| `WBP_CategoryButton` (새, `UAgTabButton`) | 분류 이름(버튼 스타일 `ButtonStyle_Tab`의 글자) + 이름 왼쪽 16px의 4px 세로 막대 `SelectionMark`. 기본값에서 포커스 받음, 포커스로 선택. |
| `WBP_CategoryList` (새, `UAgTabList`) | 세로 상자 `TabButtonBox`(이름 사이 24px), 키 아이콘 없음, `TabButtonClass` = `WBP_CategoryButton`. |
| `WBP_SettingsPanel` (새) | 화면 크기 오버레이 안에, 위쪽 172px·가운데에 가로로 `CategoryList`(`Tabs`: `Settings.Controls` → `WBP_ControlsSettings`, `Settings.Display` → `WBP_DisplaySettings`, `Settings.Language` → `WBP_LanguageSettings`), 48px, 고정 크기(720 × 600) 상자 안의 `CategoryContent`. 위치는 `WBP_InventoryTab`과 같은 방식(오버레이 칸의 위쪽 여백)입니다. |
| `WBP_ControlsSettings` (새) | 오버레이 안 왼쪽 위에 `WBP_ControlsGuide`. |
| `WBP_DisplaySettings`, `WBP_LanguageSettings` (새) | 이름(`TS_Large`)과 `WBP_Dropdown`의 2열 그리드(열 사이 48px, 줄 사이 16px). Desired Focus Widget은 첫 드롭다운. |
| `WBP_MenuTitle` (새, `UAgMenuTitle`) | 고른 탭 이름과 같은 글자 스타일, 그 아래 8px에 글자 너비만큼의 4px 흰 밑줄 이미지. |
| `WBP_SettingsScreen` (새) | `WBP_PauseMenu`와 같은 배치: 불투명한 검은 배경 이미지, 탭 바 자리에 `WBP_MenuTitle`('Settings'), 패널은 일시정지 메뉴의 탭 내용과 같은 높이, 그 아래 32px에 Back 버튼(`WBP_Button`). 패널 인스턴스 `bAutoActivate`. |
| `WBP_StageSelect` | 불투명한 검은 배경 이미지 추가, 제목 글자를 `WBP_MenuTitle`('Stage Select')로 바꿔 화면 여백에 두고, 드롭다운과 버튼 묶음의 위쪽을 제목 아래 72px로. 드롭다운과 버튼 사이 간격은 그대로. |
| `WBP_EnemySelect` | `Background`를 불투명하게, 제목 글자를 `WBP_MenuTitle`('Enemy Select')로 바꿔 화면 여백에 두고, 카드 줄과 버튼 묶음의 위쪽을 제목 아래 72px로. |
| `WBP_PauseMenu` | `TabList`의 `Tabs`에 `Menu.Settings` → `WBP_SettingsPanel` 추가. |
| `WBP_Title` | Controls 버튼 삭제, `SettingsScreenClass` 지정. |
| `WBP_UILayout` | `ModalStack` 삭제. |
| `WBP_ControlsPopup`, `WBP_SettingsPopup` | 삭제. |
| `Content/StringTables/StringTable.csv` | `Menu.Settings`, `Settings.Controls`, `Settings.Display`, `Settings.Language` 추가. 두 칼럼 모두 영어이며, 지금의 탭 이름(`Menu.Inventory`, `Menu.System`)과 같은 방식입니다. 그 밖의 영어 메뉴 글자(제목, Back, 줄 이름, 창 모드 이름)는 지금처럼 위젯 블루프린트 값입니다. |

## 2. GAS 및 데이터 설계

- 게임플레이 태그, 어빌리티, 효과, `DesignData` JSON은 바뀌지 않습니다.
- 창 모드와 해상도는 엔진이 `GameUserSettings.ini`에 저장합니다. 언어는 지금처럼 엔진 방식으로 같은 파일에 저장됩니다.

## 3. 인터페이스 & 함수 시그니처

```cpp
// AgTabList.h
USTRUCT()
struct FAgTab
{
	UPROPERTY(EditAnywhere, Category = "Tab")
	FText Label;

	UPROPERTY(EditAnywhere, Category = "Tab")
	TSubclassOf<UUserWidget> Content;
};

class UAgTabList : public UCommonTabListWidgetBase
{
public:
	void SelectDefaultTab();

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UAgActionWidget> PreviousTabAction;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> TabButtonBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UAgActionWidget> NextTabAction;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UUserWidget>> ContentWidgets;
};

// AgSettingsPanel.h
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgSettingsPanel : public UCommonActivatableWidget
{
protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgTabList> CategoryList;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonAnimatedSwitcher> CategoryContent;

private:
	UFUNCTION()
	void HandleCategorySelected(FName TabId);
};

// AgSettingsScreen.h
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgSettingsScreen : public UAgMenuScreen
{
public:
	UAgSettingsScreen(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeOnInitialized() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgSettingsPanel> SettingsPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> BackButton;
};

// AgMenuTitle.h
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgMenuTitle : public UCommonUserWidget
{
protected:
	virtual void NativePreConstruct() override;

	UPROPERTY(EditAnywhere, Category = "UI")
	FText Text;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> TitleText;
};

// AgDisplaySettings.h
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgDisplaySettings : public UCommonUserWidget
{
protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TMap<TEnumAsByte<EWindowMode::Type>, FText> WindowModeNames;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TArray<FIntPoint> StandardResolutions;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgDropdown> WindowModeDropdown;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgDropdown> ResolutionDropdown;

private:
	/** 지금 창 모드와 해상도로 두 드롭다운을 다시 채웁니다. */
	void Refresh();

	void ApplyWindowMode();
	void ApplyResolution();
};

// AgLanguageSettings.h
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgLanguageSettings : public UCommonUserWidget
{
protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgDropdown> LanguageDropdown;

private:
	void ApplyLanguage();
};
```

- `UAgDisplaySettings::ApplyWindowMode`: `SetFullscreenMode`. 지금 해상도가 새 창 모드의 목록에 없으면 목록의 가장 큰 해상도로 정하고 `ApplyResolutionSettings(false)`, `SaveSettings()`, `Refresh()`.
- `UAgDisplaySettings::Refresh`: 창 모드 목록은 `WindowModeNames`에 있는 모드를 엔진 순서로 채우고 지금 창 모드를 고름. 해상도 목록은 지금 창 모드의 목록을 넓이가 큰 것부터 '가로 × 세로'(문화권과 관계없는 글자)로 채우고 지금 해상도를 고름.

## 4. 문서 반영 (4단계)

- [캐릭터 기반 구조와 GAS](<../../Docs/Architecture/캐릭터 기반 구조와 GAS.md>) '게임 모드와 게임 스테이트': 클래스 표(팝업 두 개 삭제, 새 클래스 다섯 개, `UAgTabList`·`UAgTabButton`·`UAgMenuScreen`·`UAgDropdown`의 설명), 타이틀에서 `IMC_Default`를 거는 화면, 분류 버튼의 포커스 규칙과 내비게이션 명시, 일시정지 메뉴 클래스 다이어그램.
- [데이터 로딩과 스키마](<../../Docs/Architecture/데이터 로딩과 스키마.md>) 값 위치 표: '일시정지 메뉴 탭'(Settings 탭), '조작 안내'(`WBP_ControlsGuide`가 `WBP_SettingsPanel` 안), 새 줄 'Settings 분류'(분류와 순서, 내용 칸 크기, 창 모드 이름).
- 기획서와 그림은 Spec '2.11 기존 기획서와의 관계'.
