// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPtr.h"
#include "AgItemData.generated.h"

class UTexture2D;

UENUM()
enum class EAgItemCategory : uint8
{
	Consumable,
	Currency
};

USTRUCT()
struct FAgItemData
{
	GENERATED_BODY()

	UPROPERTY()
	EAgItemCategory Category = EAgItemCategory::Consumable;

	UPROPERTY()
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY()
	int32 MaxCount = 1;

	UPROPERTY()
	int32 StartCount = 0;
};

UCLASS()
class UAgItemSet : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, VisibleAnywhere, Category = "Items")
	TMap<FName, FAgItemData> Items;
};
