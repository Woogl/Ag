// Copyright Woogle. All Rights Reserved.

#include "UI/AgHUDWidget.h"

#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Character/AgBossCharacter.h"
#include "Character/AgPlayerCharacter.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Core/AgAssetManager.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "Engine/World.h"
#include "Game/AgGameState.h"
#include "Game/AgPlayerController.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "Input/CommonUIInputTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "UI/AgPauseMenu.h"
#include "UI/AgStatBar.h"
#include "UI/AgUILayout.h"

namespace
{
	const FName RatioParameter(TEXT("Ratio"));

	TArray<FGameplayAttribute> PlayerAttributes()
	{
		return {UAgAttributeSet::GetHPAttribute(), UAgAttributeSet::GetMaxHPAttribute(), UAgAttributeSet::GetMPAttribute(),
			UAgAttributeSet::GetMaxMPAttribute(), UAgAttributeSet::GetSPAttribute(), UAgAttributeSet::GetMaxSPAttribute(),
			UAgAttributeSet::GetUPAttribute(), UAgAttributeSet::GetMaxUPAttribute()};
	}

	TArray<FGameplayAttribute> BossAttributes()
	{
		return {UAgAttributeSet::GetHPAttribute(), UAgAttributeSet::GetMaxHPAttribute(), UAgAttributeSet::GetPPAttribute(),
			UAgAttributeSet::GetMaxPPAttribute()};
	}

	const TArray<FGameplayTag>& PlayerTags()
	{
		static const TArray<FGameplayTag> Tags = {AgGameplayTags::State_Cutscene};
		return Tags;
	}

	const TArray<FGameplayTag>& BossTags()
	{
		static const TArray<FGameplayTag> Tags = {AgGameplayTags::State_NonCombat, AgGameplayTags::State_Groggy,
			AgGameplayTags::State_Execution};
		return Tags;
	}

	void Unbind(UAbilitySystemComponent* ASC, const UObject* User, const TArray<FGameplayAttribute>& Attributes, const TArray<FGameplayTag>& Tags)
	{
		if (!ASC)
		{
			return;
		}
		for (const FGameplayAttribute& Attribute : Attributes)
		{
			ASC->GetGameplayAttributeValueChangeDelegate(Attribute).RemoveAll(User);
		}
		for (const FGameplayTag& Tag : Tags)
		{
			ASC->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved).RemoveAll(User);
		}
		ASC->AbilitySpecDirtiedCallbacks.RemoveAll(User);
		ASC->AbilityCommittedCallbacks.RemoveAll(User);
	}
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
		if (UAgAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent())
		{
			PlayerASC = ASC;
			BindPlayer(*ASC);
		}
	}

#if WITH_EDITOR
	// 데이터 파일을 다시 읽으면 보스 이름과 스킬 코스트가 바뀝니다.
	UAgAssetManager::Get().OnJsonDataReloaded.AddWeakLambda(this, [this]
	{
		UpdateSkillSlots();
		UpdateBoss();
	});
#endif

	if (AAgGameState* StageState = GetWorld()->GetGameState<AAgGameState>())
	{
		StageState->OnBossChanged.AddUObject(this, &ThisClass::HandleBossChanged);
		StageState->OnStageResultChanged.AddUObject(this, &ThisClass::UpdateStatusPanel);
	}
	HandleBossChanged();

	UpdateStatusPanel();
	UpdatePlayer();
	UpdateSkillCooldown();
}

void UAgHUDWidget::NativeDestruct()
{
	Unbind(PlayerASC.Get(), this, PlayerAttributes(), PlayerTags());
	PlayerASC.Reset();
	if (const AAgBossCharacter* BossCharacter = Boss.Get())
	{
		Unbind(BossCharacter->GetAbilitySystemComponent(), this, BossAttributes(), BossTags());
	}
	Boss.Reset();
#if WITH_EDITOR
	UAgAssetManager::Get().OnJsonDataReloaded.RemoveAll(this);
#endif
	if (const UWorld* World = GetWorld())
	{
		if (AAgGameState* StageState = World->GetGameState<AAgGameState>())
		{
			StageState->OnBossChanged.RemoveAll(this);
			StageState->OnStageResultChanged.RemoveAll(this);
		}
		World->GetTimerManager().ClearTimer(CooldownTimer);
		World->GetTimerManager().ClearTimer(BossTimer);
	}

	Super::NativeDestruct();
}

TOptional<FUIInputConfig> UAgHUDWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Game, EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown,
		true);
}

void UAgHUDWidget::OpenPauseMenu()
{
	if (AAgPlayerController* PlayerController = GetOwningPlayer<AAgPlayerController>())
	{
		PlayerController->PushScreen(EAgUILayer::Menu, PauseMenuClass);
	}
}

void UAgHUDWidget::HandleBossChanged()
{
	if (const AAgBossCharacter* OldBoss = Boss.Get())
	{
		Unbind(OldBoss->GetAbilitySystemComponent(), this, BossAttributes(), BossTags());
	}
	const AAgGameState* StageState = GetWorld()->GetGameState<AAgGameState>();
	Boss = StageState ? StageState->GetBoss() : nullptr;
	if (const AAgBossCharacter* NewBoss = Boss.Get())
	{
		BindBoss(*NewBoss->GetAbilitySystemComponent());
	}
	UpdateBoss();
}

void UAgHUDWidget::BindPlayer(UAbilitySystemComponent& ASC)
{
	for (const FGameplayAttribute& Attribute : PlayerAttributes())
	{
		ASC.GetGameplayAttributeValueChangeDelegate(Attribute).AddUObject(this, &ThisClass::HandlePlayerAttributeChanged);
	}
	for (const FGameplayTag& Tag : PlayerTags())
	{
		ASC.RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved)
			.AddWeakLambda(this, [this](const FGameplayTag, int32) { UpdateStatusPanel(); });
	}
	// 캐릭터는 HUD가 생긴 뒤 BeginPlay에서 어빌리티를 부여할 수 있으므로, 부여될 때 스킬 슬롯을 다시 그립니다.
	ASC.AbilitySpecDirtiedCallbacks.AddWeakLambda(this, [this](const FGameplayAbilitySpec&) { UpdateSkillSlots(); });
	ASC.AbilityCommittedCallbacks.AddWeakLambda(this, [this](UGameplayAbility*) { UpdateSkillCooldown(); });
}

void UAgHUDWidget::BindBoss(UAbilitySystemComponent& ASC)
{
	for (const FGameplayAttribute& Attribute : BossAttributes())
	{
		ASC.GetGameplayAttributeValueChangeDelegate(Attribute).AddWeakLambda(this, [this](const FOnAttributeChangeData&) { UpdateBoss(); });
	}
	for (const FGameplayTag& Tag : BossTags())
	{
		ASC.RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved)
			.AddWeakLambda(this, [this](const FGameplayTag, int32) { UpdateBoss(); });
	}
}

void UAgHUDWidget::UpdateStatusPanel()
{
	const UAbilitySystemComponent* ASC = PlayerASC.Get();
	const AAgGameState* StageState = GetWorld()->GetGameState<AAgGameState>();
	const bool bCutscene = ASC && ASC->HasMatchingGameplayTag(AgGameplayTags::State_Cutscene);
	const bool bStageEnded = StageState && StageState->GetStageResult() != EAgStageResult::None;
	StatusPanel->SetVisibility(bCutscene || bStageEnded ? ESlateVisibility::Collapsed : StatusPanelVisibility);
}

void UAgHUDWidget::UpdatePlayer()
{
	if (const UAbilitySystemComponent* ASC = PlayerASC.Get())
	{
		SetBarValue(*PlayerHPBar, *ASC, UAgAttributeSet::GetHPAttribute(), UAgAttributeSet::GetMaxHPAttribute());
		SetBarValue(*PlayerMPBar, *ASC, UAgAttributeSet::GetMPAttribute(), UAgAttributeSet::GetMaxMPAttribute());
		SetBarValue(*PlayerSPBar, *ASC, UAgAttributeSet::GetSPAttribute(), UAgAttributeSet::GetMaxSPAttribute(), false);
	}
	UpdateSkillSlots();
}

void UAgHUDWidget::UpdateSkillSlots()
{
	const UAgAbilitySystemComponent* ASC = PlayerASC.Get();
	if (!ASC)
	{
		return;
	}

	SetSlotUsable(*SkillSlot, CanPayCost(*ASC, AgGameplayTags::Ability_Action_Skill));
	SetSlotUsable(*UltimateSlot, CanPayCost(*ASC, AgGameplayTags::Ability_Action_Ultimate));

	if (UMaterialInstanceDynamic* Gauge = UPGauge->GetDynamicMaterial())
	{
		const float MaxUP = ASC->GetNumericAttribute(UAgAttributeSet::GetMaxUPAttribute());
		Gauge->SetScalarParameterValue(RatioParameter, MaxUP > 0.f ? ASC->GetNumericAttribute(UAgAttributeSet::GetUPAttribute()) / MaxUP : 0.f);
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

void UAgHUDWidget::UpdateSkillCooldown()
{
	FTimerManager& TimerManager = GetWorld()->GetTimerManager();
	TimerManager.ClearTimer(CooldownTimer);

	float Remaining = 0.f;
	float Duration = 0.f;
	FGameplayAbilitySpecHandle Handle;
	const UAgAbilitySystemComponent* ASC = PlayerASC.Get();
	if (const UGameplayAbility* Ability = ASC ? FindAbility(*ASC, AgGameplayTags::Ability_Action_Skill, Handle) : nullptr)
	{
		Ability->GetCooldownTimeRemainingAndDuration(Handle, ASC->AbilityActorInfo.Get(), Remaining, Duration);
	}
	const bool bCoolingDown = Remaining > 0.f && Duration > 0.f;
	const ESlateVisibility CooldownVisibility = bCoolingDown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	SkillCooldownSweep->SetVisibility(CooldownVisibility);
	SkillCooldownText->SetVisibility(CooldownVisibility);
	if (!bCoolingDown)
	{
		return;
	}

	if (UMaterialInstanceDynamic* Material = SkillCooldownSweep->GetDynamicMaterial())
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
		SkillCooldownText->SetText(bWholeSeconds ? FText::AsNumber(Shown.Y) : FText::AsNumber(Shown.Y / 10.f, &OneDecimal));
	}
	CooldownTimer = TimerManager.SetTimerForNextTick(this, &ThisClass::UpdateSkillCooldown);
}

void UAgHUDWidget::UpdateBoss()
{
	GetWorld()->GetTimerManager().ClearTimer(BossTimer);

	const AAgBossCharacter* BossCharacter = Boss.Get();
	const UAbilitySystemComponent* BossASC = BossCharacter ? BossCharacter->GetAbilitySystemComponent() : nullptr;
	const bool bBossInCombat = BossASC && !BossASC->HasMatchingGameplayTag(AgGameplayTags::State_NonCombat);
	BossStatusPanel->SetVisibility(bBossInCombat ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	if (!bBossInCombat)
	{
		return;
	}

	BossNameText->SetText(BossCharacter->GetCharacterData()->DisplayName);
	SetBarValue(*BossHPBar, *BossASC, UAgAttributeSet::GetHPAttribute(), UAgAttributeSet::GetMaxHPAttribute());

	if (BossASC->HasMatchingGameplayTag(AgGameplayTags::State_Execution))
	{
		return;
	}

	const float MaxPP = BossASC->GetNumericAttribute(UAgAttributeSet::GetMaxPPAttribute());
	float PPBarRatio = MaxPP > 0.f ? BossASC->GetNumericAttribute(UAgAttributeSet::GetPPAttribute()) / MaxPP : 0.f;
	if (BossASC->HasMatchingGameplayTag(AgGameplayTags::State_Groggy))
	{
		const FGameplayEffectQuery Query =
			FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(AgGameplayTags::State_Groggy));
		const TArray<TPair<float, float>> Times = BossASC->GetActiveEffectsTimeRemainingAndDuration(Query);
		if (!Times.IsEmpty() && Times[0].Value > 0.f)
		{
			PPBarRatio = FMath::Clamp(1.f - Times[0].Key / Times[0].Value, 0.f, 1.f);
		}
		// 그로기 동안은 남은 시간으로 PP 바를 다시 채우므로 프레임마다 그립니다.
		BossTimer = GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::UpdateBoss);
	}
	BossPPBar->SetRatio(PPBarRatio);
}

void UAgHUDWidget::SetBarValue(UAgStatBar& Bar, const UAbilitySystemComponent& ASC, const FGameplayAttribute& Value, const FGameplayAttribute& MaxValue, bool bShowDrop)
{
	Bar.SetValue(ASC.GetNumericAttribute(Value), ASC.GetNumericAttribute(MaxValue), bShowDrop);
}

const UGameplayAbility* UAgHUDWidget::FindAbility(const UAgAbilitySystemComponent& ASC, const FGameplayTag& AbilityTag, FGameplayAbilitySpecHandle& OutHandle)
{
	const FGameplayAbilitySpec* Spec = ASC.FindSpecByTag(AbilityTag);
	if (!Spec || !Spec->GetPrimaryInstance())
	{
		return nullptr;
	}
	OutHandle = Spec->Handle;
	return Spec->GetPrimaryInstance();
}

bool UAgHUDWidget::CanPayCost(const UAgAbilitySystemComponent& ASC, const FGameplayTag& AbilityTag)
{
	FGameplayAbilitySpecHandle Handle;
	const UGameplayAbility* Ability = FindAbility(ASC, AbilityTag, Handle);
	return Ability && Ability->CheckCost(Handle, ASC.AbilityActorInfo.Get());
}

void UAgHUDWidget::SetSlotUsable(UWidget& SlotWidget, bool bUsable) const
{
	SlotWidget.SetRenderOpacity(bUsable ? 1.f : DisabledSlotOpacity);
}

void UAgHUDWidget::HandlePlayerAttributeChanged(const FOnAttributeChangeData& Data)
{
	const UAbilitySystemComponent* ASC = PlayerASC.Get();
	if (!ASC)
	{
		return;
	}

	// 한 효과가 여러 어트리뷰트를 바꿀 때 알림 순서와 상관없도록, 바뀐 어트리뷰트의 표시만 다시 그립니다.
	const FGameplayAttribute& Attribute = Data.Attribute;
	if (Attribute == UAgAttributeSet::GetHPAttribute() || Attribute == UAgAttributeSet::GetMaxHPAttribute())
	{
		SetBarValue(*PlayerHPBar, *ASC, UAgAttributeSet::GetHPAttribute(), UAgAttributeSet::GetMaxHPAttribute());
	}
	else if (Attribute == UAgAttributeSet::GetSPAttribute() || Attribute == UAgAttributeSet::GetMaxSPAttribute())
	{
		// 대미지 GE가 SP를 줄이는 것은 가드와 가드 브레이크뿐입니다.
		const UGameplayEffect* Effect = Data.GEModData ? Data.GEModData->EffectSpec.Def.Get() : nullptr;
		const bool bGuardDrop = Data.NewValue < Data.OldValue && Effect && Effect->GetAssetTags().HasTagExact(AgGameplayTags::Effect_Damage);
		SetBarValue(*PlayerSPBar, *ASC, UAgAttributeSet::GetSPAttribute(), UAgAttributeSet::GetMaxSPAttribute(), bGuardDrop);
	}
	else
	{
		if (Attribute == UAgAttributeSet::GetMPAttribute() || Attribute == UAgAttributeSet::GetMaxMPAttribute())
		{
			SetBarValue(*PlayerMPBar, *ASC, UAgAttributeSet::GetMPAttribute(), UAgAttributeSet::GetMaxMPAttribute());
		}
		UpdateSkillSlots();
	}
}
