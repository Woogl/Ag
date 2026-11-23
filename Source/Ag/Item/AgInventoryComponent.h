// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AgInventoryComponent.generated.h"

/** 플레이어가 가진 아이템별 보유 수입니다. 게임 월드에서 만들어질 때 아이템 데이터의 시작 보유 수로 채웁니다. */
UCLASS()
class UAgInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAgInventoryComponent();

	int32 GetCount(FName Item) const;

	/** 최대 보유 수까지만 더하고, 더한 수를 돌려줍니다. */
	int32 AddItem(FName Item, int32 Count);

	/** Count개를 다 뺄 수 있으면 빼고 true, 아니면 그대로 두고 false를 돌려줍니다. */
	bool RemoveItem(FName Item, int32 Count);

	FSimpleMulticastDelegate OnItemsChanged;

protected:
	virtual void InitializeComponent() override;

private:
	// PIE 중 디테일 패널과 시나리오 테스트가 읽습니다.
	UPROPERTY(VisibleInstanceOnly, Category = "Items")
	TMap<FName, int32> Counts;
};
