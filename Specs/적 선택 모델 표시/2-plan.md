# 적 선택 모델 표시 기술 설계 (Plan)

## 1. 영향 범위 및 아키텍처

### 1.1 모델을 그리는 방법

엔진의 `USceneCaptureComponent2D`가 월드에 띄운 표시용 캐릭터를 렌더 타깃에 그리고, 카드의 `UImage`가 그 렌더 타깃을 브러시로 표시합니다. UMG의 3D 뷰포트 위젯(`UViewport`)은 엔진에서 Experimental이라 쓰지 않습니다.

- **표시용 캐릭터는 그 적의 클래스 그대로** 스폰합니다. 메시, 무기 컴포넌트(블루프린트에서 소켓에 붙인 것 포함), ABP를 엔진의 생성 과정이 그대로 만들므로 카드의 모습이 게임과 같습니다. 클래스 기본값에서 메시와 무기만 골라 복사하는 방법은 블루프린트 컴포넌트의 부착 정보가 SCS 노드에 있어 엔진의 생성 과정을 다시 짜야 하므로 쓰지 않습니다.
  - `SpawnActorDeferred`로 만들고 `AutoPossessAI = Disabled`로 AI 컨트롤러 없이 끝냅니다. 행동 트리·시야·전투 진입이 돌지 않고, 게임 모드의 전투원 등록과 보스 등록은 월드 BeginPlay에서만 하므로 나중에 스폰한 표시용 캐릭터는 들어가지 않습니다. Start는 레벨을 다시 로드하므로 표시용 캐릭터는 전투에 남지 않습니다.
  - 적 선택 화면이 떠 있는 동안 게임은 멈춰 있으므로, 표시용 캐릭터의 스켈레탈 메시와 씬 캡처만 `SetTickableWhenPaused(true)`로 멈춘 동안에도 틱합니다. 이동 컴포넌트는 틱하지 않아 떨어지지 않습니다.
- **보이지 않게 하는 방법**: 표시용 캐릭터의 모든 프리미티브에 `SetVisibleInSceneCaptureOnly(true)`를 주고, 씬 캡처는 `PRM_UseShowOnlyList`로 그 캐릭터만 그립니다. 엔진은 ShowOnly 목록 밖의 프리미티브를 그 캡처의 그림자 캐스터에서도 빼므로(`ShadowSetup.cpp`) 레벨 지형이 모델을 가리지 않습니다.
- **조명**: 표시용 캐릭터의 프리미티브와 표시 액터의 조명을 라이팅 채널 1에만 둡니다. 레벨의 조명(채널 0)은 모델을 비추지 않고 모델의 그림자도 아레나에 생기지 않으며, 표시 액터의 조명은 레벨을 비추지 않습니다. 하늘 조명은 채널과 관계없이 들어오므로 캡처에서는 Lumen을 끄고 노출을 고정해 밝기를 일정하게 둡니다(값은 블루프린트 기본값).
- **위치**: 레벨 위 높은 곳(원점 위 1km)에 적 종류마다 20m 간격으로 띄웁니다. 레벨의 KillZ(-10m)와 원거리 차폐가 닿지 않고, 표시 액터끼리 조명이 겹치지 않습니다.
- **카드에 꽉 차게**: 캡처 카메라를 캐릭터 정면에 두고, 캐릭터 메시 범위(`Bounds`)의 가운데를 카메라 높이에 맞추며, 범위 높이에 여유 배율을 곱한 높이가 카메라 세로 시야에 꼭 맞는 거리로 놓습니다. 캡슐은 메시를 키운 보스(1.2배)보다 작아 기준으로 쓰지 않습니다. 적마다 크기 데이터를 두지 않습니다.

### 1.2 클래스와 책임

| 클래스 | 부모 | 변경 | 책임 |
| - | - | - | - |
| `AAgCharacterPreview` (신규, `UI/`) | `AActor` | 신규 | 캐릭터 클래스 하나를 표시용으로 스폰하고 정면에서 렌더 타깃에 그림. 씬 캡처(`Capture`)와 앞쪽 조명(`Light`)을 가짐 |
| `AAgSimulationRoomGameMode` | `AAgGameMode` | 변경 | 후보 적마다 표시 액터를 처음 쓸 때 하나 스폰해 두고 렌더 타깃을 돌려줌(`GetEnemyPreview`). 표시 액터 클래스는 `EnemyPreviewClass` 기본값 |
| `UAgDropdown` | `UCommonUserWidget` | 변경 | 닫힌 칸의 위젯 클래스(`SelectedWidgetClass`)를 목록 항목(`OptionWidgetClass`)과 따로 둘 수 있음. 비워 두면 목록 항목과 같음 |
| `UAgDropdownOptionWidget` | `UCommonUserWidget` | 변경 | 받는 값이 글자에서 키와 글자로 바뀜(`SetOption`, 가상 함수) |
| `UAgEnemyCard` (신규, `UI/`) | `UAgDropdownOptionWidget` | 신규 | 적 드롭다운의 닫힌 칸. 키로 게임 모드에서 적 클래스와 렌더 타깃을 받아 모델을 표시하고, 적이 없으면 '+'를 표시 |

- 표시 액터의 주인은 게임 모드입니다. 게임 모드가 이미 후보 적(`EnemyClasses`)과 적 선택 화면 클래스를 가지고 있고, 화면과 카드도 게임 모드에서 후보를 받습니다. 카드 위젯이 표시 액터를 직접 가지면 `UComboBoxKey`가 목록을 다시 채울 때마다(자리 하나를 바꿀 때 모든 자리) 닫힌 칸 위젯을 새로 만들어 캐릭터를 다시 스폰하고 대기 모션이 처음부터 다시 시작되므로, 적 종류마다 하나를 레벨 수명 동안 둡니다. 같은 적을 고른 카드는 같은 렌더 타깃을 함께 표시합니다.
- 표시 액터와 표시용 캐릭터는 레벨과 함께 사라지므로 따로 지우지 않습니다.

### 1.3 에셋

| 에셋 | 내용 |
| - | - |
| `Content/UI/BP_EnemyPreview` (신규) | `AAgCharacterPreview`의 블루프린트. 캡처 시야각, 표시 플래그(대기·안개·구름 끔), 후처리(Lumen 끔, 노출 고정), 조명 세기와 위치, 렌더 타깃 크기, 여유 배율 |
| `Content/UI/WBP_EnemyCard` (신규) | `UAgEnemyCard`. 300 × 450 검은 바탕, 모델 칸(`ModelImage`), '+'(`EmptyMark`, 전용 글자 Size 96), 이름 칸 60px(`NameText`, Large) |
| `Content/UI/WBP_EnemySlot` (신규) | `UAgDropdown`. 화살표 없는 `ComboBox`, 안쪽 여백 0, `OptionWidgetClass` = `WBP_DropdownOption`, `SelectedWidgetClass` = `WBP_EnemyCard` |
| `WBP_EnemySelect` | `SlotList` 항목 클래스를 `WBP_EnemySlot`으로 |
| `BP_SimulationRoomGameMode` | `EnemyPreviewClass` = `BP_EnemyPreview` |

`WBP_Dropdown`은 스테이지 선택 화면이 그대로 씁니다. 위젯 블루프린트는 부모 위젯 트리의 위젯 값을 자식에서 바꿀 수 없어 적 드롭다운을 새 위젯으로 둡니다.

## 2. GAS 및 데이터 설계

- 게임플레이 태그, 어빌리티, 이펙트 변경 없음.
- `DesignData` JSON 변경 없음. 카드의 이름은 지금처럼 `Characters.json`의 `DisplayName`입니다. 표시 액터 클래스와 캡처 값은 실행 구조와 연출 값이므로 블루프린트 기본값에 둡니다.

## 3. 인터페이스 & 함수 시그니처

```cpp
// UI/AgCharacterPreview.h
UCLASS(Abstract)
class AAgCharacterPreview : public AActor
{
public:
	AAgCharacterPreview();

	/** 캐릭터를 AI 없이 스폰해 캡처 앞에 세웁니다. 한 번만 부릅니다. */
	void ShowCharacter(TSubclassOf<AAgCharacterBase> CharacterClass);

	UTextureRenderTarget2D* GetRenderTarget() const;

protected:
	virtual void BeginPlay() override;      // 렌더 타깃을 만들어 캡처에 연결

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneCaptureComponent2D> Capture;   // 루트. ShowOnly, FinalColorLDR, 매 프레임
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> Light;         // 캡처에 붙음, 라이팅 채널 1만
	UPROPERTY(EditDefaultsOnly) FIntPoint RenderTargetSize;                    // 모델 칸 비율(300 × 390)
	UPROPERTY(EditDefaultsOnly) float FramingScale;                            // 메시 범위 높이에 곱해 세로 시야에 맞출 높이

private:
	UPROPERTY() TObjectPtr<UTextureRenderTarget2D> RenderTarget;
	UPROPERTY() TObjectPtr<AAgCharacterBase> Character;
};

// Game/AgSimulationRoomGameMode.h (추가)
public:
	/** 후보 적의 모습을 그리는 렌더 타깃입니다. 처음 부를 때 표시 액터를 띄웁니다. 후보가 아니면 nullptr입니다. */
	UTextureRenderTarget2D* GetEnemyPreview(TSubclassOf<AAgCharacterBase> EnemyClass);
protected:
	UPROPERTY(EditDefaultsOnly, Category = "UI") TSubclassOf<AAgCharacterPreview> EnemyPreviewClass;
private:
	UPROPERTY() TMap<TSubclassOf<AAgCharacterBase>, TObjectPtr<AAgCharacterPreview>> EnemyPreviews;

// UI/AgDropdown.h (변경)
protected:
	/** 닫힌 칸의 위젯입니다. 비워 두면 OptionWidgetClass를 씁니다. */
	UPROPERTY(EditDefaultsOnly, Category = "UI") TSubclassOf<UAgDropdownOptionWidget> SelectedWidgetClass;
private:
	UFUNCTION() UWidget* GenerateSelectedWidget(FName Option);   // OnGenerateContentWidget
	UFUNCTION() UWidget* GenerateOptionWidget(FName Option);     // OnGenerateItemWidget
	UAgDropdownOptionWidget* CreateOptionWidget(TSubclassOf<UAgDropdownOptionWidget> WidgetClass, FName Option);

// UI/AgDropdownOptionWidget.h (변경)
public:
	virtual void SetOption(FName Key, const FText& Label);       // SetLabel 대체

// UI/AgEnemyCard.h (신규)
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgEnemyCard : public UAgDropdownOptionWidget
{
public:
	virtual void SetOption(FName Key, const FText& Label) override;
protected:
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UImage> ModelImage;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UWidget> EmptyMark;
};
```

## 4. 문서 반영 (4단계)

- [UI 시스템](<../../Docs/Design/System/UI 시스템.md>) '적 선택 화면': 카드 배치·크기, '적 카드' 규칙. '전용 글자' 표에 '+' 행.
- [캐릭터 기반 구조와 GAS](<../../Docs/Architecture/캐릭터 기반 구조와 GAS.md>) 클래스 표와 클래스 구조: `AAgCharacterPreview`, `UAgEnemyCard`, 게임 모드의 `EnemyPreviewClass`, 드롭다운의 `SelectedWidgetClass`.
