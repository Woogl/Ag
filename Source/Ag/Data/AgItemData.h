// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/AgJsonData.h"
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
	FText DisplayName;

	UPROPERTY()
	FText Description;

	UPROPERTY()
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY()
	int32 MaxCount = 1;

	UPROPERTY()
	int32 StartCount = 0;
};

UCLASS()
class UAgItemSet : public UAgJsonData
{
	GENERATED_BODY()

public:
	/** 데이터를 다시 읽으면 바뀌므로 포인터는 그 함수 안에서만 씁니다. */
	static const FAgItemData* FindItem(FName Id)
	{
		return GetItems().Find(Id);
	}

	static const TMap<FName, FAgItemData>& GetItems()
	{
		return UAgAssetManager::Get().GetData<UAgItemSet>().Items;
	}

	virtual const TCHAR* GetFileName() const override
	{
		return TEXT("Items.json");
	}

	virtual bool IsAlwaysLoaded() const override
	{
		return true;
	}

	virtual bool ReadJson(FAgJsonReader& Reader, FName Key) override
	{
		return ReadJsonInto(Reader, GET_MEMBER_NAME_CHECKED(UAgItemSet, Items));
	}

	UPROPERTY(Transient, VisibleAnywhere, Category = "Items")
	TMap<FName, FAgItemData> Items;
};
