// Copyright Woogle. All Rights Reserved.

#include "UI/AgNameplateWidget.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Core/AgGameplayTags.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "TimerManager.h"
#include "UI/AgStatBar.h"

namespace AgNameplateWidgetPrivate
{
	TArray<FGameplayAttribute> StatusAttributes()
	{
		return {UAgAttributeSet::GetHPAttribute(), UAgAttributeSet::GetMaxHPAttribute(), UAgAttributeSet::GetPPAttribute(),
			UAgAttributeSet::GetMaxPPAttribute()};
	}

	const TArray<FGameplayTag>& StatusTags()
	{
		static const TArray<FGameplayTag> Tags = {AgGameplayTags::State_Groggy, AgGameplayTags::State_Execution};
		return Tags;
	}
} // namespace AgNameplateWidgetPrivate

void UAgNameplateWidget::SetAbilitySystem(UAbilitySystemComponent* NewASC)
{
	// 같은 대상을 다시 연결하면 남아 있는 대미지 잔상이 지워지므로 그대로 둡니다.
	if (ASC.Get() == NewASC)
	{
		return;
	}
	Unbind();
	ASC = NewASC;
	LastDropTime = -UE_DOUBLE_BIG_NUMBER;
	if (NewASC)
	{
		for (const FGameplayAttribute& Attribute : AgNameplateWidgetPrivate::StatusAttributes())
		{
			NewASC->GetGameplayAttributeValueChangeDelegate(Attribute).AddUObject(this, &ThisClass::HandleAttributeChanged);
		}
		for (const FGameplayTag& Tag : AgNameplateWidgetPrivate::StatusTags())
		{
			NewASC->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved)
				.AddWeakLambda(this, [this](const FGameplayTag, int32)
			{ UpdatePP(); });
		}
	}
	UpdateHP();
	UpdatePP();
	ClearTrails();
}

bool UAgNameplateWidget::Update(bool bAllowed, bool bRequested)
{
	const bool bRecentDrop = GetWorld()->GetTimeSeconds() - LastDropTime < RecentDropShowTime;
	const bool bShow = bAllowed && (bRequested || bRecentDrop);
	// 가려진 동안에도 바는 최신 값을 따라가므로, 다시 보일 때 그동안의 피해 잔상을 재생하지 않도록 지웁니다. 요청이 없어
	// 숨어 있던 것은 피해가 곧 표시를 요청하므로, 그 피해의 잔상은 남깁니다.
	if (bShow && !bWasAllowed)
	{
		ClearTrails();
	}
	bWasAllowed = bAllowed;
	return bShow;
}

void UAgNameplateWidget::Unbind()
{
	GetWorld()->GetTimerManager().ClearTimer(GroggyTimer);
	UAbilitySystemComponent* Target = ASC.Get();
	if (!Target)
	{
		return;
	}
	for (const FGameplayAttribute& Attribute : AgNameplateWidgetPrivate::StatusAttributes())
	{
		Target->GetGameplayAttributeValueChangeDelegate(Attribute).RemoveAll(this);
	}
	for (const FGameplayTag& Tag : AgNameplateWidgetPrivate::StatusTags())
	{
		Target->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved).RemoveAll(this);
	}
}

void UAgNameplateWidget::ClearTrails()
{
	HPBar->ClearTrail();
	const UAbilitySystemComponent* Target = ASC.Get();
	if (!Target || !Target->HasMatchingGameplayTag(AgGameplayTags::State_Execution))
	{
		PPBar->ClearTrail();
	}
}

void UAgNameplateWidget::HandleAttributeChanged(const FOnAttributeChangeData& Data)
{
	const bool bResource = Data.Attribute == UAgAttributeSet::GetHPAttribute() || Data.Attribute == UAgAttributeSet::GetPPAttribute();
	if (bResource && Data.NewValue < Data.OldValue)
	{
		LastDropTime = GetWorld()->GetTimeSeconds();
	}

	if (Data.Attribute == UAgAttributeSet::GetHPAttribute() || Data.Attribute == UAgAttributeSet::GetMaxHPAttribute())
	{
		UpdateHP();
	}
	else
	{
		UpdatePP();
	}
}

void UAgNameplateWidget::UpdateHP()
{
	const UAbilitySystemComponent* Target = ASC.Get();
	if (!Target)
	{
		return;
	}
	HPBar->SetValue(Target->GetNumericAttribute(UAgAttributeSet::GetHPAttribute()),
		Target->GetNumericAttribute(UAgAttributeSet::GetMaxHPAttribute()));
}

void UAgNameplateWidget::UpdatePP()
{
	GetWorld()->GetTimerManager().ClearTimer(GroggyTimer);

	const UAbilitySystemComponent* Target = ASC.Get();
	if (!Target)
	{
		return;
	}

	const float MaxPP = Target->GetNumericAttribute(UAgAttributeSet::GetMaxPPAttribute());
	PPSection->SetVisibility(MaxPP > 0.f ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	if (MaxPP <= 0.f || Target->HasMatchingGameplayTag(AgGameplayTags::State_Execution))
	{
		PPBar->HideGroggy();
		return;
	}

	if (!Target->HasMatchingGameplayTag(AgGameplayTags::State_Groggy))
	{
		PPBar->HideGroggy();
		PPBar->SetRatio(Target->GetNumericAttribute(UAgAttributeSet::GetPPAttribute()) / MaxPP);
		return;
	}

	float Elapsed = 0.f;
	float PPBarRatio = 1.f;
	const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(AgGameplayTags::State_Groggy));
	const TArray<TPair<float, float>> Times = Target->GetActiveEffectsTimeRemainingAndDuration(Query);
	if (!Times.IsEmpty() && Times[0].Value > 0.f)
	{
		Elapsed = Times[0].Value - Times[0].Key;
		PPBarRatio = FMath::Clamp(Times[0].Key / Times[0].Value, 0.f, 1.f);
	}
	// UI 머티리얼의 Time은 실제 시간이라 일시정지·궁극기 시간 정지 중에도 흐르므로, 그로기 경과 시간을 넘깁니다.
	PPBar->ShowGroggy(Elapsed);
	PPBar->SetRatio(PPBarRatio);
	// 남은 시간이 줄어드는 것은 피해가 아니므로, 매 프레임의 감소가 잔상을 남기지 않게 합니다.
	PPBar->ClearTrail();
	// 그로기 동안은 남은 시간으로 PP 바를 줄이므로 프레임마다 그립니다.
	GroggyTimer = GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::UpdatePP);
}
