# 회복병 기술 설계

[1-spec](<1-spec.md>)을 구현하는 설계입니다. 새 어빌리티 클래스, 새 Task, 새 C++ 클래스는 없습니다. 어빌리티는 `Abilities.json` 항목 하나이고, 나머지는 어트리뷰트 2개, 데이터 필드 2종, 애님 그래프 층 하나, 전용 모션과 회복 효과 큐입니다.

## 1. 영향 범위 및 아키텍처

### 명세와 현재 구조를 맞춰 본 결과

| 항목 | 확인한 사실 | 설계에 주는 영향 |
| - | - | - |
| 잔여 횟수 이어 가기 | 레벨 이동은 타이틀, 스테이지 선택, Restart, 적 선택의 Start뿐입니다(`AgGameMode.cpp`, `AgSimulationRoomGameMode.cpp`). 던전의 보스도 같은 레벨에 있습니다([게임 플로우](<../../Docs/Design/System/게임 플로우.md>) 'Dungeon'). | 캐릭터의 어트리뷰트로 두면 같은 도전에서 이어지고, 레벨을 다시 읽는 새 도전에서 3/3이 됩니다. 레벨을 건너는 저장은 만들지 않습니다. |
| R 키 | `IMC_Default`에 묶여 있지 않습니다. | 새 입력 액션을 더합니다. |
| 초안의 후보 모션 `Buff` | 오른손이 얼굴 앞이 아니라 머리 위로 올라가, 손바닥과 입이 약 55cm 떨어집니다. 프로젝트의 애니메이션 시퀀스 1,115개를 모두 재 봐도 손이 입 근처에 머무는 모션은 사격 조준과 맨손 공격의 가드 자세뿐입니다. | 전용 모션을 만듭니다. 아래 '마시는 모션'을 따릅니다. |
| 병 메시 | 프로젝트와 팩에 없습니다. | 엔진 `Cylinder`를 줄여 붉은 재질을 입힌 스탠드인을 나이아가라로 그립니다. |
| 몸 회전 | `IsFacingFixed`는 어빌리티 몽타주가 재생 중이면 회전과 제자리 회전을 멈춥니다. | 그대로면 마시는 중 방향을 바꿀 수 없습니다. 상체 몽타주는 뺍니다. |

### 명세에서 이렇게 읽고 구현하는 것

- 시뮬레이션 룸도 3/3으로 시작하고 Restart에서 3/3으로 돌아갑니다. 연습장 규칙은 '다시 시작하면 모든 상태 초기화'뿐이라 따로 만들 동작이 없습니다.
- 가드 중과 모든 행동의 후딜(회피 후딜 포함)은 '서 있거나 걷거나 달리는 중'이 아니므로 그때 누른 R은 버립니다.
- 버린 R은 처형 키와 같이 기억한 선입력을 지웁니다([플레이어 시스템](<../../Docs/Design/System/플레이어 시스템.md>) '선입력'의 '새로 누른 입력이 기억하던 입력을 대신합니다').
- 회복병도 행동이므로, 다른 행동처럼 시작할 때 회피 반격·가드 반격 기회와 기본 공격 2타 시작 상태를 끝냅니다.
- 삼키는 순간의 효과음은 명세에 없어 만들지 않습니다.

### 변경 대상

| 대상 | 변경 |
| - | - |
| `AgGameplayTags` | `Input.Potion`, `GameplayCue.PotionBottle`, `GameplayCue.PotionHeal` |
| `UAgAttributeSet`, `FAgAttributes`, `AAgCharacterBase::InitializeAttributes` | 어트리뷰트 `Potion`, `MaxPotion`과 범위 고정 |
| `FAgEffectModifier`, `AgEffectData` | 다른 어트리뷰트에 비례하는 크기(`Coefficient`, `BackingAttribute`) |
| `FAgAbilityData`, `UAgGameplayAbility::CheckCost` | 어트리뷰트 발동 조건(`ActivationAttributes`) |
| `AAgCharacterBase::IsFacingFixed` | `DefaultSlot`을 쓰는 몽타주만 방향을 고정 |
| `AAgPlayerCharacter` | `PotionAction`, R 선입력 제외 |
| `UAgHUDWidget`, `WBP_HUD`, `WBP_ControlsGuide` | 회복병 칸과 횟수, 조작 안내 한 줄 |
| `ABP_Character` | 상체 층(스켈레톤에 이미 있는 슬롯 `UpperBody`를 씀) |
| `BP_Player` | `PotionAction` 값 |
| 새 에셋 | `IA_Potion`(`IMC_Default`에 R), `A_Potion_Drink`, `AM_Player_Potion`, 아이콘 `T_Potion`, `NS_PotionBottle`, `M_PotionBottle`, `GC_PotionBottle`, `NS_PotionHeal`, `GC_PotionHeal` |
| `Characters.json`, `Abilities.json`, `Effects.json` | 아래 '데이터' |

### 구조 결정

| 항목 | 결정 | 이유 |
| - | - | - |
| 잔여 횟수 | 어트리뷰트 `Potion`, `MaxPotion`. 시작 값은 `Characters.json`의 린에게만 적고, 다른 캐릭터는 0입니다. | 효과 하나로 HP와 횟수를 함께 바꾸고, HUD는 기존 어트리뷰트 변경 구독을 그대로 씁니다. 쿨다운 충전(`MaxCharges`)은 시간이 지나면 돌아오는 구조라 맞지 않습니다. |
| 회복과 차감 | 즉시 효과 `PotionHeal` 하나를 타임라인 0.8초에 겁니다. | 두 변화가 한 번에 적용되고, 0.8초 전에 끊기면 Task가 시작되지 않아 아무것도 바뀌지 않으며, 뒤에 끊겨도 즉시 효과는 되돌려지지 않습니다. 시각은 몽타주 위치라 시간 규칙의 모션 진행을 따릅니다. `Cost`는 발동할 때 치르므로 쓰지 않습니다. |
| 발동 조건 | 잔여 횟수 1회 이상 하나입니다. 어빌리티 정의의 새 필드 `ActivationAttributes`를 `CheckCost`가 `Cost`와 함께 봅니다. | 어빌리티 항목만 읽으면 조건이 다 보이고, 엔진의 발동 검사와 HUD의 흐림이 모두 `CheckCost`를 거쳐 한곳에서 정해집니다. `Cost`에 넣지 않는 것은 발동할 때 치르기 때문입니다. 대안은 입력 계층의 `CanUsePotion()`(한 줄, JSON 형식이 늘지 않음)이지만 조건이 항목 밖으로 나가고 HUD가 따로 흐려야 합니다. 효과로 태그를 관리하는 방식은 어트리뷰트와 같은 상태를 두 곳에 둡니다. |
| 행동 제한 | 태그는 `Ability.Action`, `State.Busy`는 어빌리티 내내, `State.Acting`은 달지 않습니다. `CancelAbilitiesWithTag`에 `Ability.Sprint`를 적습니다. | 피격 반응의 `Ability.Action` 취소와 Busy의 입력 차단·선입력을 그대로 쓰고, 이동(`CanMove`)은 끊지 않습니다. 달리기는 시작할 때 취소되고 Busy가 다시 시작을 막으며, 다 마신 뒤에는 달리기 규칙대로 Shift를 새로 눌러야 합니다. |
| 상체 재생 | `ABP_Character`의 공중 전환과 `DefaultSlot` 사이에 층을 하나 더합니다. 공중 전환까지의 포즈를 캐시하고, `Layered blend per bone`(가지 `spine_01`, 깊이 0, 메시 공간 회전 블렌드)의 바탕에 그 포즈를, 섞을 포즈에 그 포즈를 입력으로 받는 `UpperBody` 슬롯을 둡니다. 노드 5개(캐시 저장, 캐시 사용 2, 슬롯, 블렌드)이고 값은 C++에 두지 않습니다. | 명세의 '상체에만 섞기'에 필요한 최소 구성이고 제자리 회전 층과 같은 노드입니다. `AdditiveSlot`의 덧입힘은 그래프를 고치지 않지만, 손 위치가 밑 자세를 따라가 걷기·달리기에서 병이 입에 맞지 않아 쓰지 않습니다. |
| 슬롯 | 템플릿에서 온 `SK_Mannequin`에 이미 `DefaultGroup`으로 있고 아무도 쓰지 않던 `UpperBody`를 씁니다. 스켈레톤은 고치지 않습니다. | 그룹마다 몽타주 하나라는 엔진 규칙으로, 행동·피격 몽타주가 시작되면 마시는 몽타주가 멈춥니다. 그래프에서 `DefaultSlot`보다 앞이라 블렌드 아웃 중에는 들어오는 행동 몽타주가 덮습니다. |
| 몸 회전 | `IsFacingFixed`의 몽타주 조건에 `Montage->IsValidSlot(FAnimSlotGroup::DefaultSlotName)`을 더합니다. | 전신 몽타주만 방향을 고정한다는 뜻이 되어, 어빌리티 ID나 태그로 나누지 않습니다. 대상 바라보기와 상체 비틀기는 계속 계산되지만 층이 덮고, 끝나면 이어집니다. |
| 병 표시 | `AM_Player_Potion`에 엔진 `AnimNotify_GameplayCueState`를 동작 내내 두어 루프 큐 `GC_PotionBottle`을 켭니다. 큐는 왼손(`hand_l`)에 `NS_PotionBottle`을 붙이고, 이 시스템은 메시 입자 하나로 병을 그립니다. | C++이 없고 연출이 몽타주에 모입니다(무기 궤적과 같은 구조). 몽타주가 끊기면 엔진이 노티파이를 끝내 큐를 지웁니다. 나이아가라 메시에 쓰려면 재질에 사용 표시가 필요해 엔진 재질 대신 `M_PotionBottle`을 씁니다. 이미터는 `Inactive Response`를 Kill로 두어 큐가 지워지면 병이 바로 사라집니다. |
| 회복 효과 | 엔진 `AnimNotify_GameplayCue`를 `AM_Player_Potion`의 0.8초에 두어 `GameplayCue.PotionHeal`을 실행합니다. `GC_PotionHeal`은 데이터만 있는 `UAgGameplayCueNotify_Burst`이고, `root`에 붙여 `NS_PotionHeal`을 재생합니다. | 연출은 몽타주에 두는 공통 규칙([몽타주 타임라인과 구간 판정](<../../Docs/Architecture/몽타주 타임라인과 구간 판정.md>) '몽타주에 남는 노티파이')을 따릅니다. 삼키기 전에 끊기면 몽타주가 그 위치에 닿지 않아 재생되지 않습니다. 척추 뼈에 붙이면 빛가루가 그 뼈의 축 방향으로 날아가므로 `root`에 붙이고 이미터에서 가슴 높이를 정합니다. |
| HUD | `WBP_SkillSlot`(`UAgSkillSlot`)을 `Input.Potion`으로 재사용하고, 횟수 글자 `PotionCountText`를 HUD에 둡니다. | 아이콘은 스킬과 같이 `Abilities.json`의 `Icon`에 두고 원형 칸 모양이 그대로 옵니다. MP 코스트가 없어 칸의 코스트 글자는 숨고, 0회면 `CheckCost`가 실패해 MP가 모자란 스킬처럼 칸이 흐려집니다. |

### 마시는 모션

`A_Potion_Drink`는 린의 서 있는 자세인 Samurai `Idle2`를 복제해 왼팔과 머리만 움직임을 더한 1.2초짜리 시퀀스입니다. 왼손에 병을 쥐면 오른손의 카타나는 `Idle2`대로 낮게 남습니다. 몽타주 `AM_Player_Potion`은 이 시퀀스 하나를 배율 없이 `UpperBody` 슬롯에서 재생합니다.

| 몽타주 시각(초) | 동작 |
| - | - |
| 0 ~ 0.38 | 왼손을 몸에서 조금 띄우며 들어 올려 병 입구를 입에 댐. 손가락은 Dual Wield `Idle1`의 왼손 쥔 자세 |
| 0.38 ~ 0.8 | 고개를 13°까지 젖히고 병을 8°에서 30°까지 기울임. 끝에서 삼킴 |
| 0.8 ~ 1.2 | 병을 내리고 고개를 되돌림 |

- 병 입구 자리는 키마다 머리 뼈의 입 자리에서 정하고, 왼팔은 두 뼈 IK로 풉니다. 고개를 젖혀도 입구가 입에서 떨어지지 않습니다.
- 팔 길이는 시퀀스 안에서 그대로이고, 다른 팔·몸통·다리는 `Idle2` 그대로입니다.

### 공통 규칙 준수

- 어빌리티 하나는 `Abilities.json` 항목 하나로 읽힙니다. 자식 어빌리티, 흐름 Task, 어빌리티 ID 분기는 없습니다.
- 새 키는 C++가 필요한 경우라 입력 계층에 한 줄 더합니다([데이터 로딩과 스키마](<../../Docs/Architecture/데이터 로딩과 스키마.md>) '어빌리티').
- 몽타주와 ABP의 값은 에셋에, 횟수·회복 비율·타임라인은 JSON에 둡니다.

## 2. GAS 및 데이터 설계

### 태그

| 태그 | 뜻 |
| - | - |
| `Input.Potion` | 회복병 키 |
| `GameplayCue.PotionBottle` | 회복병을 마시는 동안 왼손에 보이는 병 |
| `GameplayCue.PotionHeal` | 회복병을 삼킨 순간의 회복 효과 |

### `Characters.json` (`Player`)

```json
"Attributes": {"Potion": 3, "MaxPotion": 3}
```

`Abilities`에 `Potion`을 더합니다.

### `Effects.json`

```json
"PotionHeal": {
  "Modifiers": [
    {"Attribute": "HP", "Coefficient": 0.4, "BackingAttribute": "MaxHP"},
    {"Attribute": "Potion", "Value": -1}
  ]
}
```

- `Coefficient`, `BackingAttribute`는 엔진 `FAttributeBasedFloat`의 이름이고, 코드 전용 효과 `PPRefill`이 쓰는 `MakeTargetAttribute`를 그대로 씁니다.
- HP는 `PreAttributeBaseChange`의 범위 고정으로 최대 HP에서 멈춥니다.

### `Abilities.json`

```json
"Potion": {
  "AbilityTags": ["Ability.Action"],
  "InputTag": "Input.Potion",
  "Icon": "/Game/UI/Icons/T_Potion.T_Potion",
  "ActivationBlockedTags": ["State.Busy", "State.Acting", "State.Guarding", "State.HitReaction", "State.Groggy", "State.Execution", "State.Dead", "State.Airborne"],
  "ActivationOwnedTags": ["State.Busy"],
  "CancelAbilitiesWithTag": ["Ability.Sprint"],
  "ActivationAttributes": [
    {"Attribute": "Potion", "AtLeast": 1}
  ],
  "Montage": {
    "Asset": "/Game/Animations/AM_Player_Potion.AM_Player_Potion",
    "Sections": {
      "Default": [
        {"Task": "ApplyEffect", "Start": 0.8, "Effect": "PotionHeal"}
      ]
    }
  }
}
```

### 입력

- `HandleInputPressed`의 선입력 제외에 `Input.Potion`을 더합니다(지금은 처형과 공중 점프).
- 키는 엔진 `Started`로만 받으므로 길게 눌러도 한 번입니다.
- 공중과 Busy에서는 기존 `IsActionInputBlocked`가 막고, 그 밖의 조건은 어빌리티의 발동 검사가 막습니다.

### HUD

- `WBP_HUD`의 `StatusPanel` 안, SP 바 12px 위에 왼쪽 여백을 맞춰 80px 칸을 둡니다. R은 칸의 오른쪽 테두리에 스킬 칸의 키 표시와 같은 모양으로, 횟수는 칸 오른쪽에 글자 Medium으로 `2/3`처럼 표시합니다.
- 모습은 구현 뒤 스크린샷으로 확인합니다.

## 3. 인터페이스 & 함수 시그니처

```cpp
// AgAbilityData.h
USTRUCT()
struct FAgAttributeCondition
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayAttribute Attribute;

	UPROPERTY()
	float AtLeast = 0.f;
};

// FAgAbilityData
UPROPERTY()
TArray<FAgAttributeCondition> ActivationAttributes;

// AgEffectData.h, FAgEffectModifier
UPROPERTY()
TOptional<float> Coefficient;

UPROPERTY()
FGameplayAttribute BackingAttribute;

// AgAttributeSet.h
UPROPERTY(BlueprintReadOnly, Category = "Stats")
FGameplayAttributeData Potion;
ATTRIBUTE_ACCESSORS_BASIC(UAgAttributeSet, Potion)

UPROPERTY(BlueprintReadOnly, Category = "Stats")
FGameplayAttributeData MaxPotion;
ATTRIBUTE_ACCESSORS_BASIC(UAgAttributeSet, MaxPotion)

// AgPlayerCharacter.h
UPROPERTY(EditDefaultsOnly, Category = "Input")
TObjectPtr<UInputAction> PotionAction;

// AgHUDWidget.h
UPROPERTY(meta = (BindWidget))
TObjectPtr<UAgSkillSlot> PotionSlot;

UPROPERTY(meta = (BindWidget))
TObjectPtr<UTextBlock> PotionCountText;

void UpdatePotion();
```
