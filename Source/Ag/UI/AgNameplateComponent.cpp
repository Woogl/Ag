// Copyright Woogle. All Rights Reserved.

#include "UI/AgNameplateComponent.h"

namespace AgNameplateComponentPrivate
{
	const FName FrontLayerName(TEXT("AgFrontNameplateLayer"));
} // namespace AgNameplateComponentPrivate

void UAgNameplateComponent::SetFrontmost(bool bNewFrontmost)
{
	if (bFrontmost == bNewFrontmost)
	{
		return;
	}
	bFrontmost = bNewFrontmost;

	// 엔진은 한 화면 레이어 안의 순서를 매 프레임 카메라 거리로 다시 정하므로, 앞에 둘 위젯은 바로 위 레이어로 옮겨
	// 다시 등록합니다. 레이어 이름과 ZOrder는 등록할 때만 읽힙니다.
	const bool bWasOnScreen = bAddedToScreen;
	if (bWasOnScreen)
	{
		RemoveWidgetFromScreen();
	}
	SharedLayerName = bFrontmost ? AgNameplateComponentPrivate::FrontLayerName : BackLayerName;
	LayerZOrder = bFrontmost ? BackLayerZOrder + 1 : BackLayerZOrder;
	if (bWasOnScreen)
	{
		UpdateWidget();
	}
}

void UAgNameplateComponent::BeginPlay()
{
	Super::BeginPlay();

	BackLayerName = SharedLayerName;
	BackLayerZOrder = LayerZOrder;
}
