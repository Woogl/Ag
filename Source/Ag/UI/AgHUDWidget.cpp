// Copyright Woogle. All Rights Reserved.

#include "UI/AgHUDWidget.h"

#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Character/AgBossCharacter.h"
#include "Character/AgPlayerCharacter.h"
#include "Combat/AgCombatSubsystem.h"
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
#include "UI/AgNameplateWidget.h"
#include "UI/AgPauseMenu.h"
#include "UI/AgSkillSlot.h"
#include "UI/AgStatBar.h"
#include "UI/AgUILayout.h"

namespace AgHUDWidgetPrivate
{
	const FName RatioParameter(TEXT("Ratio"));

	TArray<FGameplayAttribute> PlayerAttributes()
	{
		return {UAgAttributeSet::GetHPAttribute(), UAgAttributeSet::GetMaxHPAttribute(), UAgAttributeSet::GetMPAttribute(),
			UAgAttributeSet::GetMaxMPAttribute(), UAgAttributeSet::GetUPAttribute(), UAgAttributeSet::GetMaxUPAttribute(),
			UAgAttributeSet::GetPotionAttribute(), UAgAttributeSet::GetMaxPotionAttribute()};
	}

} // namespace AgHUDWidgetPrivate

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

	AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(GetOwningPlayerPawn());
	if (UAgAbilitySystemComponent* ASC = Player ? Player->GetAbilitySystemComponent() : nullptr)
	{
		PlayerASC = ASC;
		BindPlayer(*ASC);
	}
	for (const TPair<UAgSkillSlot*, FGameplayTag>& SkillSlot : GetSkillSlots())
	{
		SkillSlot.Key->SetInput(Player, SkillSlot.Value);
	}

#if WITH_EDITOR
	// 데이터 파일을 다시 읽으면 보스 이름과 스킬 아이콘·코스트·최대 충전이 바뀝니다.
	UAgAssetManager::Get().OnJsonDataReloaded.AddWeakLambda(this, [this]
	{
		UpdateSkillSlots();
		UpdateCooldowns();
		UpdateBoss();
	});
#endif

	GetWorld()->GetSubsystem<UAgCombatSubsystem>()->OnCombatantsChanged.AddUObject(this, &ThisClass::HandleCombatantsChanged);
	if (AAgGameState* StageState = GetWorld()->GetGameState<AAgGameState>())
	{
		StageState->OnStageResultChanged.AddUObject(this, &ThisClass::UpdateStatusPanel);
	}
	HandleCombatantsChanged();

	UpdateStatusPanel();
	UpdatePlayer();
}

void UAgHUDWidget::NativeDestruct()
{
	UnbindPlayer();
	PlayerASC.Reset();
#if WITH_EDITOR
	UAgAssetManager::Get().OnJsonDataReloaded.RemoveAll(this);
#endif
	if (const UWorld* World = GetWorld())
	{
		if (UAgCombatSubsystem* CombatSubsystem = World->GetSubsystem<UAgCombatSubsystem>())
		{
			CombatSubsystem->OnCombatantsChanged.RemoveAll(this);
		}
		if (AAgGameState* StageState = World->GetGameState<AAgGameState>())
		{
			StageState->OnStageResultChanged.RemoveAll(this);
		}
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

void UAgHUDWidget::HandleCombatantsChanged()
{
	// 같은 보스를 다시 넘기면 보스 상태가 그대로 두므로, 다른 적의 전투 진입으로 대미지 잔상이 지워지지 않습니다.
	const AAgBossCharacter* Boss = GetWorld()->GetSubsystem<UAgCombatSubsystem>()->FindBoss();
	BossStatus->SetAbilitySystem(Boss ? Boss->GetAbilitySystemComponent() : nullptr);
	UpdateBoss();
}

void UAgHUDWidget::BindPlayer(UAbilitySystemComponent& ASC)
{
	for (const FGameplayAttribute& Attribute : AgHUDWidgetPrivate::PlayerAttributes())
	{
		ASC.GetGameplayAttributeValueChangeDelegate(Attribute).AddUObject(this, &ThisClass::HandlePlayerAttributeChanged);
	}
	ASC.RegisterGameplayTagEvent(AgGameplayTags::State_Cutscene, EGameplayTagEventType::NewOrRemoved)
		.AddWeakLambda(this, [this](const FGameplayTag, int32)
	{ UpdateStatusPanel(); });
	// 캐릭터는 HUD가 생긴 뒤 BeginPlay에서 어빌리티를 부여할 수 있으므로, 부여될 때 스킬 슬롯을 다시 그립니다.
	ASC.AbilitySpecDirtiedCallbacks.AddWeakLambda(this, [this](const FGameplayAbilitySpec&)
	{
		UpdateSkillSlots();
		UpdateCooldowns();
	});
	ASC.AbilityCommittedCallbacks.AddWeakLambda(this, [this](UGameplayAbility*)
	{ UpdateCooldowns(); });
}

void UAgHUDWidget::UnbindPlayer()
{
	UAbilitySystemComponent* ASC = PlayerASC.Get();
	if (!ASC)
	{
		return;
	}
	for (const FGameplayAttribute& Attribute : AgHUDWidgetPrivate::PlayerAttributes())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(Attribute).RemoveAll(this);
	}
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_Cutscene, EGameplayTagEventType::NewOrRemoved).RemoveAll(this);
	ASC->AbilitySpecDirtiedCallbacks.RemoveAll(this);
	ASC->AbilityCommittedCallbacks.RemoveAll(this);
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
	}
	UpdatePotion();
	UpdateUPGauge();
	UpdateSkillSlots();
}

void UAgHUDWidget::UpdatePotion()
{
	if (const UAbilitySystemComponent* ASC = PlayerASC.Get())
	{
		PotionCountText->SetText(FText::Format(INVTEXT("{0}/{1}"), FText::AsNumber(FMath::RoundToInt(ASC->GetNumericAttribute(UAgAttributeSet::GetPotionAttribute()))),
			FText::AsNumber(FMath::RoundToInt(ASC->GetNumericAttribute(UAgAttributeSet::GetMaxPotionAttribute())))));
	}
}

void UAgHUDWidget::UpdateUPGauge()
{
	const UAbilitySystemComponent* ASC = PlayerASC.Get();
	UMaterialInstanceDynamic* Gauge = ASC ? UPGauge->GetDynamicMaterial() : nullptr;
	if (Gauge)
	{
		const float MaxUP = ASC->GetNumericAttribute(UAgAttributeSet::GetMaxUPAttribute());
		Gauge->SetScalarParameterValue(AgHUDWidgetPrivate::RatioParameter, MaxUP > 0.f ? ASC->GetNumericAttribute(UAgAttributeSet::GetUPAttribute()) / MaxUP : 0.f);
	}
}

void UAgHUDWidget::UpdateSkillSlots()
{
	for (const TPair<UAgSkillSlot*, FGameplayTag>& SkillSlot : GetSkillSlots())
	{
		SkillSlot.Key->UpdateAbility();
	}
}

void UAgHUDWidget::UpdateCooldowns()
{
	for (const TPair<UAgSkillSlot*, FGameplayTag>& SkillSlot : GetSkillSlots())
	{
		SkillSlot.Key->UpdateCooldown();
	}
}

void UAgHUDWidget::UpdateBoss()
{
	const UWorld* World = GetWorld();
	const AAgBossCharacter* Boss = World ? World->GetSubsystem<UAgCombatSubsystem>()->FindBoss() : nullptr;
	BossStatusPanel->SetVisibility(Boss ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	if (Boss)
	{
		BossNameText->SetText(Boss->GetCharacterData()->DisplayName);
	}
}

TArray<TPair<UAgSkillSlot*, FGameplayTag>, TInlineAllocator<6>> UAgHUDWidget::GetSkillSlots() const
{
	return {
		{Skill1Slot.Get(), AgGameplayTags::Input_Skill1},
		{Skill2Slot.Get(), AgGameplayTags::Input_Skill2},
		{Skill3Slot.Get(), AgGameplayTags::Input_Skill3},
		{Skill4Slot.Get(), AgGameplayTags::Input_Skill4},
		{UltimateSlot.Get(), AgGameplayTags::Input_Ultimate},
		{PotionSlot.Get(), AgGameplayTags::Input_Potion},
	};
}

void UAgHUDWidget::SetBarValue(UAgStatBar& Bar, const UAbilitySystemComponent& ASC, const FGameplayAttribute& Value, const FGameplayAttribute& MaxValue)
{
	Bar.SetValue(ASC.GetNumericAttribute(Value), ASC.GetNumericAttribute(MaxValue));
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
	else if (Attribute == UAgAttributeSet::GetMPAttribute() || Attribute == UAgAttributeSet::GetMaxMPAttribute())
	{
		SetBarValue(*PlayerMPBar, *ASC, UAgAttributeSet::GetMPAttribute(), UAgAttributeSet::GetMaxMPAttribute());
	}
	else if (Attribute == UAgAttributeSet::GetPotionAttribute() || Attribute == UAgAttributeSet::GetMaxPotionAttribute())
	{
		UpdatePotion();
	}
	else
	{
		UpdateUPGauge();
	}
	// 슬롯의 흐림은 어빌리티 정의의 비용과 발동 조건이 정하므로, 어느 자원이 바뀌어도 다시 봅니다.
	UpdateSkillSlots();
}
