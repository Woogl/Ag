// Copyright Woogle. All Rights Reserved.

#include "Data/AgCharacterData.h"

#include "Core/AgAssetManager.h"

const FAgAbilityData* UAgCharacterData::FindAbility(FName Id) const
{
	return Abilities.Contains(Id) ? UAgAssetManager::Get().FindAbility(Id) : nullptr;
}
