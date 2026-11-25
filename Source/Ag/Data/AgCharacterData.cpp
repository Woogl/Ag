// Copyright Woogle. All Rights Reserved.

#include "Data/AgCharacterData.h"

UAgCharacterData* UAgCharacterData::Find(FName Id)
{
	return UAgAssetManager::Get().GetData<UAgCharacterSet>().Characters.FindRef(Id);
}

TArray<FName> UAgCharacterData::GetIds()
{
	TArray<FName> Ids;
	UAgAssetManager::Get().GetData<UAgCharacterSet>().Characters.GetKeys(Ids);
	return Ids;
}

bool UAgCharacterSet::ReadJson(FAgJsonReader& Reader, FName Key)
{
	return ReadJsonInto(Reader, GET_MEMBER_NAME_CHECKED(UAgCharacterSet, Characters));
}

void UAgCharacterSet::CopyJson(const UAgJsonData& Loaded)
{
	const TMap<FName, TObjectPtr<UAgCharacterData>> Previous = Characters;
	Super::CopyJson(Loaded);

	// 캐릭터는 자기 데이터 객체를 들고 있으므로, ID가 그대로인 항목은 새 객체의 값을 기존 객체에 복사해 기존 객체를 계속 씁니다.
	for (TPair<FName, TObjectPtr<UAgCharacterData>>& Pair : Characters)
	{
		if (const TObjectPtr<UAgCharacterData>* Existing = Previous.Find(Pair.Key))
		{
			CopyJsonFields(*Pair.Value, **Existing);
			Pair.Value = *Existing;
		}
	}
}
