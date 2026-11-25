// Copyright Woogle. All Rights Reserved.

#include "Data/AgAbilityData.h"

#include "Core/AgJsonReader.h"
#include "Data/AgCharacterData.h"
#include "Data/AgEffectData.h"
#include "Data/AgSpawnActorData.h"

bool FAgResourceCost::IsFree() const
{
	return MP <= 0.f && UP <= 0.f;
}

void FAgResourceCost::AddToSpec(FGameplayEffectSpec& Spec) const
{
	AgEffectData::SetResourceAmounts(Spec, -MP, -UP);
}

bool FAgPatternSelection::IsAvailable(float TargetDistance, int32 Phase) const
{
	return Distance.Contains(TargetDistance) && IsInPhase(Phase);
}

bool FAgPatternSelection::IsInPhase(int32 Phase) const
{
	return Phases.IsEmpty() || Phases.Contains(Phase);
}

bool FAgAbilityData::HasSameGrantSettings(const FAgAbilityData& Other) const
{
	if (AbilityTags != Other.AbilityTags || ActivationRequiredTags != Other.ActivationRequiredTags ||
		ActivationBlockedTags != Other.ActivationBlockedTags || ActivationOwnedTags != Other.ActivationOwnedTags ||
		BlockAbilitiesWithTag != Other.BlockAbilitiesWithTag || CancelAbilitiesWithTag != Other.CancelAbilitiesWithTag ||
		bAllowRetrigger != Other.bAllowRetrigger || AbilityTriggers.Num() != Other.AbilityTriggers.Num())
	{
		return false;
	}
	for (int32 Index = 0; Index < AbilityTriggers.Num(); ++Index)
	{
		if (AbilityTriggers[Index].TriggerTag != Other.AbilityTriggers[Index].TriggerTag ||
			AbilityTriggers[Index].TriggerSource != Other.AbilityTriggers[Index].TriggerSource)
		{
			return false;
		}
	}
	return true;
}

bool UAgAbilitySet::ReadJson(FAgJsonReader& Reader, FName Key)
{
	const FProperty& Property = GetJsonProperty(GET_MEMBER_NAME_CHECKED(UAgAbilitySet, Abilities));
	if (!Key.IsNone())
	{
		// Key는 캐릭터 ID이고, 그 캐릭터의 어빌리티 목록에 있는 항목만 읽습니다.
		const UAgCharacterData* Character = UAgCharacterData::Find(Key);
		return Character && Reader.ReadEntries(GetJsonPath(), Character->Abilities, Property, &Abilities);
	}
	bool bRead = Reader.ReadFile(GetJsonPath(), Property, &Abilities);
	// 어느 어빌리티도 가리키지 않는 생성 액터 정의도 검사합니다.
	const FString SpawnActorsPath = UAgAssetManager::GetJsonPath(FAgSpawnActorDefinition::GetFileName());
	for (FName Id : Reader.GetEntryIds(SpawnActorsPath))
	{
		FAgSpawnActorDefinition Definition;
		bRead = Reader.ReadSpawnActor(SpawnActorsPath, Id, Definition) && bRead;
	}
	return bRead;
}
