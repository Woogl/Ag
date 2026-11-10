// Copyright Woogle. All Rights Reserved.

#include "UI/AgSkillSlot.h"

#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "Character/AgPlayerCharacter.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Data/AgAbilityData.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"

namespace AgSkillSlotPrivate
{
	const FName RatioParameter(TEXT("Ratio"));
}

void UAgSkillSlot::SetInput(AAgPlayerCharacter* InPlayer, const FGameplayTag& InInputTag)
{
	Player = InPlayer;
	InputTag = InInputTag;
	UpdateAbility();
	UpdateCooldown();
}

void UAgSkillSlot::UpdateAbility()
{
	// 데이터를 다시 읽으면 아이콘과 코스트가 바뀌므로, 어빌리티의 사본이 아니라 캐릭터 데이터의 지금 정의를 봅니다.
	const UAgGameplayAbility* Ability = FindAbility();
	const FAgAbilityData* Data = Ability ? Player->FindAbility(Ability->GetCurrentAbilitySpecHandle()) : nullptr;

	UTexture2D* Icon = Data ? Data->Icon.Get() : nullptr;
	IconImage->SetBrushFromTexture(Icon);
	// 이미지가 없는 아이콘은 빈 사각형으로 그려지므로 숨깁니다.
	IconImage->SetVisibility(Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	const int32 Cost = Data ? FMath::RoundToInt(Data->Cost.MP) : 0;
	if (Cost != ShownCost)
	{
		ShownCost = Cost;
		CostText->SetText(FText::AsNumber(Cost));
		CostText->SetVisibility(Cost > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	const bool bUsable = Ability && Ability->CheckCost(Ability->GetCurrentAbilitySpecHandle(), Ability->GetCurrentActorInfo());
	Body->SetRenderOpacity(bUsable ? 1.f : DisabledOpacity);
}

void UAgSkillSlot::UpdateCooldown()
{
	FTimerManager& TimerManager = GetWorld()->GetTimerManager();
	TimerManager.ClearTimer(CooldownTimer);

	float Remaining = 0.f;
	float Duration = 0.f;
	int32 Charges = 0;
	int32 MaxCharges = 0;
	if (const UAgGameplayAbility* Ability = FindAbility())
	{
		Ability->GetRechargeTimeRemainingAndDuration(Remaining, Duration);
		Charges = Ability->GetCharges();
		const FAgAbilityData* Data = Player->FindAbility(Ability->GetCurrentAbilitySpecHandle());
		MaxCharges = Data ? Data->MaxCharges : 0;
	}

	const bool bShowsCharges = MaxCharges >= 2;
	ChargeText->SetVisibility(bShowsCharges ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (bShowsCharges && Charges != ShownCharges)
	{
		ShownCharges = Charges;
		ChargeText->SetText(FText::AsNumber(Charges));
	}

	const bool bRecharging = Remaining > 0.f && Duration > 0.f;
	const bool bCoolingDown = bRecharging && Charges == 0;
	CooldownSweep->SetVisibility(bRecharging ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	CooldownText->SetVisibility(bCoolingDown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (bRecharging)
	{
		if (UMaterialInstanceDynamic* Material = CooldownSweep->GetDynamicMaterial())
		{
			Material->SetScalarParameterValue(AgSkillSlotPrivate::RatioParameter, Remaining / Duration);
		}
	}
	if (bCoolingDown)
	{
		const bool bWholeSeconds = Remaining >= 1.f;
		const FIntPoint Shown(bWholeSeconds ? 1 : 0, FMath::CeilToInt(bWholeSeconds ? Remaining : Remaining * 10.f));
		if (Shown != ShownCooldown)
		{
			ShownCooldown = Shown;
			FNumberFormattingOptions OneDecimal;
			OneDecimal.MinimumFractionalDigits = 1;
			OneDecimal.MaximumFractionalDigits = 1;
			CooldownText->SetText(bWholeSeconds ? FText::AsNumber(Shown.Y) : FText::AsNumber(Shown.Y / 10.f, &OneDecimal));
		}
	}
	if (Charges < MaxCharges)
	{
		CooldownTimer = TimerManager.SetTimerForNextTick(this, &ThisClass::UpdateCooldown);
	}
}

void UAgSkillSlot::NativeDestruct()
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CooldownTimer);
	}

	Super::NativeDestruct();
}

const UAgGameplayAbility* UAgSkillSlot::FindAbility() const
{
	const AAgPlayerCharacter* Character = Player.Get();
	TArray<FGameplayAbilitySpecHandle> Handles;
	if (Character)
	{
		Character->FindInputAbilities(InputTag, Handles);
	}
	const FGameplayAbilitySpec* Spec = Handles.IsEmpty() ? nullptr : Character->GetAbilitySystemComponent()->FindAbilitySpecFromHandle(Handles[0]);
	return Spec ? Cast<UAgGameplayAbility>(Spec->GetPrimaryInstance()) : nullptr;
}
