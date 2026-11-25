// Copyright Woogle. All Rights Reserved.

#include "UI/AgHUDWidget.h"

#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Character/AgBossCharacter.h"
#include "Character/AgPlayerCharacter.h"
#include "Combat/AgCombatSubsystem.h"
#include "CommonInputSettings.h"
#include "ICommonInputModule.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Core/AgAssetManager.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "Data/AgItemData.h"
#include "Engine/World.h"
#include "Game/AgGameState.h"
#include "Game/AgPlayerController.h"
#include "Input/CommonUIInputTypes.h"
#include "Item/AgInventoryComponent.h"
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
			UAgAttributeSet::GetMaxMPAttribute(), UAgAttributeSet::GetUPAttribute(), UAgAttributeSet::GetMaxUPAttribute()};
	}

} // namespace AgHUDWidgetPrivate

void UAgHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 엔진의 뒤로 처리(bIsBackHandler)는 메뉴 입력 모드에서만 받으므로, 게임 입력 모드인 HUD는 뒤로 액션을 직접 바인딩합니다.
	FBindUIActionArgs BackArgs(ICommonInputModule::GetSettings().GetEnhancedInputBackAction(), false,
		FSimpleDelegate::CreateUObject(this, &ThisClass::OpenPauseMenu));
	BackArgs.InputMode = ECommonInputMode::Game;
	RegisterUIActionBinding(BackArgs);
}

void UAgHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(GetOwningPlayerPawn());
	PlayerASC = Player ? Player->GetAbilitySystemComponent() : nullptr;
	const AAgPlayerController* PlayerController = GetOwningPlayer<AAgPlayerController>();
	PlayerInventory = PlayerController ? PlayerController->GetInventory() : nullptr;
	BindPlayer();
	for (const TPair<UAgSkillSlot*, FGameplayTag>& SkillSlot : GetSkillSlots())
	{
		SkillSlot.Key->SetInput(Player, SkillSlot.Value);
	}

#if WITH_EDITOR
	// 데이터 파일을 다시 읽으면 보스 이름, 스킬 아이콘·코스트·최대 충전, 회복병의 최대 보유 수가 바뀝니다.
	UAgAssetManager::Get().OnJsonDataReloaded.AddWeakLambda(this, [this]
	{
		UpdateSkillSlots();
		UpdateCooldowns();
		UpdateBoss();
		UpdatePotion();
	});
#endif

	GetWorld()->GetSubsystem<UAgCombatSubsystem>()->OnCombatantsChanged.AddUObject(this, &ThisClass::UpdateBoss);
	if (AAgGameState* StageState = GetWorld()->GetGameState<AAgGameState>())
	{
		StageState->OnStageResultChanged.AddUObject(this, &ThisClass::UpdateStatusPanel);
	}
	UpdateBoss();

	UpdateStatusPanel();
	UpdatePlayer();
}

void UAgHUDWidget::NativeDestruct()
{
	UnbindPlayer();
	PlayerASC.Reset();
	PlayerInventory.Reset();
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

void UAgHUDWidget::BindPlayer()
{
	if (UAgInventoryComponent* Inventory = PlayerInventory.Get())
	{
		// 슬롯의 흐림은 그 어빌리티가 쓰는 아이템의 보유 수도 봅니다.
		Inventory->OnItemsChanged.AddWeakLambda(this, [this]
		{
			UpdatePotion();
			UpdateSkillSlots();
		});
	}
	UAbilitySystemComponent* ASC = PlayerASC.Get();
	if (!ASC)
	{
		return;
	}
	for (const FGameplayAttribute& Attribute : AgHUDWidgetPrivate::PlayerAttributes())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(Attribute).AddUObject(this, &ThisClass::HandlePlayerAttributeChanged);
	}
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_Cutscene, EGameplayTagEventType::NewOrRemoved)
		.AddWeakLambda(this, [this](const FGameplayTag, int32)
	{ UpdateStatusPanel(); });
	// 캐릭터는 HUD가 생긴 뒤 BeginPlay에서 어빌리티를 부여할 수 있으므로, 스펙이 바뀔 때(부여, 발동, 종료) 스킬 슬롯과
	// 회복병 칸 어빌리티의 아이템으로 세는 회복병 횟수를 다시 그립니다.
	ASC->AbilitySpecDirtiedCallbacks.AddWeakLambda(this, [this](const FGameplayAbilitySpec&)
	{
		UpdateSkillSlots();
		UpdateCooldowns();
		UpdatePotion();
	});
	ASC->AbilityCommittedCallbacks.AddWeakLambda(this, [this](UGameplayAbility*)
	{ UpdateCooldowns(); });
}

void UAgHUDWidget::UnbindPlayer()
{
	if (UAgInventoryComponent* Inventory = PlayerInventory.Get())
	{
		Inventory->OnItemsChanged.RemoveAll(this);
	}
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
	const bool bStageEnded = StageState && StageState->IsStageResultDecided();
	StatusPanel->SetVisibility(bCutscene || bStageEnded ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
}

void UAgHUDWidget::UpdatePlayer()
{
	UpdateHPBar();
	UpdateMPBar();
	UpdatePotion();
	UpdateUPGauge();
}

void UAgHUDWidget::UpdateHPBar()
{
	if (const UAbilitySystemComponent* ASC = PlayerASC.Get())
	{
		PlayerHPBar->SetValue(ASC->GetNumericAttribute(UAgAttributeSet::GetHPAttribute()), ASC->GetNumericAttribute(UAgAttributeSet::GetMaxHPAttribute()));
	}
}

void UAgHUDWidget::UpdateMPBar()
{
	if (const UAbilitySystemComponent* ASC = PlayerASC.Get())
	{
		PlayerMPBar->SetValue(ASC->GetNumericAttribute(UAgAttributeSet::GetMPAttribute()), ASC->GetNumericAttribute(UAgAttributeSet::GetMaxMPAttribute()));
	}
}

void UAgHUDWidget::UpdatePotion()
{
	const UAgGameplayAbility* Ability = PotionSlot->FindAbility();
	const FAgAbilityData* Data = Ability ? Ability->FindDefinition() : nullptr;
	const FAgItemData* Item = Data ? UAgAssetManager::Get().FindItem(Data->Item) : nullptr;
	const UAgInventoryComponent* Inventory = PlayerInventory.Get();
	if (Item && Inventory)
	{
		PotionCountText->SetText(FText::Format(INVTEXT("{0}/{1}"), FText::AsNumber(Inventory->GetCount(Data->Item)), FText::AsNumber(Item->MaxCount)));
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
	// 같은 보스를 다시 넘기면 보스 상태가 그대로 두므로, 다른 적의 전투 진입으로 대미지 잔상이 지워지지 않습니다.
	const AAgBossCharacter* Boss = GetWorld()->GetSubsystem<UAgCombatSubsystem>()->FindBoss();
	BossStatus->SetAbilitySystem(Boss ? Boss->GetAbilitySystemComponent() : nullptr);
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

void UAgHUDWidget::HandlePlayerAttributeChanged(const FOnAttributeChangeData& Data)
{
	// 한 효과가 여러 어트리뷰트를 바꿀 때 알림 순서와 상관없도록, 바뀐 어트리뷰트의 표시만 다시 그립니다.
	const FGameplayAttribute& Attribute = Data.Attribute;
	if (Attribute == UAgAttributeSet::GetHPAttribute() || Attribute == UAgAttributeSet::GetMaxHPAttribute())
	{
		UpdateHPBar();
	}
	else if (Attribute == UAgAttributeSet::GetMPAttribute() || Attribute == UAgAttributeSet::GetMaxMPAttribute())
	{
		UpdateMPBar();
	}
	else
	{
		UpdateUPGauge();
	}
	// 슬롯의 흐림은 어빌리티 정의의 비용과 발동 조건이 정하므로, 어느 자원이 바뀌어도 다시 봅니다.
	UpdateSkillSlots();
}
