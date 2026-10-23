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
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "EngineUtils.h"
#include "Game/AgPlayerController.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/PlayerController.h"
#include "Input/CommonUIInputTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UI/AgPauseMenu.h"
#include "UI/AgUILayout.h"

namespace
{
	const FName RatioParameter(TEXT("Ratio"));
}

void UAgHUDWidget::HideStatus()
{
	bStatusHidden = true;
	StatusPanel->SetVisibility(ESlateVisibility::Collapsed);
}

void UAgHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// Esc는 Enhanced Input이 아니라 CommonUI UI 액션으로 받습니다. CommonUI의 Enhanced Input 지원은 전역 설정이고,
	// 타이틀에는 IMC_Default가 없어 팝업이 키를 찾지 못합니다.
	FBindUIActionArgs EscapeArgs(FUIActionTag::ConvertChecked(AgGameplayTags::UI_Action_Escape), false,
		FSimpleDelegate::CreateUObject(this, &ThisClass::OpenPauseMenu));
	// UI 액션 바인딩은 기본으로 메뉴 입력 모드에서만 받으므로, 게임 입력 모드인 HUD는 바꿔 둡니다.
	EscapeArgs.InputMode = ECommonInputMode::Game;
	RegisterUIActionBinding(EscapeArgs);
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

TOptional<FUIInputConfig> UAgHUDWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Game, EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown,
		true);
}

void UAgHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 궁극기 컷신은 플레이어 컨트롤러를 시네마틱 모드로 둡니다.
	const APlayerController* PlayerController = GetOwningPlayer();
	const bool bCutscene = PlayerController && PlayerController->bCinematicMode;
	StatusPanel->SetVisibility(bStatusHidden || bCutscene ? ESlateVisibility::Collapsed : StatusPanelVisibility);

	// 잔상은 게임 시간으로 흘러 슬로우모션에 함께 느려집니다.
	// 엔진은 일시정지 중에도 GetDeltaSeconds에 프레임 시간을 넣으므로 멈춘 동안은 0으로 셉니다.
	const UWorld* World = GetWorld();
	const float GameDeltaTime = World->IsPaused() ? 0.f : World->GetDeltaSeconds();
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
	const AAgBossCharacter* BossCharacter = Boss.Get();
	const bool bBossInCombat =
		BossCharacter && !BossCharacter->GetAbilitySystemComponent()->HasMatchingGameplayTag(AgGameplayTags::State_NonCombat);
	BossStatusPanel->SetVisibility(bBossInCombat ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	if (bBossInCombat)
	{
		UpdateBoss(*BossCharacter, GameDeltaTime);
	}
}

void UAgHUDWidget::OpenPauseMenu()
{
	if (AAgPlayerController* PlayerController = GetOwningPlayer<AAgPlayerController>())
	{
		PlayerController->PushScreen(EAgUILayer::Menu, PauseMenuClass);
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

	const UAbilitySystemComponent* ASC = Player.GetAbilitySystemComponent();
	SetSlotUsable(*SkillSlot, CanPayCost(*ASC, AgGameplayTags::Ability_Action_Skill));
	SetSlotUsable(*UltimateSlot, CanPayCost(*ASC, AgGameplayTags::Ability_Action_Ultimate));
	UpdateCooldown(*ASC, AgGameplayTags::Ability_Action_Skill, *SkillCooldownSweep, *SkillCooldownText);

	if (UMaterialInstanceDynamic* Gauge = UPGauge->GetDynamicMaterial())
	{
		Gauge->SetScalarParameterValue(RatioParameter, Stats->GetMaxUP() > 0.f ? Stats->GetUP() / Stats->GetMaxUP() : 0.f);
	}

	FGameplayAbilitySpecHandle SkillHandle;
	const UAgGameplayAbility* Skill = Cast<UAgGameplayAbility>(FindAbility(*ASC, AgGameplayTags::Ability_Action_Skill, SkillHandle));
	const int32 SkillCost = Skill ? FMath::RoundToInt(Skill->GetCost().MP) : 0;
	if (SkillCost != ShownSkillCost)
	{
		ShownSkillCost = SkillCost;
		SkillCostText->SetText(FText::AsNumber(SkillCost));
	}
}

void UAgHUDWidget::UpdateBoss(const AAgBossCharacter& BossCharacter, float GameDeltaTime)
{
	// 에디터에서 데이터 파일을 다시 읽으면 이름이 바뀌므로 매 프레임 넣습니다. 같은 텍스트면 Slate가 다시 그리지 않습니다.
	BossNameText->SetText(BossCharacter.GetCharacterData()->DisplayName);

	const UAgAttributeSet* Stats = BossCharacter.GetAttributeSet();
	SetBarRatio(*BossHPBar, Stats->GetHP(), Stats->GetMaxHP());
	UpdateTrail(BossHPTrail, *BossHPTrailBar, Stats->GetMaxHP() > 0.f ? Stats->GetHP() / Stats->GetMaxHP() : 0.f, GameDeltaTime);

	const UAbilitySystemComponent* BossASC = BossCharacter.GetAbilitySystemComponent();
	if (BossASC->HasMatchingGameplayTag(AgGameplayTags::State_Execution))
	{
		return;
	}

	float PPBarRatio = Stats->GetMaxPP() > 0.f ? Stats->GetPP() / Stats->GetMaxPP() : 0.f;
	if (BossASC->HasMatchingGameplayTag(AgGameplayTags::State_Groggy))
	{
		const FGameplayEffectQuery Query =
			FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(AgGameplayTags::State_Groggy));
		const TArray<TPair<float, float>> Times = BossASC->GetActiveEffectsTimeRemainingAndDuration(Query);
		if (!Times.IsEmpty() && Times[0].Value > 0.f)
		{
			PPBarRatio = FMath::Clamp(1.f - Times[0].Key / Times[0].Value, 0.f, 1.f);
		}
	}
	BossPPBar->SetPercent(PPBarRatio);
	UpdateTrail(BossPPTrail, *BossPPTrailBar, PPBarRatio, GameDeltaTime);
}

const UGameplayAbility* UAgHUDWidget::FindAbility(const UAbilitySystemComponent& ASC, const FGameplayTag& AbilityTag, FGameplayAbilitySpecHandle& OutHandle)
{
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
	if (Trail.LastBarRatio >= 0.f && BarRatio < Trail.LastBarRatio)
	{
		if (bShowDrop)
		{
			Trail.Wait = TrailDelay;
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
		const float Alpha = TrailShrinkTime > 0.f ? FMath::Min(Trail.ShrinkElapsed / TrailShrinkTime, 1.f) : 1.f;
		Trail.Ratio = FMath::Lerp(Trail.ShrinkFrom, BarRatio, Alpha);
	}
	TrailBar.SetPercent(Trail.Ratio);
}

void UAgHUDWidget::HandlePlayerSPChanged(const FOnAttributeChangeData& Data)
{
	// 대미지 GE가 SP를 줄이는 것은 가드와 가드 브레이크뿐입니다.
	const UGameplayEffect* Effect = Data.GEModData ? Data.GEModData->EffectSpec.Def.Get() : nullptr;
	if (Data.NewValue < Data.OldValue && Effect && Effect->GetAssetTags().HasTagExact(AgGameplayTags::Effect_Damage))
	{
		bPlayerSPGuardDrop = true;
	}
}
