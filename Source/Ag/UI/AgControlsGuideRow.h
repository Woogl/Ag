// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "AgControlsGuideRow.generated.h"

class UTextBlock;
struct FAgControlsGuideEntry;

UCLASS(Abstract)
class UAgControlsGuideRow : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	void SetEntry(const FAgControlsGuideEntry& Entry);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> KeysText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ActionText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DescriptionText;
};
