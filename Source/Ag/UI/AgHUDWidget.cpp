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
}

TOptional<FUIInputConfig> UAgHUDWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Game, EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown, /*bHideCursorDuringViewportCapture*/ true);
}

void UAgHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (const AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(GetOwningPlayerPawn()))
	{
		UpdatePlayer(*Player);
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
		UpdateBoss(*BossCharacter);
	}
}

void UAgHUDWidget::UpdatePlayer(const AAgPlayerCharacter& Player)
{
	const UAgAttributeSet* Stats = Player.GetAttributeSet();
	SetBarRatio(PlayerHPBar, Stats->GetHP(), Stats->GetMaxHP());
	SetBarRatio(PlayerMPBar, Stats->GetMP(), Stats->GetMaxMP());
	SetBarRatio(PlayerSPBar, Stats->GetSP(), Stats->GetMaxSP());
	if (PlayerSPBar)
	{
		// Hidden keeps its place, so the HP and MP bars never move.
		PlayerSPBar->SetVisibility(Stats->GetSP() < Stats->GetMaxSP() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}

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

void UAgHUDWidget::UpdateBoss(const AAgBossCharacter& BossCharacter)
{
	const UAgAttributeSet* Stats = BossCharacter.GetAttributeSet();
	SetBarRatio(BossHPBar, Stats->GetHP(), Stats->GetMaxHP());

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
