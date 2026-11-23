# 입력 키 아이콘 기술 설계 (Plan)

## 1. 영향 범위 및 아키텍처

CommonUI의 입력 아이콘 기능을 엔진 정석 구조로 씁니다(사용자 결정: "모두 다 정석대로"). 게임패드를 더할 때는 데이터와 설정만 더하고 코드는 고치지 않습니다.

| 부분 | 내용 |
| - | - |
| CommonUI 설정 | `Config/DefaultGame.ini` `[/Script/CommonInput.CommonInputSettings]`: `bEnableEnhancedInputSupport=True`, `InputData=/Game/Input/BP_UIInputData` |
| 플랫폼 설정 | `Config/Windows/WindowsGame.ini` `[CommonInputPlatformSettings_Windows CommonInputPlatformSettings]`: 컨트롤러 데이터 등록, `bSupportsGamepad=False`(게임 개요: 키보드·마우스만 지원. 켜 두고 패드 데이터가 없으면 패드를 건드릴 때 입력 종류가 게임패드로 바뀌어 아이콘이 사라짐). 에디터 Project Settings가 플랫폼 설정을 이 파일에 저장하므로 같은 곳에 둡니다 |
| 컨트롤러 데이터 | `UCommonInputBaseControllerData`를 상속한 데이터 전용 BP `/Game/UI/InputIcons/BP_KeyboardMouseControllerData`. `InputBrushDataMap`에 키 18개 → 텍스처(브러시 36 × 36px), `InputBrushKeySets`에 W·A·S·D 묶음 → `T_Key_WASD`(156 × 36px) |
| 텍스처 | Kenney Input Prompts 1.5 (CC0)의 Keyboard & Mouse 'Double'(128px). 글자 자리가 투명하게 뚫려 있어 밝은 배경에서 읽히지 않으므로 뚫린 부분을 어둡게 채우고 옅은 어두운 외곽선을 둘러 가져옵니다. 원본은 `Art/UI/InputIcons`(가공한 PNG + Kenney 라이선스), 에셋은 `/Game/UI/InputIcons/T_Key_<엔진 키 이름>`·`T_Key_WASD` |
| 위젯 | `UAgActionWidget : UCommonActionWidget`. WBP에 바로 놓고 씁니다(감싸는 WBP 없음, 사용자 결정). 크기는 컨트롤러 데이터의 브러시 크기이고, 크게 보여야 하는 곳(상호작용 안내 48px)은 엔진 `ScaleBox`로 키웁니다 |
| Esc | CommonUI UI 액션 태그(`UI.Action.Escape`)에서 Enhanced Input의 엔진 뒤로 액션으로 옮깁니다. `IA_Back`(Esc)을 `IMC_UI`에 두고 `BP_UIInputData`의 `EnhancedInputBackAction`으로 지정합니다. 엔진 아이콘 위젯은 UI 액션 태그를 그리지 못해, 조작 안내의 일시정지 줄을 그리려면 Enhanced Input 액션이어야 합니다 |
| UI 입력 매핑 | CommonUI는 Enhanced Input UI 액션의 키를 로컬 플레이어에 걸린 매핑에서 찾으므로, `AAgPlayerController`가 `IMC_UI`를 늘 걸어 둡니다(`BP_PlayerController` 기본값 `UIInputMapping`). 화면마다 걸면 한 화면이 닫힐 때 다른 화면의 매핑까지 빠집니다 |
| 메뉴의 Esc 닫기 | 조작 안내 팝업·일시정지 메뉴·스테이지 선택은 엔진 `bIsBackHandler`(기본 처리 = 화면 닫기). 프로젝트의 `BindEscapeToClose`는 지웁니다 |
| HUD의 Esc | 엔진 뒤로 처리는 메뉴 입력 모드에서만 받아, 게임 입력 모드의 HUD는 뒤로 액션을 직접 바인딩해 일시정지 메뉴를 엽니다 |
| 타이틀의 키 | 조작 안내 팝업 `WBP_ControlsPopup`의 엔진 속성 `InputMapping`에 `IMC_Default`. 팝업이 활성인 동안만 그 매핑이 걸려 타이틀에서도 키를 찾습니다. 타이틀에는 그 액션을 바인딩한 폰이 없어 게임 입력은 생기지 않습니다 |

엔진에서 벗어난 곳은 `UAgActionWidget::GetIcon` 하나입니다. 엔진은 Enhanced Input 액션의 첫 키 아이콘만 찾아, 키가 여럿인 액션(이동)은 W만 보입니다. 액션에 매핑된 현재 입력 종류의 키를 모두 `UCommonInputPlatformSettings::TryGetInputBrush(키 배열)`에 넘깁니다. 키가 하나면 키 아이콘, 여럿이면 키 묶음 아이콘을 찾는 엔진 함수이고, 엔진의 데이터 테이블 액션이 여러 키를 그리는 방식과 같습니다. 게임패드에서 이동은 왼쪽 스틱 키 하나라 그 키의 아이콘이 나옵니다.

`Docs/Architecture` 규칙: 기본 방침 '구조 선택'(엔진 기능과 CommonUI 하위 클래스 우선), '위젯 갱신'(아이콘은 엔진의 입력 방식 변경·매핑 재구성 이벤트로만 갱신, Tick 없음). `UCommonActionWidget`은 `UUserWidget`이 아니라 `DisableNativeTick` 대상이 아닙니다.

## 2. GAS 및 데이터 설계

- 태그: `UI.Action.Escape`를 지웁니다(`AgGameplayTags`, `Config/DefaultInput.ini`의 `CommonUIInputSettings` 항목).
- JSON 변경 없음.
- 문구: 조작 안내의 한 줄짜리 두 항목 `Controls.Keys`·`Controls.Actions`를 지우고, 줄마다 조작 이름 12개를 `Content/StringTables/StringTable.csv`에 둡니다(`Controls.Move` … `Controls.Pause`).
- 쓰지 않게 된 `TS_ControlsKey`(키 글자 스타일)는 지웁니다.
- 모듈 의존성: `UCommonInputPlatformSettings::Get()`이 쓰는 `UPlatformSettingsManager`가 `DeveloperSettings` 모듈에 있어 비공개 의존성에 더합니다.

## 3. 인터페이스 & 함수 시그니처

```cpp
UCLASS()
class UAgActionWidget : public UCommonActionWidget
{
public:
	virtual FSlateBrush GetIcon() const override;
};

// AAgPlayerController
UPROPERTY(EditDefaultsOnly, Category = "UI")
TObjectPtr<UInputMappingContext> UIInputMapping;   // ReceivedPlayer에서 AddMappingContext

// UAgControlsPopup, UAgPauseMenu, UAgStageSelectScreen
XXX(const FObjectInitializer& ObjectInitializer);   // bIsBackHandler = true
```

## 4. 바꾸는 에셋

| 에셋 | 변경 |
| - | - |
| `WBP_HUD` | `Skill1Key`~`Skill4Key`·`PotionKey`(원 + 글자)를 `UAgActionWidget`(IA_Skill1~4, IA_Potion)로 교체, 칸 36px |
| `WBP_InteractionPrompt` | 원 + 글자를 `KeySize`(48px) > `KeyScale`(ScaleBox) > `UAgActionWidget`(IA_Interact)로 교체 |
| `WBP_ControlsGuide` | 두 줄글을 그리드 `Rows`(12줄 × 키 아이콘 열·조작 이름 열)로 다시 구성. 스킬 줄은 액션 위젯 넷을 4px 간격으로 |
| `WBP_ControlsPopup` | `InputMapping` = `IMC_Default` |
| `BP_PlayerController` | `UIInputMapping` = `IMC_UI` |
| 새 에셋 | `IA_Back`, `IMC_UI`, `BP_UIInputData`(`/Game/Input`), `BP_KeyboardMouseControllerData`, `T_Key_*` 19장(`/Game/UI/InputIcons`) |

## 5. 정본 반영 위치 (4단계)

- `Docs/Design/System/UI 시스템.md`: '입력 키 표시', 스킬 슬롯 키 표시, 회복병 키, 상호작용 안내 키 표시·위치, 조작 안내 팝업.
- `Docs/Art/아트 표현 사양.md`: '입력 키 아이콘' 행(Kenney Input Prompts, 뚫린 부분 채움과 외곽선).
- `Docs/Architecture/데이터 로딩과 스키마.md`: '조작 안내' 행(그리드, `TS_ControlsKey` 삭제), '입력 키 매핑' 행(Esc는 `IMC_UI`의 `IA_Back`), 키 아이콘 위치(컨트롤러 데이터, 플랫폼 설정, CommonUI 입력 데이터).
- `Docs/Architecture/캐릭터 기반 구조와 GAS.md`: UI 클래스 표에 `UAgActionWidget`, `UAgMenuScreen` 설명의 Esc 처리.
- `Docs/Art/에셋 조사 개요.md` 등 리소스 목록에 Kenney Input Prompts가 들어갈 자리가 있으면 추가.
