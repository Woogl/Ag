// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "AgControlsGuideRow.generated.h"

class UTextBlock;

UCLASS(Abstract)
class UAgControlsGuideRow : public UCommonUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativePreConstruct() override;

	UPROPERTY(EditAnywhere, Category = "Controls Guide")
	FText Keys;

	UPROPERTY(EditAnywhere, Category = "Controls Guide")
	FText Action;

	UPROPERTY(EditAnywhere, Category = "Controls Guide", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> KeysText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ActionText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DescriptionText;
};
