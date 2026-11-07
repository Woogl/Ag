// Copyright Woogle. All Rights Reserved.

#include "UI/AgSkillSlot.h"

#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "Character/AgPlayerCharacter.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Data/AgAbilityData.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"

namespace
{
	const FName RatioParameter(TEXT("Ratio"));
}

void UAgSkillSlot::SetInput(UAgAbilitySystemComponent* InASC, const FGameplayTag& InInputTag)
{
	ASC = InASC;
	InputTag = InInputTag;
	UpdateAbility();
	UpdateCooldown();
}

void UAgSkillSlot::UpdateAbility()
{
	FGameplayAbilitySpecHandle Handle;
	const UGameplayAbility* Ability = FindAbility(Handle);
	const AAgPlayerCharacter* Player = Ability ? Cast<AAgPlayerCharacter>(ASC->GetAvatarActor()) : nullptr;
	const FAgAbilityData* Data = Player ? Player->FindAbility(Handle) : nullptr;

	UTexture2D* Icon = Data ? Data->Icon.Get() : nullptr;
	if (IconImage->GetBrush().GetResourceObject() != Icon)
	{
		IconImage->SetBrushFromTexture(Icon);
	}
	// 이미지가 없는 아이콘은 빈 사각형으로 그려지므로 숨깁니다.
	IconImage->SetVisibility(Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	const int32 Cost = Data ? FMath::RoundToInt(Data->Cost.MP) : 0;
	if (Cost != ShownCost)
	{
		ShownCost = Cost;
		CostText->SetText(FText::AsNumber(Cost));
		CostText->SetVisibility(Cost > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	const bool bUsable = Ability && Ability->CheckCost(Handle, ASC->AbilityActorInfo.Get());
	Body->SetRenderOpacity(bUsable ? 1.f : DisabledOpacity);
}

void UAgSkillSlot::UpdateCooldown()
{
	FTimerManager& TimerManager = GetWorld()->GetTimerManager();
	TimerManager.ClearTimer(CooldownTimer);

	float Remaining = 0.f;
	float Duration = 0.f;
	FGameplayAbilitySpecHandle Handle;
	if (const UGameplayAbility* Ability = FindAbility(Handle))
	{
		Ability->GetCooldownTimeRemainingAndDuration(Handle, ASC->AbilityActorInfo.Get(), Remaining, Duration);
	}
	const bool bCoolingDown = Remaining > 0.f && Duration > 0.f;
	const ESlateVisibility CooldownVisibility = bCoolingDown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	CooldownSweep->SetVisibility(CooldownVisibility);
	CooldownText->SetVisibility(CooldownVisibility);
	if (!bCoolingDown)
	{
		return;
	}

	if (UMaterialInstanceDynamic* Material = CooldownSweep->GetDynamicMaterial())
	{
		Material->SetScalarParameterValue(RatioParameter, Remaining / Duration);
	}

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
	CooldownTimer = TimerManager.SetTimerForNextTick(this, &ThisClass::UpdateCooldown);
}

void UAgSkillSlot::NativeDestruct()
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CooldownTimer);
	}

	Super::NativeDestruct();
}

const UGameplayAbility* UAgSkillSlot::FindAbility(FGameplayAbilitySpecHandle& OutHandle) const
{
	const UAgAbilitySystemComponent* AbilitySystem = ASC.Get();
	const AAgPlayerCharacter* Player = AbilitySystem ? Cast<AAgPlayerCharacter>(AbilitySystem->GetAvatarActor()) : nullptr;
	TArray<FGameplayAbilitySpecHandle> Handles;
	if (Player)
	{
		Player->FindInputAbilities(InputTag, Handles);
	}
	const FGameplayAbilitySpec* Spec = Handles.IsEmpty() ? nullptr : AbilitySystem->FindAbilitySpecFromHandle(Handles[0]);
	if (!Spec || !Spec->GetPrimaryInstance())
	{
		return nullptr;
	}
	OutHandle = Spec->Handle;
	return Spec->GetPrimaryInstance();
}
