// Copyright Woogle. All Rights Reserved.

#include "UI/AgHUDWidget.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Character/AgBossCharacter.h"
#include "Character/AgPlayerCharacter.h"
#include "Combat/AgCombatLibrary.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "Data/AgCombatRules.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	/** Scalar parameter of the UP gauge material: the filled share of the border, 0 to 1. */
	const FName UPRatioParameter(TEXT("Ratio"));
}

void UAgHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (BossPPBar)
	{
		PPColor = BossPPBar->GetFillColorAndOpacity();
	}

	// An icon whose brush has no image would draw a plain box, so it stays hidden until one is put in.
	for (UImage* Icon : { SkillIconImage.Get(), UltimateIconImage.Get() })
	{
		if (Icon && !Icon->GetBrush().GetResourceObject())
		{
			Icon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UAgHUDWidget::HideStatus()
{
	if (StatusPanel)
	{
		StatusPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

TOptional<FUIInputConfig> UAgHUDWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Game, EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown, /*bHideCursorDuringViewportCapture*/ true);
}

void UAgHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// The trails run on game time, so the slow motion slows them too.
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
			const UAgBossData* Data = Cast<UAgBossData>(It->GetCharacterData());
			if (BossNameText && Data)
			{
				BossNameText->SetText(Data->DisplayName);
			}
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
	SetBarRatio(PlayerHPBar, Stats->GetHP(), Stats->GetMaxHP());
	SetBarRatio(PlayerMPBar, Stats->GetMP(), Stats->GetMaxMP());
	SetBarRatio(PlayerSPBar, Stats->GetSP(), Stats->GetMaxSP());
	SetValueText(PlayerHPText, Stats->GetHP(), Stats->GetMaxHP(), ShownHP);
	SetValueText(PlayerMPText, Stats->GetMP(), Stats->GetMaxMP(), ShownMP);
	UpdateTrail(PlayerTrail, PlayerHPTrailBar, Stats->GetMaxHP() > 0.f ? Stats->GetHP() / Stats->GetMaxHP() : 0.f, GameDeltaTime);

	const UAbilitySystemComponent* ASC = Player.GetAbilitySystemComponent();
	SetSlotUsable(SkillSlot, CanUseAbility(*ASC, AgGameplayTags::Ability_Action_Skill));
	SetSlotUsable(UltimateSlot, CanUseAbility(*ASC, AgGameplayTags::Ability_Action_Ultimate));

	// The material fills the border from the top center clockwise by this ratio.
	if (UltimateGauge)
	{
		if (UMaterialInstanceDynamic* Gauge = UltimateGauge->GetDynamicMaterial())
		{
			Gauge->SetScalarParameterValue(UPRatioParameter, Stats->GetMaxUP() > 0.f ? Stats->GetUP() / Stats->GetMaxUP() : 0.f);
		}
	}

	const UAgPlayerData* Data = Cast<UAgPlayerData>(Player.GetCharacterData());
	const int32 SkillCost = Data ? FMath::RoundToInt(Data->SkillMPCost) : 0;
	if (SkillCostText && SkillCost != ShownSkillCost)
	{
		ShownSkillCost = SkillCost;
		SkillCostText->SetText(FText::AsNumber(SkillCost));
	}
}

void UAgHUDWidget::UpdateBoss(const AAgBossCharacter& BossCharacter, float GameDeltaTime)
{
	const UAgAttributeSet* Stats = BossCharacter.GetAttributeSet();
	SetBarRatio(BossHPBar, Stats->GetHP(), Stats->GetMaxHP());
	UpdateTrail(BossTrail, BossHPTrailBar, Stats->GetMaxHP() > 0.f ? Stats->GetHP() / Stats->GetMaxHP() : 0.f, GameDeltaTime);

	if (BossPPBar)
	{
		// 그로기: another color, refilling from empty over the groggy time. When an execution makes the groggy last
		// longer, the bar stays full in that color until the groggy ends.
		const bool bGroggy = BossCharacter.GetAbilitySystemComponent()->HasMatchingGameplayTag(AgGameplayTags::State_Groggy);
		if (bGroggy)
		{
			const UAgCombatRules* Rules = UAgCombatLibrary::GetCombatRules();
			const float Elapsed = BossCharacter.GetGroggyElapsedTime();
			const bool bCounting = Rules && Rules->GroggyDuration > 0.f && Elapsed >= 0.f;
			BossPPBar->SetPercent(bCounting ? FMath::Min(Elapsed / Rules->GroggyDuration, 1.f) : 0.f);
		}
		else
		{
			SetBarRatio(BossPPBar, Stats->GetPP(), Stats->GetMaxPP());
		}
		BossPPBar->SetFillColorAndOpacity(bGroggy ? GroggyPPColor : PPColor);
	}
}

bool UAgHUDWidget::CanUseAbility(const UAbilitySystemComponent& ASC, const FGameplayTag& AbilityTag)
{
	for (const FGameplayAbilitySpec& Spec : ASC.GetActivatableAbilities())
	{
		const UGameplayAbility* Ability = Spec.GetPrimaryInstance();
		if (Ability && Ability->GetAssetTags().HasTagExact(AbilityTag))
		{
			const FGameplayAbilityActorInfo* ActorInfo = ASC.AbilityActorInfo.Get();
			return Ability->CheckCost(Spec.Handle, ActorInfo) && Ability->CheckCooldown(Spec.Handle, ActorInfo);
		}
	}
	return false;
}

void UAgHUDWidget::SetBarRatio(UProgressBar* Bar, float Value, float MaxValue)
{
	if (Bar)
	{
		Bar->SetPercent(MaxValue > 0.f ? Value / MaxValue : 0.f);
	}
}

void UAgHUDWidget::SetSlotUsable(UWidget* SlotWidget, bool bUsable) const
{
	if (SlotWidget)
	{
		SlotWidget->SetRenderOpacity(bUsable ? 1.f : DisabledSlotOpacity);
	}
}

void UAgHUDWidget::SetValueText(UTextBlock* Text, float Value, float MaxValue, FIntPoint& ShownValues)
{
	const FIntPoint Values(FMath::RoundToInt(Value), FMath::RoundToInt(MaxValue));
	if (Text && Values != ShownValues)
	{
		ShownValues = Values;
		const FNumberFormattingOptions& Format = FNumberFormattingOptions::DefaultNoGrouping();
		Text->SetText(FText::Format(INVTEXT("{0} / {1}"), FText::AsNumber(Values.X, &Format), FText::AsNumber(Values.Y, &Format)));
	}
}

void UAgHUDWidget::UpdateTrail(FHPTrail& Trail, UProgressBar* TrailBar, float HPRatio, float DeltaTime) const
{
	if (!TrailBar)
	{
		return;
	}

	if (Trail.LastHPRatio >= 0.f && HPRatio < Trail.LastHPRatio)
	{
		Trail.Wait = TrailDelay;
		Trail.ShrinkElapsed = -1.f;
	}
	Trail.LastHPRatio = HPRatio;

	if (Trail.Ratio <= HPRatio)
	{
		Trail.Ratio = HPRatio;
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
		Trail.Ratio = FMath::Lerp(Trail.ShrinkFrom, HPRatio, Alpha);
	}
	TrailBar->SetPercent(Trail.Ratio);
}
