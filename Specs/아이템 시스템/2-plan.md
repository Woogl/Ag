# 아이템 시스템 기술 설계 (Plan)

## 1. 영향 범위 및 아키텍처

| 대상 | 변경 |
| - | - |
| `DesignData/Items.json` (새 파일) | 아이템 정의. 지금은 `Potion` 하나 |
| `Data/AgItemData.h` (새 파일) | `EAgItemCategory`, `FAgItemData`, `UAgItemSet` |
| `Core/AgAssetManager` | `Items` 멤버, `JsonFiles`에 `Items.json`(Abilities.json보다 앞), `FindItem`, `GetItems` |
| `Item/AgInventoryComponent` (새 파일) | 아이템별 보유 수, 얻기(`AddItem`)·쓰기(`RemoveItem`), 바뀜 알림, 게임 월드에서 시작할 때 시작 보유 수로 채움 |
| `Game/AgPlayerController` | 컴포넌트 `Inventory`와 `GetInventory()` |
| `Data/AgAbilityData.h` | `FAgAbilityData::Item` 추가. `ActivationAttributes`와 `FAgAttributeCondition` 삭제(회복병만 썼음) |
| `AbilitySystem/AgGameplayAbility.cpp` | `CheckCost`: `Item`이 있으면 플레이어 컨트롤러의 인벤토리에 그 아이템이 1개 이상 |
| `AbilitySystem/Tasks/AgAbilityTask_UseItem` (새 파일) | 시작할 때 어빌리티의 `Item` 1개를 쓰고 끝나는 Task |
| `AbilitySystem/AgAttributeSet`, `Data/AgCharacterData.h`, `Character/AgCharacterBase.cpp` | `Potion`·`MaxPotion` 어트리뷰트와 그 범위 고정·초기화 삭제 |
| `UI/AgHUDWidget`, `UI/AgSkillSlot` | 회복병 횟수를 회복병 칸 어빌리티의 `Item`으로 인벤토리에서 읽고, 인벤토리 알림에 다시 그림. 슬롯의 `FindAbility`를 public으로 |
| `Game/AgCheatManager` | `AgAddItem`, `AgRemoveItem` |
| JSON | `Characters.json` Player의 `Potion`·`MaxPotion` 삭제, `Abilities.json` `Potion`의 `ActivationAttributes` → `Item`, 타임라인 0.8초에 `UseItem`, `Effects.json` `PotionHeal`의 Potion −1 삭제 |

공통 규칙과의 관계는 다음과 같습니다.

- **데이터**: 아이템 정의는 `Items.json` 한 곳에 둡니다. 쓰는 행동과 아이템의 연결은 행동 쪽(`Abilities.json`의 `Item`)에만 적어, 발동 조건과 쓰기가 같은 값을 봅니다.
- **어빌리티 하나 + 범용 Task**: 아이템 쓰기는 아이템을 쓰는 모든 행동이 함께 쓰는 기능 하나라 Task(`UseItem`)로 둡니다. 어빌리티 클래스는 `CheckCost`에서 보유 수만 봅니다.
- **주인**: 보유 수는 플레이어 컨트롤러의 컴포넌트가 맡습니다. 아이템은 캐릭터의 몸이 아니라 플레이어의 것이라, 컨트롤러는 플레이어를, 폰은 몸을 나타내는 엔진의 역할 구분을 따릅니다. 레벨을 다시 열면 컨트롤러도 새로 만들어지므로 Restart와 다른 스테이지 들어가기는 시작 보유 수에서 시작합니다.
- **찾는 길**: 어빌리티와 `UseItem`은 GAS 액터 정보의 `PlayerController`(캐릭터가 빙의할 때 `RefreshAbilityActorInfo`로 갱신)에서 컴포넌트를 찾습니다. HUD와 치트는 자기 플레이어 컨트롤러의 `GetInventory()`를 씁니다. 플레이어 컨트롤러가 없는 캐릭터(적)는 `Item`이 있는 행동을 발동하지 못합니다.
- **시작 시점**: 엔진 `UActorComponent::InitializeComponent`(게임 월드에서 액터를 만들 때, BeginPlay 전)에서 채웁니다. 컨트롤러는 로그인할 때 만들어지므로 HUD가 처음 읽기 전에 값이 있습니다.
- **같은 시각**: 회복(`ApplyEffect` `PotionHeal`)과 `UseItem`은 0.8초에 함께 시작하고 서로의 결과를 바꾸지 않아 순서가 없어도 됩니다. 0.8초 전에 끊기면 둘 다 시작하지 않습니다.
- **위젯 갱신**: HUD는 인벤토리 알림(`OnItemsChanged`)으로 다시 그립니다. Tick을 쓰지 않습니다.

## 2. GAS 및 데이터 설계

### `Items.json`

```json
{
  "Potion": {
    "Category": "Consumable",
    "MaxCount": 3,
    "StartCount": 3
  }
}
```

| 필드 | 뜻 | 기본값 |
| - | - | - |
| `Category` | `Consumable`(소비 아이템), `Currency`(재화 아이템). 항목마다 명시합니다. | `Consumable` |
| `MaxCount` | 최대 보유 수 | 1 |
| `StartCount` | 스테이지에 들어갈 때의 보유 수. 최대 보유 수를 넘으면 최대 보유 수로 시작합니다. | 0 |

- 카테고리는 기획서의 분류를 데이터에 적는 값이고, 지금 코드는 이 값으로 나누지 않습니다. 소비 아이템은 행동의 `Item`이 가리키는 아이템, 재화 아이템은 치트와 앞으로 얻고 내는 곳이 수를 더하고 빼는 아이템입니다.
- 아이콘·이름·설명은 아직 보여 줄 화면이 없어 두지 않습니다. 회복병 칸의 아이콘은 지금처럼 어빌리티의 `Icon`입니다.

### `Abilities.json`의 `Potion`

```json
"Potion": {
  "AbilityTags": ["Ability.Action"],
  "InputTag": "Input.Potion",
  "Icon": "/Game/UI/Icons/T_Potion.T_Potion",
  "ActivationBlockedTags": [...],
  "ActivationOwnedTags": ["State.Busy"],
  "CancelAbilitiesWithTag": ["Ability.Sprint"],
  "Item": "Potion",
  "Montage": {
    "Asset": "/Game/Animations/AM_Player_Potion.AM_Player_Potion",
    "Sections": {
      "Default": [
        {"Task": "ApplyEffect", "Start": 0.8, "Effect": "PotionHeal"},
        {"Task": "UseItem", "Start": 0.8}
      ]
    }
  }
}
```

- `Item`: 선택. 이 행동이 쓰는 아이템의 ID(`Items.json`). 있으면 그 아이템이 1개 이상일 때만 발동하고(`CheckCost`라 HUD 칸의 흐림도 따름), 발동할 때는 쓰지 않습니다.
- `UseItem`: Params 필드가 없습니다(공통 시각 필드만). 시작할 때 어빌리티의 `Item` 1개를 플레이어 컨트롤러의 인벤토리에서 빼고 끝납니다. 발동(`Tasks`)과 타임라인 모두에 둘 수 있습니다. 쓸 아이템이 없으면 경고 로그만 남기고, 어빌리티를 끝내지 않습니다(Task는 어빌리티를 끝내지 않음).
- `Effects.json`의 `PotionHeal`은 HP 회복 수정자 하나만 남습니다.
- 새 게임플레이 태그는 없습니다.

### 다시 읽기

| 데이터 | 반영 시점 |
| - | - |
| `StartCount` | 플레이어를 다시 만들 때(다음 플레이 시작, Restart) |
| `MaxCount` | 다음 얻기부터. HUD는 다시 읽기 알림(`OnJsonDataReloaded`)에 회복병 횟수를 다시 그립니다. 이미 가진 수가 새 최대 보유 수보다 많으면 그대로 둡니다. |
| 어빌리티 `Item` | `CheckCost`는 지금 정의, `UseItem`은 발동할 때 복사한 정의를 봅니다(다른 필드와 같음). |

## 3. 인터페이스 & 함수 시그니처

```cpp
// Data/AgItemData.h
UENUM()
enum class EAgItemCategory : uint8
{
	Consumable,
	Currency
};

USTRUCT()
struct FAgItemData
{
	GENERATED_BODY()

	UPROPERTY()
	EAgItemCategory Category = EAgItemCategory::Consumable;

	UPROPERTY()
	int32 MaxCount = 1;

	UPROPERTY()
	int32 StartCount = 0;
};

UCLASS()
class UAgItemSet : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, VisibleAnywhere, Category = "Items")
	TMap<FName, FAgItemData> Items;
};

// Core/AgAssetManager.h
/** 데이터를 다시 읽으면 바뀌므로 포인터는 그 함수 안에서만 씁니다. */
const FAgItemData* FindItem(FName Id) const;
const TMap<FName, FAgItemData>& GetItems() const;

// Item/AgInventoryComponent.h
UCLASS()
class UAgInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAgInventoryComponent();

	int32 GetCount(FName Item) const;

	/** 최대 보유 수까지만 더하고, 더한 수를 돌려줍니다. */
	int32 AddItem(FName Item, int32 Count);

	/** 가진 수가 모자라면 빼지 않고 false를 돌려줍니다. */
	bool RemoveItem(FName Item, int32 Count);

	FSimpleMulticastDelegate OnItemsChanged;

protected:
	virtual void InitializeComponent() override;

private:
	// PIE 중 디테일 패널과 시나리오 테스트가 읽습니다.
	UPROPERTY(VisibleInstanceOnly, Category = "Items")
	TMap<FName, int32> Counts;
};

// Game/AgPlayerController.h
UAgInventoryComponent* GetInventory() const;

UPROPERTY(VisibleAnywhere, Category = "Items")
TObjectPtr<UAgInventoryComponent> Inventory;

// Data/AgAbilityData.h — FAgAbilityData
UPROPERTY(meta = (DataId = "Items"))
FName Item;

// AbilitySystem/Tasks/AgAbilityTask_UseItem.h
USTRUCT()
struct FAgUseItemParams : public FAgTaskParams
{
	GENERATED_BODY()

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

UCLASS()
class UAgAbilityTask_UseItem : public UAbilityTask
{
	GENERATED_BODY()

public:
	virtual void Activate() override;
};

// UI/AgSkillSlot.h — private → public
const UAgGameplayAbility* FindAbility() const;

// Game/AgCheatManager.h
UFUNCTION(Exec)
void AgAddItem(FName Id, int32 Count);

UFUNCTION(Exec)
void AgRemoveItem(FName Id, int32 Count);
```

- `AddItem`, `RemoveItem`은 `Items.json`에 없는 ID와 0 이하의 수에는 아무것도 하지 않습니다(없는 ID는 경고 로그). 바뀐 경우에만 `OnItemsChanged`를 알립니다.
- 치트는 결과를 '아이템 +더한 수 (보유/최대)'로 알리고, 모자라서 내지 못하면 가진 수를 알립니다.
- HUD: `PlayerAttributes`와 어트리뷰트 처리에서 Potion 두 개를 빼고, `UpdatePotion`은 회복병 칸 어빌리티의 `Item`으로 소유 플레이어 컨트롤러의 인벤토리에서 '보유/최대'를 그립니다. 인벤토리 알림에는 `UpdatePotion`과 `UpdateSkillSlots`(흐림)를, 스펙 바뀜(부여)과 데이터 다시 읽기에는 `UpdatePotion`을 더 부릅니다.

## 4. 검증 방법

- 빌드는 [에디터 작업 스킬](<../../.agents/skills/ag-editor-workflow/SKILL.md>)의 순서를 따르고, 공유 에디터를 닫을 수 없으면 임시 사본에서 빌드·테스트합니다.
- 회귀 시나리오 `item_potion.py`(`Saved/ScenarioTest/Scenarios`)를 더합니다: 시작 3개와 HUD 3/3, 마실 때마다 0.8초에 하나씩 줄어듦, 0개에서 R은 마시지 않고 칸이 흐려짐, 0.8초 전 피격 중단은 줄지 않음, 얻기 상한, 레벨을 다시 열면 3개.
- 재화는 테스트하는 동안만 `Items.json`에 시험용 재화 아이템(`TestCurrency`, 최대 1000)을 넣고 치트로 얻기 상한, 모자랄 때 내지 않기, 내기를 확인한 뒤 지웁니다.
- 기존 회귀 시나리오를 바꾸기 전과 후에 돌려 비교합니다.
