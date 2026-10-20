// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AgControlsGuideData.generated.h"

class UInputMappingContext;

struct FAgControlsGuideEntry
{
	FText Keys;
	FText Action;
	FText Description;
};

UCLASS()
class UAgControlsGuideData : public UObject
{
	GENERATED_BODY()

public:
	TArray<FAgControlsGuideEntry> MakeEntries(const UInputMappingContext& Mapping) const;

	UPROPERTY(Transient)
	TMap<FName, FText> KeyDisplayNames;

	UPROPERTY(Transient)
	TMap<FName, FText> ActionDescriptions;
};
