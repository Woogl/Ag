# 영어 지원 기술 설계 (Plan)

## 1. 영향 범위 및 아키텍처

### 1.1 번역: 문구 표의 언어 칼럼

- 문구 표 `Content/StringTables/StringTable.csv` 하나에 모든 언어를 둡니다. 칼럼은 `Key`, `SourceString`(원문, 한국어), `en`(영어)입니다. 언어를 더할 때는 그 언어 코드 이름의 칼럼을 더합니다.
- `Key`, `SourceString`은 엔진 스트링 테이블이 정한 칼럼 이름이라 바꾸지 않습니다. 엔진은 나머지 칼럼을 항목의 메타데이터로 읽으므로(`StringTableCore.cpp`의 `ImportStringsFromCSVString`) 문구 표 등록은 지금과 같습니다.
- 게임 인스턴스가 시작할 때 `UAgLanguageSubsystem`이 등록된 문구 표의 항목마다 이름이 언어인 메타데이터 칼럼을 엔진의 Polyglot 텍스트 데이터(`FPolyglotTextData`, `FTextLocalizationManager::RegisterPolyglotTextData`)로 등록합니다. 엔진이 이 데이터를 번역으로 써서, `LOCTABLE`로 문구 표를 가리키는 글자(JSON, 위젯)가 언어에 따라 바뀝니다.
- 엔진 표준 번역 파일(gather 커맨드렛, PO, `.locres`)은 쓰지 않습니다. 설정 파일·생성 파일·실행 단계가 늘고, 외부 번역가용 PO와 위젯 글자 자동 수집은 이 프로젝트(현지화 문구는 모두 문구 표)에 필요 없습니다.
- 에디터에서 CSV를 고치면 엔진이 문구 표를 다시 읽고, 번역은 다음 PIE 시작(게임 인스턴스 생성) 때 다시 등록됩니다.

### 1.2 시작 언어와 언어 바꾸기: `UAgLanguageSubsystem`

`UGameInstanceSubsystem`. 게임 전체의 언어를 맡습니다. 언어 저장은 엔진 방식(`GameUserSettings.ini`의 `[Internationalization] Language`)을 그대로 씁니다.

- **시작 언어** (`Initialize`): 엔진은 번역을 등록하기 전에 시작 언어를 고르므로(번역이 없어 영어로 시작) 등록한 뒤 게임(에디터가 아닌 실행)에서 다시 고릅니다. 원하는 언어는 엔진과 같은 순서(명령줄 `LANGUAGE=`·`CULTURE=`, 게임·엔진 설정의 `Language`·`Culture`, 윈도우 언어)로 찾고, 그 언어나 상위 언어(ko-KR → ko)의 번역이 있으면 그 언어, 없으면 영어로 시작합니다. 이 결정은 저장하지 않아, 고른 적이 없으면 윈도우 언어를 바꿀 때 따라갑니다.
- **언어 목록** (`GetLanguages`): 엔진이 아는 게임 번역 언어(`GetLocalizedCultureNames(Game)`, 등록한 Polyglot 데이터의 원문·번역 언어)를 원문 언어 먼저, 나머지는 이름순으로 돌려줍니다. 표시 이름은 그 언어로 적은 엔진 문화권 이름(`FCulture::GetNativeLanguage()`: 한국어, English)입니다.
- **지금 언어** (`GetLanguage`), **언어 바꾸기** (`SetLanguage`):
  - 게임: 엔진 `UKismetInternationalizationLibrary::SetCurrentLanguage(언어, true)`로 바꾸고 저장합니다.
  - 에디터(PIE): 엔진 언어를 바꾸면 에디터 UI 언어까지 바뀌므로, 엔진의 게임 번역 미리 보기(`EnableGameLocalizationPreview`)로 게임 문구만 바꾸고 저장하지 않습니다. 서브시스템이 끝날 때(PIE 종료) 미리 보기를 되돌립니다.

### 1.3 Settings 팝업: `UAgSettingsPopup`, `WBP_SettingsPopup`

- `UAgSettingsPopup : UAgMenuScreen`. `UAgControlsPopup`과 같은 구성(`bIsBackHandler`, Confirm 버튼으로 닫기)에 언어 드롭다운(`UAgDropdown`, `LanguageDropdown`)을 더합니다.
  - 열 때마다(`NativeOnActivated`) 서브시스템의 언어 목록으로 드롭다운을 채우고 지금 언어를 고릅니다. 스테이지 선택 화면과 같은 방식입니다.
  - 드롭다운의 `OnSelectionChanged`에 서브시스템 `SetLanguage`를 겁니다. 처음 선택은 드롭다운입니다.
  - 뒤의 타이틀 버튼으로 포커스가 새지 않게, 드롭다운 아래는 Confirm, Confirm 위는 드롭다운으로 잇고 나머지 방향은 멈춥니다.
- `WBP_SettingsPopup`: `WBP_ControlsPopup`을 복제해 부모를 바꾸고, 조작 안내 자리에 'Language' 글자와 `WBP_Dropdown`(360 × 72)을 가로로 놓습니다(사이 48px).
- 타이틀: `UAgTitleScreen`에 `SettingsButton`과 `SettingsPopupClass`를 더하고, `WBP_Title`의 Controls와 Quit 사이에 Settings 버튼을 놓습니다. 팝업은 Modal 레이어로 엽니다.

### 1.4 패키징

`DefaultGame.ini`의 `[/Script/UnrealEd.ProjectPackagingSettings]`:

- `InternationalizationPreset`을 엔진 기본 `English`에서 `EFIGSCJK`로 바꿉니다. `English`에는 한국어 문화권 데이터가 없어 패키지에서 한국어로 바꾸거나 한국어 이름을 표시할 수 없습니다. 영어와 한국어를 함께 담는 가장 작은 묶음입니다.
- `CulturesToStage`를 엔진 기본(en)에서 `ko`, `en`으로 바꿔, 엔진 자체 문구도 두 언어를 넣습니다.

## 2. GAS 및 데이터 설계

- 태그, 어빌리티, 효과, JSON 스키마 변경은 없습니다.

## 3. 인터페이스 & 함수 시그니처

```cpp
UCLASS()
class UAgLanguageSubsystem : public UGameInstanceSubsystem
{
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** 고를 수 있는 언어(문화권 이름)입니다. 원문 언어가 먼저입니다. */
	static TArray<FString> GetLanguages();

	FString GetLanguage() const;
	void SetLanguage(const FString& Language);
};

UCLASS(Abstract, meta = (DisableNativeTick))
class UAgSettingsPopup : public UAgMenuScreen
{
protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgDropdown> LanguageDropdown;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgButton> ConfirmButton;
};

// UAgTitleScreen에 추가
UPROPERTY(EditDefaultsOnly, Category = "UI")
TSubclassOf<UAgSettingsPopup> SettingsPopupClass;

UPROPERTY(meta = (BindWidget))
TObjectPtr<UAgButton> SettingsButton;
```

## 4. 확인 방법과 한계

- 언어 바꾸기, 영어 문구, 저장, 번역이 없는 언어: 테스트 창(게임 실행)에서 확인합니다. 윈도우 언어가 한국어가 아닌 경우는 설정 파일에 번역이 없는 언어(fr)를 적어 같은 코드 경로로 확인하고, 실제 영어 윈도우 확인은 따로 합니다. 테스트가 쓴 `GameUserSettings.ini`의 언어 값은 끝나면 지웁니다.
- 타이틀 화면과 Settings 팝업 조작은 테스트 창 도구가 다루지 못해 PIE에서 확인합니다(띄우기 전에 묻습니다).
- 패키지: 패키지 게임이 문구 표를 읽고 두 언어로 바꾸는지 확인합니다.
