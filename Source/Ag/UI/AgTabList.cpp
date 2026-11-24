// Copyright Woogle. All Rights Reserved.

#include "UI/AgTabList.h"

#include "Ag.h"
#include "CommonActivatableWidget.h"
#include "CommonAnimatedSwitcher.h"
#include "Components/HorizontalBox.h"
#include "UI/AgActionWidget.h"
#include "UI/AgTabButton.h"

UAgTabList::UAgTabList(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoListenForInput = true;
}

void UAgTabList::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	PreviousTabAction->SetEnhancedInputAction(PreviousTabEnhancedInputAction);
	NextTabAction->SetEnhancedInputAction(NextTabEnhancedInputAction);
}

void UAgTabList::NativeConstruct()
{
	Super::NativeConstruct();

	UCommonAnimatedSwitcher* Switcher = GetLinkedSwitcher();
	if (!Switcher)
	{
		UE_LOG(LogAg, Error, TEXT("%s: no linked switcher, so the tabs have nowhere to show their content."), *GetName());
		return;
	}
	// 엔진은 화면에서 빠질 때 탭을 모두 지우므로 붙을 때마다 다시 등록합니다. 내용 화면은 처음 한 번만 만들어 상태를 이어 갑니다.
	for (int32 Index = 0; Index < Tabs.Num(); ++Index)
	{
		if (!ContentWidgets.IsValidIndex(Index))
		{
			UCommonActivatableWidget* Content = CreateWidget<UCommonActivatableWidget>(this, Tabs[Index].Content);
			Switcher->AddChild(Content);
			ContentWidgets.Add(Content);
		}
		RegisterTab(FName(TEXT("Tab"), Index), TabButtonClass, ContentWidgets[Index]);
	}
	// 메뉴가 켜진 뒤에 탭을 바꾸면 지난번 탭이 먼저 켜져 CommonUI의 첫 포커스가 그쪽에 남으므로, 켜지기 전인 지금 고릅니다.
	if (Tabs.IsValidIndex(DefaultTabIndex))
	{
		SelectTabByID(GetTabIdAtIndex(DefaultTabIndex), true);
	}
}

void UAgTabList::HandleTabCreation_Implementation(FName TabNameID, UCommonButtonBase* TabButton)
{
	Super::HandleTabCreation_Implementation(TabNameID, TabButton);

	const int32 Index = GetRegisteredTabsByID().FindChecked(TabNameID).TabIndex;
	CastChecked<UAgTabButton>(TabButton)->SetLabel(Tabs[Index].Label);
	TabButtonBox->AddChildToHorizontalBox(TabButton);
}
