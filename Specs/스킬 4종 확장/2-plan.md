# 스킬 4종 확장 기술 설계 (Plan)

## 1. 영향 범위 및 아키텍처

### 지금 구조

- 키로 발동하는 어빌리티는 정의의 `InputTag`로 찾습니다. 입력 계층(`AAgPlayerCharacter`)은 입력 액션과 입력 태그의 짝을 `SetupPlayerInputComponent`의 표로 묶고, 그 태그의 어빌리티를 모두 발동해 봅니다. 막힘 검사·선입력·모션 캔슬은 입력 태그 단위라, 스킬 키가 넷이 되어도 규칙 코드는 그대로입니다.
- 쿨다운 효과는 어빌리티 인스턴스를 출처로 찾으므로(`CooldownQuery(Source)`), 같은 태그를 쓰는 어빌리티끼리도 쿨다운이 따로입니다.
- `HitReward`는 `Rewards`의 `AbilityTag`를 공격 어빌리티의 태그에 `HasTag`로 맞춥니다. 네 스킬이 `Ability.Action.Skill`을 함께 쓰면 '스킬 적중 UP +15' 행 하나가 넷에 모두 적용됩니다.
- HUD(`UAgHUDWidget`)는 스킬 칸 하나와 궁극기 칸 하나의 위젯을 직접 들고, `Input.Skill`·`Input.Ultimate`로 어빌리티를 찾아 코스트·흐림·쿨다운을 그립니다. 아이콘은 `WBP_HUD`의 이미지 브러시에 지정되어 있습니다.

### 바꾸는 곳

| 대상 | 바뀌는 내용 | 공통 규칙 |
| - | - | - |
| `AgGameplayTags` | `Input.Skill`을 지우고 `Input.Skill1`~`Input.Skill4`를 더합니다. `Ability.Action.Skill`은 네 스킬이 함께 씁니다. | 태그는 네이티브로만 선언합니다. 스킬별 어빌리티 태그는 그 태그를 읽는 규칙이 생길 때 그 스킬의 작업에서 더합니다. |
| `AAgPlayerCharacter` | `SkillAction`을 `Skill1Action`~`Skill4Action`으로 바꾸고 입력 짝 표에 네 줄을 둡니다. | 입력 계층의 규칙(막힘, 선입력, 떼면 끝나기)은 고치지 않습니다. |
| 입력 에셋 | `IA_Skill`을 복제해 `IA_Skill1`~`IA_Skill4`를 같은 설정으로 만들고 `IA_Skill`은 지웁니다. `IMC_Default`에서 E 매핑을 지우고 `One`~`Four` 키를 각 입력 액션에 매핑합니다. `BP_Player`의 네 입력 액션 기본값을 지정합니다. | 실행 구조의 에셋 참조라 에셋 기본값에 둡니다. 이름 변경 대신 복제해 리다이렉터를 남기지 않습니다. |
| `FAgAbilityData` | `TObjectPtr<UTexture2D> Icon`을 더합니다. HUD 칸이 그 입력의 어빌리티 아이콘을 보여 줍니다. | 스킬마다 고르는 콘텐츠의 참조라 기획 데이터에 둡니다(사용자 결정). JSON 판독기는 오브젝트 필드를 에셋 경로로 읽으므로 판독기는 고치지 않습니다. |
| `UAgSkillSlot` (신규, `UUserWidget`) · `WBP_SkillSlot` (신규) | 칸 하나: 입력 태그로 찾은 어빌리티의 아이콘, MP 코스트, MP·UP 부족 시 흐림, 쿨다운 표시. 스킬 칸 넷과 궁극기 칸이 같은 위젯을 씁니다. MP 코스트가 0이면 코스트를 숨기고, 쿨다운이 없으면 쿨다운 표시도 나오지 않으므로 궁극기 칸에는 아이콘만 보입니다. | 칸 다섯이 같은 규칙이라 한 클래스에 둡니다. 위젯 Tick은 쓰지 않고, 쿨다운 중에만 다음 틱 타이머로 갱신합니다(지금 HUD와 같은 방식). |
| `UAgHUDWidget` | 스킬·궁극기 칸 위젯과 그 갱신 코드를 `UAgSkillSlot` 다섯 개(`Skill1Slot`~`Skill4Slot`, `UltimateSlot`)로 바꿉니다. 칸과 입력 태그의 짝은 HUD 코드의 표로 정하고, ASC 알림(어트리뷰트 변화, 어빌리티 부여, 커밋, 데이터 다시 읽기)을 받아 칸들에 전합니다. UP 게이지는 HUD에 남습니다. | ASC 구독은 HUD 한 곳에서 합니다. 입력 계층처럼 짝을 코드 표로 두어 태그를 에셋에 흩지 않습니다. |
| `WBP_HUD` | 스킬 십자(칸 넷, 키 표시 넷)와 궁극기 칸·UP 게이지를 명세의 배치로 놓습니다. 키 표시는 반투명 원과 Small 글자로 된 고정 위젯입니다. `SkillIconImage`·`UltimateIconImage`의 브러시 이미지는 지웁니다. | 위치·모양은 위젯 에셋에 둡니다. |
| `M_CooldownSweep`, `M_UPGauge` | 칸 모양을 원으로 그리도록 `Radius`를 칸 크기의 절반으로 둡니다. `M_UPGauge`의 채움 위치는 지금 사각 둘레 길이로 재므로, 원에서 고르게 채우도록 위쪽 가운데부터 시계 방향 각도로 바꿉니다. | 머티리얼 계산은 Custom HLSL 노드 하나에 둡니다. |
| `WBP_ControlsGuide` | `Keys`의 `E`를 `1, 2, 3, 4`로, `Actions`의 `스킬`을 `스킬 1, 2, 3, 4`로 바꿉니다. | HUD와 조작 안내 팝업이 같이 씁니다. |
| `Scripts/ScenarioTest/runner.py` | 키 표 `KEYS`의 `Input.Skill`을 `Input.Skill1`~`Input.Skill4`(`One`~`Four`)로 바꿉니다. | 키 매핑이 바뀌면 이 표도 바꾸는 규칙을 따릅니다. |
| `Saved/ScenarioTest/Scenarios/player_skill.py` | 누르는 입력을 `Input.Skill2`로 바꿉니다. | 회귀 시나리오입니다. |

### 칸 배치 (`WBP_HUD`, 화면 오른쪽 아래 기준)

십자 가운데를 화면 오른쪽 아래 모서리에서 왼쪽 184px, 위 163px에 둡니다. 그러면 궁극기 칸 오른쪽 끝이 오른쪽 여백 48px, 스킬 4 칸 아래쪽 끝이 아래 여백 48px에 닿습니다.

| 위젯 | 중심(오른쪽 아래 모서리 기준) | 크기 |
| - | - | - |
| `Skill1Slot` | (−259, −163) | 80 |
| `Skill2Slot` | (−184, −238) | 80 |
| `Skill3Slot` | (−109, −163) | 80 |
| `Skill4Slot` | (−184, −88) | 80 |
| 키 표시 1·2·3·4 | (−214, −163), (−184, −193), (−154, −163), (−184, −133) | 28 |
| `UltimateSlot`, `UPGauge` | (−88, −259) | 80, 84 |

- `WBP_SkillSlot`: 아이콘 54, 쿨다운 원형 표시 76, 코스트는 가운데 정렬에 렌더 이동 (29, 29). 코스트 글자는 `TS_Small`에 검은 외곽선(쿨다운 숫자와 같은 2)을 더한 `TS_SmallOutline`을 씁니다. 키 표시는 어두운 원 위라 `TS_Small`을 씁니다.
- 머티리얼 크기 값: `M_CooldownSweep` `Size` 76·`Radius` 38, `M_UPGauge` `Size` 84·`Radius` 42·`Width` 4.

### 영향 없는 곳

- 상태별 허용 입력·선입력·모션 캔슬·후딜 이동 캔슬 코드는 입력 태그 단위라 고치지 않습니다.
- 쿨다운 충전(`MaxCharges`)과 HUD의 충전 횟수 표시는 이 작업에서 다루지 않습니다.
- `HitReward`의 `Rewards`(`Ability.Action.Skill` UP +15)는 그대로 넷에 적용됩니다.

## 2. GAS 및 데이터 설계

### 태그 (`AgGameplayTags`)

```cpp
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Skill1);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Skill2);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Skill3);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Skill4);
```

### `DesignData/Abilities.json`

- `Skill` 항목의 ID를 `Skill2`로, `InputTag`를 `Input.Skill2`로 바꾸고 `"Icon": "/Game/UI/Icons/T_Skill_DashSlash.T_Skill_DashSlash"`를 더합니다.
- `Skill1`·`Skill3`·`Skill4`는 `Skill2`와 같은 내용에 `InputTag`만 `Input.Skill1`·`Input.Skill3`·`Input.Skill4`로 다릅니다.
- `Ultimate`에 `"Icon": "/Game/UI/Icons/T_Ultimate_Judgment.T_Ultimate_Judgment"`를 더합니다.

### `DesignData/Characters.json`

`Player`의 `Abilities`에서 `Skill`을 `Skill1`, `Skill2`, `Skill3`, `Skill4`로 바꿉니다.

## 3. 인터페이스 & 함수 시그니처

```cpp
// AgAbilityData.h (FAgAbilityData)
UPROPERTY()
TObjectPtr<UTexture2D> Icon;

// AgPlayerCharacter.h
UPROPERTY(EditDefaultsOnly, Category = "Input")
TObjectPtr<UInputAction> Skill1Action;
// Skill2Action, Skill3Action, Skill4Action

// AgSkillSlot.h
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgSkillSlot : public UUserWidget
{
public:
	/** 이 칸이 보여 줄 입력과 그 어빌리티를 가진 ASC를 정합니다. */
	void SetInput(UAgAbilitySystemComponent* InASC, const FGameplayTag& InInputTag);

	/** 아이콘, MP 코스트, 흐림을 다시 그립니다. */
	void UpdateAbility();

	/** 쿨다운 표시를 다시 그리고, 쿨다운 중이면 다음 틱에 다시 부릅니다. */
	void UpdateCooldown();

protected:
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> IconImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CostText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CooldownSweep;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CooldownText;

	UPROPERTY(EditAnywhere, Category = "HUD", meta = (ClampMin = 0, ClampMax = 1))
	float DisabledOpacity = 1.f;

private:
	const UGameplayAbility* FindAbility(FGameplayAbilitySpecHandle& OutHandle) const;

	TWeakObjectPtr<UAgAbilitySystemComponent> ASC;
	FGameplayTag InputTag;
	int32 ShownCost = INDEX_NONE;
	FIntPoint ShownCooldown = FIntPoint(INDEX_NONE, INDEX_NONE);
	FTimerHandle CooldownTimer;
};

// AgHUDWidget.h: SkillSlot·SkillIconImage·SkillCostText·SkillCooldownSweep·SkillCooldownText·UltimateIconImage,
// DisabledSlotOpacity, FindAbility, CanPayCost, SetSlotUsable, UpdateSkillCooldown과 그 상태를 지우고 아래를 둡니다.
UPROPERTY(meta = (BindWidget))
TObjectPtr<UAgSkillSlot> Skill1Slot;
// Skill2Slot, Skill3Slot, Skill4Slot
UPROPERTY(meta = (BindWidget))
TObjectPtr<UAgSkillSlot> UltimateSlot;
```

## 4. 문서 반영 (4단계)

| 문서 | 반영 |
| - | - |
| [캐릭터 기반 구조와 GAS](<../../Docs/Architecture/캐릭터 기반 구조와 GAS.md>) '입력' | HUD 칸이 찾는 입력 태그를 `Input.Skill1`~`Input.Skill4`, `Input.Ultimate`로 고칩니다. |
| [데이터 로딩과 스키마](<../../Docs/Architecture/데이터 로딩과 스키마.md>) '어빌리티' | `Icon` 필드(선택, HUD 칸 아이콘 텍스처 경로)를 더합니다. |
| 데이터 로딩과 스키마 'JSON 외부 데이터 위치' | 'HUD 스킬·궁극기 슬롯' 행을 `WBP_SkillSlot` 기본값의 `DisabledOpacity`와 `WBP_HUD`의 칸 배치로 고치고, 아이콘 이미지를 뺍니다. |
| 기획서·아트 문서 | 1-spec의 '기존 기획과의 관계' 표를 따릅니다. |
