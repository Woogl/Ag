// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "AgNameplateComponent.generated.h"

/** 적 네임플레이트를 그리는 화면 공간 위젯입니다. 락온 대상의 네임플레이트는 다른 대상들보다 앞에 그립니다. */
UCLASS()
class UAgNameplateComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	void SetFrontmost(bool bNewFrontmost);

protected:
	virtual void BeginPlay() override;

private:
	FName BackLayerName;
	int32 BackLayerZOrder = 0;

	bool bFrontmost = false;
};
