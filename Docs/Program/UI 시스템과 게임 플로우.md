# UI 시스템과 게임 플로우

HUD, 화면 UI, 게임 플로우를 어떻게 구현할지 정합니다. 카메라와 화면 후처리는 [카메라와 화면 효과](<카메라와 화면 효과.md>)를 따릅니다.

## 기술 결정

| 항목 | 결정 | 이유 |
| - | - | - |
| HUD | `WBP_HUD`의 배치는 디자이너에서, 동작은 C++ 부모 클래스(BindWidget)에서 처리합니다. 감소 잔상은 HP 바, 플레이어 SP 바, 보스 PP 바 뒤의 잔상 바이고, 대기·줄어드는 시간은 `WBP_HUD` 기본값입니다([데이터 위치표](<데이터 위치표.md>)). SP 잔상은 HUD가 플레이어 ASC의 SP 변화 델리게이트를 듣다가 대미지 GE가 줄인 SP(가드·가드 브레이크)에만 세우고, 다른 감소는 잔상이 바를 바로 따라갑니다. 잔상은 게임 시간으로 흘러 슬로우모션에 맞춰 느려지고, 게임이 일시정지되면(`UWorld::IsPaused`) 흐른 시간을 0으로 세어 멈춥니다. 조작 안내는 공용 위젯 `WBP_ControlsGuide`의 글자에 키와 조작 이름을 그대로 적고, `WBP_HUD`와 조작 안내 팝업(`WBP_ControlsPopup`)이 이 위젯을 함께 씁니다. 플레이어나 보스의 사망 연출에는 상태 요소(플레이어 상태, 슬롯, 보스 상태)와 조작 안내를 담은 패널을 숨기고, 궁극기 컷신 동안(플레이어 컨트롤러의 시네마틱 모드)에도 숨겼다가 끝나면 되돌립니다. | 모양은 에셋이 정하고 코드는 값만 맡습니다. SP는 회피·달리기로도 줄어 바 값만으로는 가드 피격을 가릴 수 없고, 델리게이트가 넘기는 GE 정보로 가립니다. 엔진은 일시정지 중에도 `GetDeltaSeconds`에 프레임 시간을 넣습니다(`UWorld::Tick`). 조작 안내는 바뀌지 않는 문구라 화면 UI의 다른 문구처럼 위젯에 둡니다. 두 화면이 한 위젯을 써서, 키 배치를 바꾸면 `IMC_Default`(Esc는 아래 'Esc 입력'의 `DefaultInput.ini`)와 이 위젯의 문구만 고칩니다. |
| 슬롯 흐림, 쿨다운 표시 | 슬롯 흐림은 행동 태그(`Ability.Action.Skill`, `Ability.Action.Ultimate`)로 찾은 스펙의 독립 인스턴스에서 공통 비용 확인을 불러 정하고, 스킬 슬롯의 비용 숫자도 어빌리티의 비용(`GetCost`)을 씁니다. 스펙은 입력 계층과 같은 함수(`UAgAbilitySystemComponent::FindSpecByTag`)로 찾습니다. 쿨다운 표시는 해당 스펙 인스턴스가 출처인 쿨다운 GameplayEffect에서 남은 시간과 전체 시간을 읽어, 숫자는 위젯에 쓰고 원형 표시 머티리얼에는 남은 비율만 넘깁니다. | 사용 조건과 쿨다운을 어빌리티 한 곳에만 둡니다. 덮는 모양은 머티리얼이 정합니다. |
| 그로기·처형 중 PP 바 | `State.Groggy`를 주는 효과의 지난 시간 ÷ 지속 시간만큼 채웁니다. 공중에서도 같은 효과의 시간을 쓰고, 감소 잔상도 이 채운 값을 따라갑니다. 보스에게 `State.Execution`이 있으면 PP 바와 감소 잔상 갱신을 건너뛰어 마지막 표시값을 유지합니다. 처형 상태가 끝나면 실제 PP 표시로 돌아갑니다. 처형 없이 그로기가 끝날 때도 실제 PP를 표시합니다. | 처형 전환으로 그로기 효과가 제거되어도 바가 0으로 떨어지지 않도록 기존 표시를 유지합니다. 실제 PP 회복은 [보스 AI와 패턴](<보스 AI와 패턴.md>) '그로기와 처형 피격'을 따릅니다. |
| 월드 위치 HUD | 락온 마커, 처형 안내, 대미지 숫자는 화면 공간 위젯 컴포넌트로 표시합니다. 락온 마커와 처형 안내는 보스의 락온 지점과 머리 본에 붙이고(락온 마커와 카메라가 쓰는 `LockOnBone`, 처형 안내를 붙이는 `ExecutionPromptBone`은 `AgBossCharacter.cpp`의 상수로만 둡니다. 락온 본이 없으면 보스의 액터 위치를 씁니다), 보스가 매 프레임 표시 여부를 정하며, 플레이어 컨트롤러가 시네마틱 모드(궁극기 컷신)이면 숨깁니다. 처형 안내는 보스가 그로기 중일 때 플레이어 처형 행동의 Targeting을 읽기 전용으로 평가해 이 보스가 그 대상이면 표시합니다. 대미지 숫자는 대미지 GameplayEffect가 실행하는 큐(`GC_DamageFloater`)가 플레이어가 준 대미지일 때 맞은 위치(처형 타격은 대상의 캡슐 중심)에 액터를 생성하고, 액터가 떠오르는 동안 투명해지는 것은 위젯 애니메이션이 맡습니다. 표시 시간(애니메이션 길이)과 떠오르는 높이는 위젯에 둡니다. 대미지 숫자는 실제 시간으로 흘러 슬로우모션의 영향을 받지 않습니다. 게임이 일시정지되면 액터가 틱하지 않아 떠오르기가 멈추고, 위젯이 `FadeAnimation`의 재생 속도를 0으로 둬(`SetPlaybackSpeed`) 페이드도 함께 멈춥니다. | 대미지 숫자는 HUD가 숨은 뒤에도 끝까지 재생해야 해 HUD와 따로 둡니다. 위젯 애니메이션은 실제 시간으로 재생되므로, 떠오르는 것도 실제 시간으로 세면 둘을 맞추려고 게임 속도를 넘길 필요가 없습니다. 위젯 애니메이션은 슬레이트가 틱해 일시정지와 상관없이 재생됩니다. 엔진이 화면 공간 위젯 컴포넌트를 플레이어 화면의 −100 순서에 그려(`WidgetComponent.cpp`의 `LayerZOrder`), 월드 위치 HUD는 UI 레이어에 넣지 않아도 모든 레이어 아래에 표시됩니다. |
| UP 게이지 | 텍스처 없는 기능용 UI 머티리얼로 슬롯 테두리를 채웁니다. 채우는 모양은 머티리얼이 정하고, 코드는 UP 비율만 머티리얼 파라미터로 넘깁니다. 채우는 수식은 머티리얼의 Custom 노드(HLSL) 하나에 넣어 그래프를 작게 둡니다. | 코드가 아트를 직접 그리지 않고 모양은 에셋이 맡습니다. 수식이 텍스트라 AI도 고칠 수 있습니다. |
| UI 레이어 | 레이아웃 `WBP_UILayout`(부모 `UAgUILayout`, `UCommonUserWidget`)에 레이어마다 `UCommonActivatableWidgetStack`을 하나씩, 아래부터 `GameStack`, `MenuStack`, `ModalStack` 순서로 Overlay에 겹치고 BindWidget으로 찾습니다. 스택의 전환 시간(`TransitionDuration`)은 0입니다. 레이어는 C++ 열거형 `EAgUILayer`로 구분하고 `PushScreen`을 부르는 쪽이 고릅니다(HUD와 결과 UI는 플레이어 컨트롤러의 `ShowHUD`·`ShowResult`). 플레이어 컨트롤러가 `ReceivedPlayer`에서 레이아웃을 만들어 `AddToPlayerScreen`으로 띄우고(레이아웃 클래스는 `BP_PlayerController`의 기본값), `PushScreen`이 해당 스택의 `AddWidget`을 부릅니다. 화면은 스스로 `DeactivateWidget`을 불러 닫고, 스택이 비활성화된 화면을 내립니다. 다른 화면에 가려질 수 있는 화면(지금은 타이틀)은 `bAutoRestoreFocus`를 켜고, 복원할 대상이 없으면 `NativeGetDesiredFocusTarget`으로 갑니다(`CommonUI.FallbackToDesiredOnAutoRestoreFailure=1`). | 스택이 '마지막에 연 화면만 표시, 닫으면 그 전 화면 표시'를 그대로 해 줍니다. 레이아웃이 활성화 위젯이 아니어서 스택에 올린 화면은 각각 CommonUI의 독립 루트가 되고, CommonUI는 활성 루트 가운데 가장 위에 그려진 것(Overlay 순서)에 입력 설정과 포커스를 주며, 입력을 잃는 루트는 그때 포커스된 위젯을 복원 대상으로 기억합니다. 빈 스택은 스스로 Collapsed가 되어 아래 레이어의 클릭을 막지 않습니다. 기획에 화면 전환 연출이 없습니다. Lyra의 `UI.Layer.*` 태그는 Game Feature 데이터가 레이어를 고르기 위한 것인데, 우리는 C++만 화면을 엽니다. 게임 모드는 `PostLogin` 안(`HandleStartingNewPlayer`)에서 첫 화면을 여는데, 엔진은 그 직전 `SetPlayer`에서 `ReceivedPlayer`를 부릅니다. 레벨이 바뀌면 엔진이 이전 월드의 뷰포트 위젯을 모두 내려(`UGameViewportSubsystem::HandleRemoveWorld`) 레이아웃과 화면이 함께 사라집니다. Lyra의 `GameUIManagerSubsystem`·`GameUIPolicy`는 로컬 플레이어 여럿(화면 분할)을 위한 것이라 두지 않습니다. |
| 화면 UI | 타이틀, 조작 안내 팝업, 일시정지 메뉴, 결과 UI는 CommonUI 화면으로 만듭니다. 화면마다 입력 모드(화면 UI는 UI 전용 입력과 커서 표시, HUD는 게임 입력과 커서 숨김)와 첫 선택 버튼을 정합니다. 화면의 버튼은 모두 공용 버튼 `WBP_Button`을 쓰고, 상태별 모양과 효과음은 표준 버튼 스타일 `ButtonStyle_Default`가 정합니다. 실행 키(좌클릭, Enter, Space)는 Slate 버튼의 기본 처리로 동작하므로 CommonUI 입력 데이터(확인·뒤로 액션)는 만들지 않고, CommonUI 설정 `CommonButtonAcceptKeyHandling`을 `TriggerClick`으로 둡니다. 결과 UI가 화면을 어둡게 하며 나타나는 페이드 인은 위젯 애니메이션(`FadeInAnimation`)이 맡고, 코드는 그것을 재생하고 끝나면 버튼을 켭니다. | 새 화면의 버튼도 같은 모양이 되도록 모양을 한 에셋에서 관리합니다. 페이드의 길이와 모양은 대미지 숫자처럼 위젯에서 바꿉니다. 팝업과 일시정지 메뉴를 닫는 Esc는 아래 'Esc 입력'의 UI 액션으로 받아 CommonUI 입력 데이터가 필요하지 않습니다. 이 설정의 기본값(`Ignore`)은 Enter와 Space를 버튼에 넘기지 않습니다. |
| Esc 입력 | Esc는 CommonUI UI 액션 하나(`UI.Action.Escape`, `AgGameplayTags`의 네이티브 태그)로 받고, 키 매핑은 CommonUI 입력 설정(`DefaultInput.ini`의 `CommonUIInputSettings`)에 둡니다. 전투 HUD, 조작 안내 팝업, 일시정지 메뉴가 각자 `RegisterUIActionBinding`으로 받습니다. HUD는 게임 입력 모드에서 받도록 바인딩의 `InputMode`를 Game으로 둡니다(기본값 Menu). 타이틀과 결과 UI는 바인딩하지 않습니다. PIE에서는 Esc가 PIE를 끝내므로, Esc 동작은 'Standalone Game'으로 실행하거나 에디터 단축키(Play World의 Stop)를 바꿔서 확인합니다. | CommonUI는 UI 액션을 활성 루트(가장 위 화면)의 바인딩에만 넘겨(`UCommonUIActionRouterBase::ProcessInput`) 'Esc는 입력을 받는 화면 하나에만 들어간다'는 기획 규칙이 그대로 됩니다. Lyra도 HUD 레이아웃이 `UI.Action.Escape`를 받아 Menu 레이어에 Esc 메뉴를 엽니다. Enhanced Input으로 받으려면 CommonUI의 Enhanced Input 지원을 전역으로 켜야 하고, 타이틀에는 `IMC_Default`가 걸려 있지 않아 팝업에서 키를 찾지 못합니다. CommonUI는 PIE에서 에디터의 PIE 종료 단축키와 같은 키를 일부러 넘기고, 단축키는 에디터 사용자 설정이라 프로젝트가 바꾸지 않습니다. |
| 일시정지 메뉴 | `WBP_PauseMenu`(부모 `UAgPauseMenu`)는 전투 HUD가 Esc로 Menu 레이어에 엽니다(클래스는 `WBP_HUD`의 `PauseMenuClass`). 메뉴가 활성화될 때 플레이어 컨트롤러의 `SetPause(true)`, 비활성화될 때 `SetPause(false)`를 부릅니다. Resume과 Esc는 메뉴를 비활성화하고, 나머지 버튼은 결과 UI처럼 보스전 게임 모드의 함수를 부릅니다. | 엔진 일시정지는 월드 틱, 타이머(스테이지 결과 연출 뒤 결과 UI, 슬로우모션 종료), 레벨 시퀀스, 나이아가라, 카메라 갱신(카메라 셰이크 포함), 게임 효과음을 함께 멈춥니다. 메뉴의 활성화·비활성화 쌍에 묶어 멈춤과 재개가 어긋나지 않습니다. 화면 클래스는 그 화면을 여는 객체가 갖습니다. |
| 조작 안내 팝업 | `WBP_ControlsPopup`(부모 `UAgControlsPopup`)에 `WBP_ControlsGuide`와 `ConfirmButton`을 두고, 타이틀의 `ControlsButton`이 Modal 레이어에 엽니다(팝업 클래스는 `WBP_Title`의 `ControlsPopupClass`). Confirm과 Esc('Esc 입력')는 팝업을 비활성화하며, Confirm의 방향 탐색은 모두 Stop입니다. 팝업의 화면 전체 배경이 마우스 입력을 가로막습니다. 타이틀 버튼은 위·아래 연결만 명시합니다. | 팝업이 따로 활성 루트가 되어 포커스와 CommonUI 입력을 가져가므로, 타이틀 버튼을 끄지 않아도 키보드로 뒤의 버튼이 실행되지 않습니다. 배경은 클릭이 뒤의 버튼에 닿는 것을 막습니다. |
| 결과 처리 | 보스전 게임 모드가 플레이어와 보스의 사망 가운데 먼저 처리된 것으로 결과(보스는 Stage Clear, 플레이어는 Game Over)를 정하고, 스테이지 결과 연출 시간이 지나면 결과 UI를 띄웁니다([사망 처리](<사망 처리.md>) '사망에서 결과 UI까지'). 그 밖의 캐릭터가 사망해도 결과와 HUD는 바뀌지 않습니다. Restart는 레벨을 다시 로드합니다. | 게임 플로우의 Stage Clear와 Game Over는 보스와 플레이어의 사망으로만 정해집니다. |

## 클래스 구조

### 게임 모드

레벨마다 하나씩 있습니다. 타이틀로 돌아가기와 게임 종료는 부모 `AAgGameMode`가, 보스전 열기는 타이틀용이, 다시 시작은 보스전용이 맡습니다.

| 클래스 | 부모 | 역할 |
| - | - | - |
| `AAgGameMode` | `AGameModeBase` | 타이틀로 돌아가기와 게임 종료. 타이틀용·보스전용 게임 모드의 부모 |
| `AAgFrontEndGameMode` | `AAgGameMode` | 타이틀 레벨의 흐름, 보스전 열기 |
| `AAgBossStageGameMode` | `AAgGameMode` | 보스전 레벨의 흐름, 결과 판정, 다시 시작(레벨 다시 열기) |

```mermaid
classDiagram
    direction TB
    class AAgFrontEndGameMode {
        <<BP_FrontEndGameMode>>
        UAgTitleScreen TitleScreenClass
        UWorld BossStageMap
    }
    class AAgBossStageGameMode {
        <<BP_BossStageGameMode>>
        UAgHUDWidget HUDWidgetClass
        UAgResultScreen ResultScreenClass
    }
    AGameModeBase <|-- AAgGameMode
    AAgGameMode <|-- AAgFrontEndGameMode
    AAgGameMode <|-- AAgBossStageGameMode

    AAgGameMode --> AAgPlayerController : PlayerControllerClass
    AAgGameMode ..> UGameMapsSettings : GameDefaultMap
    AAgFrontEndGameMode ..> AAgPlayerController : PushScreen
    AAgBossStageGameMode ..> AAgPlayerController : ShowHUD, HideHUD<br>ShowResult
    AAgBossStageGameMode ..> AAgCharacterBase : RegisterCombatant<br>OnDied
    UAgTitleScreen ..> AAgFrontEndGameMode : OpenBossStage<br>QuitGame
    UAgResultScreen ..> AAgBossStageGameMode : RestartStage<br>OpenFrontEnd, QuitGame
    UAgPauseMenu ..> AAgBossStageGameMode : RestartStage<br>OpenFrontEnd, QuitGame
```

### 플레이어 컨트롤러

UI 레이아웃을 만들어 화면을 레이어에 열고, 테스트용 콘솔 명령을 붙입니다.

| 클래스 | 부모 | 역할 |
| - | - | - |
| `AAgPlayerController` | `APlayerController` | UI 레이아웃 생성, 레이어를 받아 화면 열기, 테스트용 콘솔 명령 연결 |
| `UAgUILayout` | `UCommonUserWidget` | `WBP_UILayout`의 부모. 레이어별 스택을 갖고, 레이어를 받아 화면을 엽니다. |
| `UAgCheatManager` | `UCheatManager` | 테스트용 콘솔 명령 (개발 빌드) |

```mermaid
classDiagram
    direction TB
    class AAgPlayerController {
        <<BP_PlayerController>>
        UAgUILayout UILayoutClass
    }
    class UAgUILayout {
        <<WBP_UILayout>>
    }
    APlayerController <|-- AAgPlayerController
    AAgPlayerController --> UAgUILayout : UILayout
    UCommonUserWidget <|-- UAgUILayout
    UAgUILayout *-- UCommonActivatableWidgetStack : GameStack<br>MenuStack<br>ModalStack
    AAgPlayerController --> UAgHUDWidget : HUDWidget
    AAgPlayerController ..> UAgResultScreen : SetResult
    UAgTitleScreen ..> AAgPlayerController : PushScreen(Modal)
    UAgHUDWidget ..> AAgPlayerController : PushScreen(Menu)
    UAgPauseMenu ..> AAgPlayerController : SetPause
    AAgPlayerController --> UAgCheatManager : CheatClass
    UCheatManager <|-- UAgCheatManager
    UAgCheatManager ..> AAgCharacterBase : Kill
```

### 화면 UI와 HUD

| 클래스 | 부모 | 역할 |
| - | - | - |
| `UAgHUDWidget` | `UCommonActivatableWidget` | `WBP_HUD`의 부모. 플레이어 상태, 슬롯, 보스 상태, 조작 안내. 보스전의 게임 입력 모드를 정하고, Esc로 일시정지 메뉴를 엽니다. |
| `UAgTitleScreen` | `UCommonActivatableWidget` | 타이틀 화면, 조작 안내 팝업 열기 |
| `UAgControlsPopup` | `UCommonActivatableWidget` | 조작 안내 팝업, Confirm·Esc로 닫기 |
| `UAgPauseMenu` | `UCommonActivatableWidget` | 일시정지 메뉴, 활성화 동안 게임 일시정지, Resume·Esc로 닫기 |
| 조작 안내 위젯 | `UCommonUserWidget` | 배치만 있어 C++ 부모 없이 바로 상속합니다(`WBP_ControlsGuide`). |
| `UAgResultScreen` | `UCommonActivatableWidget` | Stage Clear UI, Game Over UI |
| `UAgButton` | `UCommonButtonBase` | 화면 UI 버튼 |

```mermaid
classDiagram
    direction LR
    class UAgHUDWidget {
        <<WBP_HUD>>
        UAgPauseMenu PauseMenuClass
    }
    class UAgTitleScreen {
        <<WBP_Title>>
        UAgControlsPopup ControlsPopupClass
    }
    class UAgControlsPopup {
        <<WBP_ControlsPopup>>
    }
    class UAgPauseMenu {
        <<WBP_PauseMenu>>
    }
    class UAgResultScreen {
        <<WBP_Result>>
    }
    class UAgButton {
        <<WBP_Button>>
    }
    UCommonActivatableWidget <|-- UAgHUDWidget
    UCommonActivatableWidget <|-- UAgTitleScreen
    UCommonActivatableWidget <|-- UAgControlsPopup
    UCommonActivatableWidget <|-- UAgPauseMenu
    UCommonActivatableWidget <|-- UAgResultScreen
    UCommonButtonBase <|-- UAgButton

    UAgTitleScreen *-- UAgButton : StartButton<br>ControlsButton<br>QuitButton
    UAgControlsPopup *-- UAgButton : ConfirmButton
    UAgPauseMenu *-- UAgButton : ResumeButton<br>RestartButton<br>ReturnToTitleButton<br>QuitButton
    UCommonUserWidget <|-- WBP_ControlsGuide
    UAgControlsPopup *-- WBP_ControlsGuide : ControlsGuide
    UAgHUDWidget *-- WBP_ControlsGuide : ControlsGuide
    UAgResultScreen *-- UAgButton : RestartButton<br>ReturnToTitleButton<br>QuitButton
    UAgHUDWidget ..> AAgPlayerCharacter : 스탯, SP 변화, 쿨다운
    UAgHUDWidget ..> AAgBossCharacter : 스탯, 그로기 시간 효과
```

### 월드 위치 HUD

보스에 붙어 월드 위치에 표시하는 위젯과, 대미지 숫자 큐가 맞은 위치에 띄우는 대미지 숫자입니다.

| 클래스 | 부모 | 역할 |
| - | - | - |
| 락온 마커·처형 안내 위젯 | `UCommonUserWidget` | 배치만 있어 C++ 부모 없이 바로 상속합니다. |
| `UAgDamageFloaterWidget` | `UCommonUserWidget` | `WBP_DamageFloater`의 부모. 숫자, 표시 시간과 떠오르는 높이 |
| `AAgDamageFloater` | `AActor` | 대미지 숫자를 맞은 위치에 띄우고, 떠오르며 투명해진 뒤 사라집니다. |

```mermaid
classDiagram
    direction TB
    class AAgBossCharacter {
        <<BP_Boss>>
        UWidgetComponent LockOnMarker
        UWidgetComponent ExecutionPrompt
    }
    class UAgDamageFloaterWidget {
        <<WBP_DamageFloater>>
    }
    UAgGameplayCueNotify_DamageFloater <|-- GC_DamageFloater
    UAgGameplayCueNotify_DamageFloater ..> AAgDamageFloater : Spawn
    AActor <|-- AAgDamageFloater
    AAgDamageFloater ..> UAgDamageFloaterWidget : SetDamage
    GC_DamageFloater --> UAgDamageFloaterWidget : FloaterWidget
    UCommonUserWidget <|-- UAgDamageFloaterWidget
    UCommonUserWidget <|-- WBP_LockOnMarker
    UCommonUserWidget <|-- WBP_ExecutionPrompt
    AAgBossCharacter --> WBP_LockOnMarker : LockOnMarker의 위젯
    AAgBossCharacter --> WBP_ExecutionPrompt : ExecutionPrompt의 위젯
```
