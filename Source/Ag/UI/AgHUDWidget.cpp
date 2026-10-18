// Copyright Woogle. All Rights Reserved.

#include "UI/AgHUDWidget.h"

#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Character/AgBossCharacter.h"
#include "Character/AgPlayerCharacter.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Core/AgAssetManager.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "Data/AgCombatRules.h"
#include "EngineUtils.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	/** UP 게이지·쿨다운 원형 표시 머티리얼의 스칼라 파라미터: 채우거나 덮는 비율 (0~1) */
	const FName RatioParameter(TEXT("Ratio"));
}

void UAgHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	StatusPanelVisibility = StatusPanel->GetVisibility();

	// 이미지가 없는 아이콘은 빈 사각형으로 그려지므로, 이미지를 넣기 전까지 숨깁니다.
	for (UImage* Icon : {SkillIconImage.Get(), UltimateIconImage.Get()})
	{
		if (!Icon->GetBrush().GetResourceObject())
		{
			Icon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (const AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(GetOwningPlayerPawn()))
	{
		ShowControlsGuide(*Player);

		if (UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent())
		{
			PlayerASC = ASC;
			PlayerSPChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(UAgAttributeSet::GetSPAttribute())
			                            .AddUObject(this, &ThisClass::HandlePlayerSPChanged);
		}
	}
}

void UAgHUDWidget::NativeDestruct()
{
	if (UAbilitySystemComponent* ASC = PlayerASC.Get())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UAgAttributeSet::GetSPAttribute()).Remove(PlayerSPChangedHandle);
	}
	PlayerASC.Reset();
	PlayerSPChangedHandle.Reset();

	Super::NativeDestruct();
}

void UAgHUDWidget::HandlePlayerSPChanged(const FOnAttributeChangeData& Data)
{
	// 대미지 GE가 SP를 줄이는 것은 가드와 가드 브레이크뿐입니다(전투 시스템 '피격 처리 순서').
	const UGameplayEffect* Effect = Data.GEModData ? Data.GEModData->EffectSpec.Def.Get() : nullptr;
	if (Data.NewValue < Data.OldValue && Effect && Effect->GetAssetTags().HasTagExact(AgGameplayTags::Effect_Damage))
	{
		bPlayerSPGuardDrop = true;
	}
}

void UAgHUDWidget::ShowControlsGuide(const AAgPlayerCharacter& Player)
{
	const UInputMappingContext* Mapping = Player.GetInputMapping();
	if (!Mapping)
	{
		return;
	}

	// 한 행동에 매핑이 여럿이면(이동의 W, A, S, D) 그 행동의 줄에 키를 모읍니다.
	TArray<const UInputAction*> Actions;
	TMap<const UInputAction*, TArray<FText>> KeysByAction;
	for (const FEnhancedActionKeyMapping& KeyMapping : Mapping->GetMappings())
	{
		if (!KeyMapping.Action)
		{
			continue;
		}
		TArray<FText>& Keys = KeysByAction.FindOrAdd(KeyMapping.Action);
		if (Keys.IsEmpty())
		{
			Actions.Add(KeyMapping.Action);
		}
		const FText* DisplayName = KeyDisplayNames.Find(KeyMapping.Key);
		Keys.Add(DisplayName ? *DisplayName : KeyMapping.Key.GetDisplayName(/*bLongDisplayName*/ false));
	}

	TArray<FText> KeyLines;
	TArray<FText> ActionLines;
	for (const UInputAction* Action : Actions)
	{
		KeyLines.Add(FText::Join(FText::FromString(TEXT(", ")), KeysByAction[Action]));
		ActionLines.Add(Action->ActionDescription);
	}
	const FText LineBreak = FText::FromString(TEXT("\n"));
	ControlsGuideKeys->SetText(FText::Join(LineBreak, KeyLines));
	ControlsGuideActions->SetText(FText::Join(LineBreak, ActionLines));
}

void UAgHUDWidget::HideStatus()
{
	bStatusHidden = true;
	StatusPanel->SetVisibility(ESlateVisibility::Collapsed);
}

TOptional<FUIInputConfig> UAgHUDWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Game, EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown,
	    /*bHideCursorDuringViewportCapture*/ true);
}

void UAgHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 궁극기 컷신(전투 HUD '상황별 표시 규칙'): 컷신이 플레이어 컨트롤러를 시네마틱 모드로 두는 동안 숨깁니다.
	const APlayerController* PlayerController = GetOwningPlayer();
	const bool bCutscene = PlayerController && PlayerController->bCinematicMode;
	StatusPanel->SetVisibility(bStatusHidden || bCutscene ? ESlateVisibility::Collapsed : StatusPanelVisibility);

	// 잔상은 게임 시간으로 흘러 슬로우모션에 함께 느려집니다.
	const float GameDeltaTime = GetWorld()->GetDeltaSeconds();
	if (const AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(GetOwningPlayerPawn()))
	{
		UpdatePlayer(*Player, GameDeltaTime);
	}

	if (!Boss.IsValid())
	{
		for (TActorIterator<AAgBossCharacter> It(GetWorld()); It; ++It)
		{
			Boss = *It;
			break;
		}
	}
	if (const AAgBossCharacter* BossCharacter = Boss.Get())
	{
		UpdateBoss(*BossCharacter, GameDeltaTime);
	}
}

void UAgHUDWidget::UpdatePlayer(const AAgPlayerCharacter& Player, float GameDeltaTime)
{
	const UAgAttributeSet* Stats = Player.GetAttributeSet();
	SetBarRatio(*PlayerHPBar, Stats->GetHP(), Stats->GetMaxHP());
	SetBarRatio(*PlayerMPBar, Stats->GetMP(), Stats->GetMaxMP());
	SetBarRatio(*PlayerSPBar, Stats->GetSP(), Stats->GetMaxSP());
	SetValueText(*PlayerHPText, Stats->GetHP(), Stats->GetMaxHP(), ShownHP);
	SetValueText(*PlayerMPText, Stats->GetMP(), Stats->GetMaxMP(), ShownMP);
	UpdateTrail(PlayerHPTrail, *PlayerHPTrailBar, Stats->GetMaxHP() > 0.f ? Stats->GetHP() / Stats->GetMaxHP() : 0.f, GameDeltaTime);
	UpdateTrail(PlayerSPTrail, *PlayerSPTrailBar, Stats->GetMaxSP() > 0.f ? Stats->GetSP() / Stats->GetMaxSP() : 0.f, GameDeltaTime,
	    bPlayerSPGuardDrop);
	bPlayerSPGuardDrop = false;

	// 슬롯은 MP·UP가 부족할 때만 흐리게 합니다. 스킬 쿨다운은 쿨다운 표시로 보여 줍니다.
	const UAbilitySystemComponent* ASC = Player.GetAbilitySystemComponent();
	SetSlotUsable(*SkillSlot, CanPayCost(*ASC, AgGameplayTags::Ability_Action_Skill));
	SetSlotUsable(*UltimateSlot, CanPayCost(*ASC, AgGameplayTags::Ability_Action_Ultimate));
	UpdateCooldown(*ASC, AgGameplayTags::Ability_Action_Skill, *SkillCooldownSweep, *SkillCooldownText);

	// 머티리얼이 이 비율만큼 테두리를 위쪽 가운데부터 시계 방향으로 채웁니다.
	if (UMaterialInstanceDynamic* Gauge = UltimateGauge->GetDynamicMaterial())
	{
		Gauge->SetScalarParameterValue(RatioParameter, Stats->GetMaxUP() > 0.f ? Stats->GetUP() / Stats->GetMaxUP() : 0.f);
	}

	FGameplayAbilitySpecHandle SkillHandle;
	const UAgGameplayAbility* Skill = Cast<UAgGameplayAbility>(FindAbility(*ASC, AgGameplayTags::Ability_Action_Skill, SkillHandle));
	const int32 SkillCost = Skill ? FMath::RoundToInt(Skill->GetCost().Amount) : 0;
	if (SkillCost != ShownSkillCost)
	{
		ShownSkillCost = SkillCost;
		SkillCostText->SetText(FText::AsNumber(SkillCost));
	}
}

void UAgHUDWidget::UpdateBoss(const AAgBossCharacter& BossCharacter, float GameDeltaTime)
{
	// 이름은 매 프레임 데이터에서 읽어 바뀌었을 때만 넣습니다. 에디터에서 데이터 파일을 다시 읽으면 바뀝니다.
	const FText& BossName = BossCharacter.GetCharacterData()->DisplayName;
	if (!BossNameText->GetText().IdenticalTo(BossName))
	{
		BossNameText->SetText(BossName);
	}

	const UAgAttributeSet* Stats = BossCharacter.GetAttributeSet();
	SetBarRatio(*BossHPBar, Stats->GetHP(), Stats->GetMaxHP());
	UpdateTrail(BossHPTrail, *BossHPTrailBar, Stats->GetMaxHP() > 0.f ? Stats->GetHP() / Stats->GetMaxHP() : 0.f, GameDeltaTime);

	// 그로기: 빈 바를 그로기 시간(착지한 순간부터 세는 GroggyTime 효과) 동안 다시 채웁니다. 착지 전은 빈 바이고, 처형이나
	// 히트스톱으로 그로기가 길어져 효과가 끝난 뒤에는 끝날 때까지 가득 찬 채로 둡니다.
	float PPBarRatio = Stats->GetMaxPP() > 0.f ? Stats->GetPP() / Stats->GetMaxPP() : 0.f;
	const UAbilitySystemComponent* BossASC = BossCharacter.GetAbilitySystemComponent();
	if (BossASC->HasMatchingGameplayTag(AgGameplayTags::State_Groggy))
	{
		const FGameplayEffectQuery Query =
		    FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(AgGameplayTags::State_GroggyTime));
		const TArray<TPair<float, float>> Times = BossASC->GetActiveEffectsTimeRemainingAndDuration(Query);
		if (!Times.IsEmpty() && Times[0].Value > 0.f)
		{
			PPBarRatio = FMath::Clamp(1.f - Times[0].Key / Times[0].Value, 0.f, 1.f);
		}
		else
		{
			PPBarRatio = BossASC->HasMatchingGameplayTag(AgGameplayTags::State_Airborne) ? 0.f : 1.f;
		}
	}
	BossPPBar->SetPercent(PPBarRatio);
	// 잔상은 바에 표시한 값을 따라갑니다. 그로기 중에는 다시 채우는 값입니다.
	UpdateTrail(BossPPTrail, *BossPPTrailBar, PPBarRatio, GameDeltaTime);
}

const UGameplayAbility* UAgHUDWidget::FindAbility(
    const UAbilitySystemComponent& ASC, const FGameplayTag& AbilityTag, FGameplayAbilitySpecHandle& OutHandle)
{
	// 입력 계층과 같은 행동 태그 검색입니다.
	const UAgAbilitySystemComponent* AgASC = Cast<UAgAbilitySystemComponent>(&ASC);
	const FGameplayAbilitySpec* Spec = AgASC ? AgASC->FindSpecByTag(AbilityTag) : nullptr;
	if (!Spec || !Spec->GetPrimaryInstance())
	{
		return nullptr;
	}
	OutHandle = Spec->Handle;
	return Spec->GetPrimaryInstance();
}

bool UAgHUDWidget::CanPayCost(const UAbilitySystemComponent& ASC, const FGameplayTag& AbilityTag)
{
	FGameplayAbilitySpecHandle Handle;
	const UGameplayAbility* Ability = FindAbility(ASC, AbilityTag, Handle);
	return Ability && Ability->CheckCost(Handle, ASC.AbilityActorInfo.Get());
}

void UAgHUDWidget::UpdateCooldown(const UAbilitySystemComponent& ASC, const FGameplayTag& AbilityTag, UImage& Sweep, UTextBlock& Text)
{
	float Remaining = 0.f;
	float Duration = 0.f;
	FGameplayAbilitySpecHandle Handle;
	if (const UGameplayAbility* Ability = FindAbility(ASC, AbilityTag, Handle))
	{
		Ability->GetCooldownTimeRemainingAndDuration(Handle, ASC.AbilityActorInfo.Get(), Remaining, Duration);
	}
	const bool bCoolingDown = Remaining > 0.f && Duration > 0.f;
	const ESlateVisibility CooldownVisibility = bCoolingDown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;

	Sweep.SetVisibility(CooldownVisibility);
	UMaterialInstanceDynamic* Material = bCoolingDown ? Sweep.GetDynamicMaterial() : nullptr;
	if (Material)
	{
		Material->SetScalarParameterValue(RatioParameter, Remaining / Duration);
	}

	Text.SetVisibility(CooldownVisibility);
	// 1초 이상은 정수, 1초 미만은 소수 한 자리, 둘 다 올림 (전투 HUD '스킬·궁극기 슬롯').
	const int32 Tenths = FMath::CeilToInt(Remaining * 10.f);
	const int32 Key = Remaining >= 1.f ? 1000 + FMath::CeilToInt(Remaining) : Tenths;
	if (bCoolingDown && Key != ShownCooldownKey)
	{
		ShownCooldownKey = Key;
		FNumberFormattingOptions OneDecimal;
		OneDecimal.MinimumFractionalDigits = 1;
		OneDecimal.MaximumFractionalDigits = 1;
		Text.SetText(Remaining >= 1.f ? FText::AsNumber(FMath::CeilToInt(Remaining)) : FText::AsNumber(Tenths / 10.f, &OneDecimal));
	}
}

void UAgHUDWidget::SetBarRatio(UProgressBar& Bar, float Value, float MaxValue)
{
	Bar.SetPercent(MaxValue > 0.f ? Value / MaxValue : 0.f);
}

void UAgHUDWidget::SetSlotUsable(UWidget& SlotWidget, bool bUsable) const
{
	SlotWidget.SetRenderOpacity(bUsable ? 1.f : DisabledSlotOpacity);
}

void UAgHUDWidget::SetValueText(UTextBlock& Text, float Value, float MaxValue, FIntPoint& ShownValues)
{
	const FIntPoint Values(FMath::RoundToInt(Value), FMath::RoundToInt(MaxValue));
	if (Values != ShownValues)
	{
		ShownValues = Values;
		const FNumberFormattingOptions& Format = FNumberFormattingOptions::DefaultNoGrouping();
		Text.SetText(FText::Format(INVTEXT("{0} / {1}"), FText::AsNumber(Values.X, &Format), FText::AsNumber(Values.Y, &Format)));
	}
}

void UAgHUDWidget::UpdateTrail(FAgTrail& Trail, UProgressBar& TrailBar, float BarRatio, float DeltaTime, bool bShowDrop) const
{
	const UAgCombatRules& Rules = UAgAssetManager::Get().GetCombatRules();
	if (Trail.LastBarRatio >= 0.f && BarRatio < Trail.LastBarRatio)
	{
		if (bShowDrop)
		{
			Trail.Wait = Rules.HUDTrailDelay;
			Trail.ShrinkElapsed = -1.f;
		}
		else if (Trail.Ratio <= Trail.LastBarRatio)
		{
			Trail.Ratio = BarRatio;
		}
	}
	Trail.LastBarRatio = BarRatio;

	if (Trail.Ratio <= BarRatio)
	{
		Trail.Ratio = BarRatio;
		Trail.ShrinkElapsed = -1.f;
	}
	else if (Trail.ShrinkElapsed < 0.f)
	{
		Trail.Wait -= DeltaTime;
		if (Trail.Wait <= 0.f)
		{
			Trail.ShrinkFrom = Trail.Ratio;
			Trail.ShrinkElapsed = 0.f;
		}
	}
	else
	{
		Trail.ShrinkElapsed += DeltaTime;
		const float Alpha = Rules.HUDTrailShrinkTime > 0.f ? FMath::Min(Trail.ShrinkElapsed / Rules.HUDTrailShrinkTime, 1.f) : 1.f;
		Trail.Ratio = FMath::Lerp(Trail.ShrinkFrom, BarRatio, Alpha);
	}
	TrailBar.SetPercent(Trail.Ratio);
}
