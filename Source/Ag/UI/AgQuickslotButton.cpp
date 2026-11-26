// Copyright Woogle. All Rights Reserved.

#include "UI/AgQuickslotButton.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "Character/AgPlayerCharacter.h"
#include "CommonLazyImage.h"
#include "Components/TextBlock.h"
#include "Data/AgAbilityData.h"
#include "Data/AgItemData.h"
#include "Engine/Texture2D.h"
#include "Game/AgPlayerController.h"
#include "Item/AgInventoryComponent.h"

void UAgQuickslotButton::SetInput(AAgPlayerCharacter* InPlayer, const FGameplayTag& InInputTag)
{
	Player = InPlayer;
	InputTag = InInputTag;
	UpdateItem();
}

void UAgQuickslotButton::UpdateItem()
{
	const AAgPlayerCharacter* Character = Player.Get();
	const UAgGameplayAbility* Ability = Character ? Character->FindInputAbility(InputTag) : nullptr;
	const FAgAbilityData* Data = Ability ? Ability->FindDefinition() : nullptr;
	const FAgItemData* Item = Data ? UAgItemSet::FindItem(Data->Item) : nullptr;
	const AAgPlayerController* PlayerController = GetOwningPlayer<AAgPlayerController>();
	const UAgInventoryComponent* Inventory = PlayerController ? PlayerController->GetInventory() : nullptr;

	const TSoftObjectPtr<UTexture2D> Icon = Item ? Item->Icon : TSoftObjectPtr<UTexture2D>();
	// 이미지가 없는 아이콘은 빈 사각형으로 그려지므로 숨깁니다.
	IconImage->SetVisibility(Icon.IsNull() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	if (!Icon.IsNull())
	{
		IconImage->SetBrushFromLazyTexture(Icon);
	}

	const int32 Count = Item && Inventory ? Inventory->GetCount(Data->Item) : 0;
	CountText->SetVisibility(Item ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (Item)
	{
		CountText->SetText(FText::Format(INVTEXT("{0}/{1}"), FText::AsNumber(Count), FText::AsNumber(Item->MaxCount)));
	}
	Body->SetRenderOpacity(Count > 0 ? 1.f : DisabledOpacity);
}
