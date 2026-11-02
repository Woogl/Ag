// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "AgEnemyOptionWidget.generated.h"

class UCommonTextBlock;

UCLASS(Abstract, meta = (DisableNativeTick))
class UAgEnemyOptionWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	/** None이면 EmptyText를 표시합니다. */
	void SetEnemy(FName CharacterDataId);

protected:
	UPROPERTY(EditAnywhere, Category = "UI")
	FText EmptyText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> NameText;
};
