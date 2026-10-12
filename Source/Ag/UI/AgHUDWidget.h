// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "InputCoreTypes.h"
#include "AgHUDWidget.generated.h"

class AAgBossCharacter;
class AAgPlayerCharacter;
class UAbilitySystemComponent;
class UGameplayAbility;
class UImage;
class UProgressBar;
class UTextBlock;
struct FGameplayAbilitySpecHandle;
struct FGameplayTag;
struct FOnAttributeChangeData;

/**
 * 전투 HUD. WBP_HUD의 부모로, 배치는 위젯에 있고 이 클래스는 값만 넣습니다.
 * 월드에 표시하는 락온 마커와 처형 안내는 보스에 있고, 대미지 숫자는 대미지 숫자 큐(GC_DamageNumber)가 띄웁니다.
 * 활성인 동안 입력은 게임에만 가고 커서는 숨깁니다(게임 플로우 'Boss Stage').
 */
UCLASS(Abstract)
class UAgHUDWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	/** 사망 연출(전투 HUD '상황별 표시 규칙'): 플레이어 상태, 슬롯, 보스 상태, 조작 안내를 숨깁니다. */
	void HideStatus();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 아래 위젯은 모두 WBP_HUD에 있어야 합니다(BindWidget). 빠지면 위젯 블루프린트 컴파일이 오류로 알립니다.

	/** 사망 연출과 궁극기 컷신에 숨기는 요소(플레이어 상태, 슬롯, 보스 상태, 조작 안내)를 모두 담습니다. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> StatusPanel;

	/** 플레이어 상태: SP 바 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PlayerSPBar;

	/** 플레이어 상태: SP 바 뒤에 두는 감소 잔상. 가드 피격으로 준 SP만 남깁니다. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PlayerSPTrailBar;

	/** 플레이어 상태: HP 바 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PlayerHPBar;

	/** 플레이어 상태: HP 바 뒤에 두는 감소 잔상 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PlayerHPTrailBar;

	/** 플레이어 상태: HP 수치 (현재값 / 최대값) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PlayerHPText;

	/** 플레이어 상태: MP 바 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PlayerMPBar;

	/** 플레이어 상태: MP 수치 (현재값 / 최대값) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PlayerMPText;

	/** 스킬 슬롯. MP가 부족한 동안 흐려집니다. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> SkillSlot;

	/** 스킬 슬롯: 스킬 아이콘. 디자이너에서 브러시에 텍스처나 머티리얼을 넣고, 넣지 않으면 숨깁니다. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> SkillIconImage;

	/** 스킬 슬롯: 스킬의 MP 코스트 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SkillCostText;

	/** 스킬 슬롯: 쿨다운 표시의 원형 표시. 머티리얼이 남은 쿨다운 비율만큼 슬롯을 덮습니다. 스킬 슬롯 밖에 있어 흐려지지 않습니다. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> SkillCooldownSweep;

	/** 스킬 슬롯: 쿨다운 표시의 숫자(남은 쿨다운). 스킬 슬롯 밖에 있어 흐려지지 않습니다. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SkillCooldownText;

	/** 궁극기 슬롯. UP가 가득 차지 않은 동안 흐려집니다. UP 게이지는 그 밖에 있어 흐려지지 않습니다. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> UltimateSlot;

	/** 궁극기 슬롯: 궁극기 아이콘. 스킬 아이콘과 같이 다룹니다. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> UltimateIconImage;

	/** 궁극기 슬롯: UP 게이지. 머티리얼이 UP 비율만큼 슬롯 테두리를 채웁니다. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> UltimateGauge;

	/** 보스 상태: 보스 이름 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BossNameText;

	/** 보스 상태: HP 바 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> BossHPBar;

	/** 보스 상태: HP 바 뒤에 두는 감소 잔상 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> BossHPTrailBar;

	/** 보스 상태: PP 바 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> BossPPBar;

	/** 보스 상태: PP 바 뒤에 두는 감소 잔상 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> BossPPTrailBar;

	/** 조작 안내: 키 열. 한 줄에 행동 하나의 키를 표시합니다. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ControlsGuideKeys;

	/** 조작 안내: 행동 이름 열. 키 열과 같은 줄 순서입니다. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ControlsGuideActions;

	/** UI 스타일 '입력 키 표시': 엔진 키 이름과 다르게 표시하는 키(마우스 입력 등)의 표시 이름 */
	UPROPERTY(EditAnywhere, Category = "HUD|Controls Guide")
	TMap<FKey, FText> KeyDisplayNames;

	/** 스킬·궁극기 슬롯: MP·UP가 부족해 흐린 슬롯의 불투명도 */
	UPROPERTY(EditAnywhere, Category = "HUD", meta = (ClampMin = 0, ClampMax = 1))
	float DisabledSlotOpacity = 1.f;

	/** 감소 잔상: 바가 줄어든 뒤 잔상이 기다리는 시간 */
	UPROPERTY(EditAnywhere, Category = "HUD|Trail", meta = (Units = "s", ClampMin = 0))
	float TrailDelay = 0.f;

	/** 감소 잔상: 기다린 뒤 바의 현재 값까지 줄어드는 시간 */
	UPROPERTY(EditAnywhere, Category = "HUD|Trail", meta = (Units = "s", ClampMin = 0))
	float TrailShrinkTime = 0.f;

private:
	/** 바 하나의 감소 잔상 */
	struct FAgTrail
	{
		/** 잔상의 현재 길이 (0~1) */
		float Ratio = 1.f;
		/** 지난 프레임에 바가 표시한 비율. 첫 프레임 전에는 음수입니다. */
		float LastBarRatio = -1.f;
		/** 잔상이 줄어들기 시작할 때까지 남은 시간 */
		float Wait = 0.f;
		/** 줄어들기 시작할 때의 길이 */
		float ShrinkFrom = 1.f;
		/** 줄어들기 시작한 뒤 지난 시간. 줄어들지 않는 동안은 음수입니다. */
		float ShrinkElapsed = -1.f;
	};

	/** 조작 안내: 플레이어 입력 매핑의 행동마다 키와 행동 이름(입력 액션의 설명)을 매핑 순서대로 한 줄씩 채웁니다. */
	void ShowControlsGuide(const AAgPlayerCharacter& Player);

	void UpdatePlayer(const AAgPlayerCharacter& Player, float GameDeltaTime);
	void UpdateBoss(const AAgBossCharacter& BossCharacter, float GameDeltaTime);

	/** 부여된 어빌리티 중 AbilityTag를 가진 것을 스펙 핸들(OutHandle)과 함께 돌려줍니다. 없으면 null입니다. */
	static const UGameplayAbility* FindAbility(const UAbilitySystemComponent& ASC, const FGameplayTag& AbilityTag, FGameplayAbilitySpecHandle& OutHandle);

	/** 어빌리티 자신의 비용 확인을 통과하면 참입니다(슬롯 흐림: MP·UP 부족). */
	static bool CanPayCost(const UAbilitySystemComponent& ASC, const FGameplayTag& AbilityTag);

	/** 쿨다운 표시: 어빌리티 쿨다운의 남은 비율과 시간을 표시하고, 쿨다운 중이 아니면 숨깁니다. */
	void UpdateCooldown(const UAbilitySystemComponent& ASC, const FGameplayTag& AbilityTag, UImage& Sweep, UTextBlock& Text);

	static void SetBarRatio(UProgressBar& Bar, float Value, float MaxValue);
	void SetSlotUsable(UWidget& SlotWidget, bool bUsable) const;

	/** 반올림한 정수가 바뀔 때만 '현재값 / 최대값'을 씁니다. */
	static void SetValueText(UTextBlock& Text, float Value, float MaxValue, FIntPoint& ShownValues);

	/**
	 * 바가 줄면 잔상은 (줄어드는 중이라도) 그 자리에서 다시 기다린 뒤 바의 현재 값까지 줄어듭니다.
	 * bShowDrop이 거짓인 감소는 잔상을 다시 세우지 않고, 잔상이 없으면 잔상도 바를 바로 따라갑니다.
	 */
	void UpdateTrail(FAgTrail& Trail, UProgressBar& TrailBar, float BarRatio, float DeltaTime, bool bShowDrop = true) const;

	/** 대미지 GE(가드·가드 브레이크)로 SP가 줄면 다음 틱에 SP 감소 잔상을 세웁니다. 회피·달리기 소모는 남기지 않습니다. */
	void HandlePlayerSPChanged(const FOnAttributeChangeData& Data);

	TWeakObjectPtr<AAgBossCharacter> Boss;

	/** SP 변화를 듣는 플레이어 ASC와 그 델리게이트 핸들 */
	TWeakObjectPtr<UAbilitySystemComponent> PlayerASC;
	FDelegateHandle PlayerSPChangedHandle;

	/** 지난 틱 뒤 가드 피격으로 SP가 줄었습니다. */
	bool bPlayerSPGuardDrop = false;

	/** 사망 연출로 StatusPanel을 숨겼습니다. 궁극기 컷신과 달리 다시 표시하지 않습니다. */
	bool bStatusHidden = false;

	/** WBP_HUD에 정한 StatusPanel의 표시 방식. 궁극기 컷신이 끝나면 되돌립니다. */
	ESlateVisibility StatusPanelVisibility = ESlateVisibility::SelfHitTestInvisible;

	int32 ShownSkillCost = INDEX_NONE;
	int32 ShownCooldownKey = INDEX_NONE;
	FIntPoint ShownHP = FIntPoint(INDEX_NONE, INDEX_NONE);
	FIntPoint ShownMP = FIntPoint(INDEX_NONE, INDEX_NONE);
	FAgTrail PlayerHPTrail;
	FAgTrail PlayerSPTrail;
	FAgTrail BossHPTrail;
	FAgTrail BossPPTrail;
};
