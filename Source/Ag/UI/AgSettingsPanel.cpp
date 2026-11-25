// Copyright Woogle. All Rights Reserved.

#include "UI/AgSettingsPanel.h"

#include "CommonAnimatedSwitcher.h"
#include "CommonButtonBase.h"
#include "UI/AgTabList.h"

void UAgSettingsPanel::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	CategoryList->SetLinkedSwitcher(CategoryContent);
	CategoryList->OnTabSelected.AddDynamic(this, &ThisClass::HandleCategorySelected);
}

UWidget* UAgSettingsPanel::NativeGetDesiredFocusTarget() const
{
	return CategoryList->GetTabButtonBaseByID(CategoryList->GetActiveTab());
}

void UAgSettingsPanel::HandleCategorySelected(FName TabId)
{
	// 기하 탐색에 맡기면 ←가 같은 높이의 다른 분류 이름에 닿아 분류가 바뀌므로, 고른 분류와 그 내용 사이를 직접 잇습니다.
	UCommonButtonBase* CategoryButton = CategoryList->GetTabButtonBaseByID(TabId);
	// 내비게이션은 포커스를 받는 위젯으로만 옮기므로, 사용자 위젯(드롭다운)이면 그 위젯의 포커스 대상까지 내려갑니다.
	UWidget* ContentTarget = CategoryContent->GetActiveWidget();
	while (const UUserWidget* UserWidget = Cast<UUserWidget>(ContentTarget))
	{
		ContentTarget = UserWidget->GetDesiredFocusWidget();
	}
	if (ContentTarget)
	{
		CategoryList->SetNavigationRuleExplicit(EUINavigation::Right, ContentTarget);
	}
	else
	{
		CategoryList->SetNavigationRuleBase(EUINavigation::Right, EUINavigationRule::Stop);
	}
	CategoryContent->SetNavigationRuleExplicit(EUINavigation::Left, CategoryButton);
	// W·S로 분류를 바꾸면 포커스가 이전 분류 이름이나 숨겨진 내용에 남으므로 고른 분류의 이름으로 옮깁니다.
	if (HasFocusedDescendants() && !CategoryButton->HasAnyUserFocus())
	{
		CategoryButton->SetFocus();
	}
}
