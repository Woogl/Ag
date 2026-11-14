// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "AgInteractionComponent.generated.h"

/**
 * 플레이어가 지금 상호작용 키로 쓸 대상을 고르고, 그 대상의 안내 지점에 상호작용 안내(화면 공간 위젯)를 띄웁니다.
 * 안내 지점과 안내 사이의 간격은 WBP 배치가 정합니다.
 */
UCLASS()
class UAgInteractionComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	UAgInteractionComponent();

	/** 지금 고른 대상에 상호작용합니다. 대상이 없으면 아무 일도 하지 않습니다. */
	void Interact();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
