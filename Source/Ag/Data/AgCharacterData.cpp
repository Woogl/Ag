// Copyright Woogle. All Rights Reserved.

#include "Data/AgCharacterData.h"

#include "Ag.h"
#include "Animation/AnimMontage.h"
#include "Core/AgAssetManager.h"

const FAgAbilityData* UAgCharacterData::FindAbility(FName Id) const
{
	return Abilities.Contains(Id) ? UAgAssetManager::Get().FindAbility(Id) : nullptr;
}

UAnimMontage* UAgCharacterData::FindMontage(FName Id) const
{
	// 몽타주 ID는 코드에 적은 이름이나 어빌리티 ID라 컴파일러가 확인하지 못하므로, 없으면 바로 알립니다.
	UAnimMontage* Montage = Montages.FindRef(Id);
	if (!Montage)
	{
		UE_LOG(LogAg, Error, TEXT("%s has no montage %s in Montages."), *GetName(), *Id.ToString());
	}
	return Montage;
}
