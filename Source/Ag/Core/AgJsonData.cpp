// Copyright Woogle. All Rights Reserved.

#include "Core/AgJsonData.h"

#include "Core/AgJsonReader.h"
#include "UObject/UnrealType.h"

bool UAgJsonData::ReadJson(FAgJsonReader& Reader, FName Key)
{
	return Reader.ReadFile(GetJsonPath(), *this);
}

void UAgJsonData::GatherAssetPaths(const FAgJsonReader& Reader, FName Key, TArray<FSoftObjectPath>& OutPaths) const
{
	TArray<FString> Strings;
	Reader.GatherStrings(GetJsonPath(), Reader.GetEntryIds(GetJsonPath()), Strings);
	FAgJsonReader::AddAssetPaths(Strings, OutPaths);
}

void UAgJsonData::CopyJson(const UAgJsonData& Loaded)
{
	CopyJsonFields(Loaded, *this);
}

void UAgJsonData::CopyJsonFields(const UObject& From, UObject& To)
{
	for (TFieldIterator<FProperty> It(To.GetClass()); It; ++It)
	{
		if (FAgJsonReader::IsField(**It))
		{
			It->CopyCompleteValue_InContainer(&To, &From);
		}
	}
}

FString UAgJsonData::GetJsonPath() const
{
	return UAgAssetManager::GetJsonPath(GetFileName());
}

bool UAgJsonData::ReadJsonInto(FAgJsonReader& Reader, FName PropertyName)
{
	const FProperty& Property = GetJsonProperty(PropertyName);
	return Reader.ReadFile(GetJsonPath(), Property, Property.ContainerPtrToValuePtr<void>(this));
}

const FProperty& UAgJsonData::GetJsonProperty(FName PropertyName) const
{
	const FProperty* Property = GetClass()->FindPropertyByName(PropertyName);
	checkf(Property, TEXT("%s has no UPROPERTY %s."), *GetClass()->GetName(), *PropertyName.ToString());
	return *Property;
}
