// Copyright Woogle. All Rights Reserved.

#include "Data/AgCharacterData.h"

#include "Animation/AnimMontage.h"
#include "Data/AgDataFile.h"

void UAgCharacterData::PostLoad()
{
	Super::PostLoad();

	// The numbers and settings are read on every load; they are never saved in the asset.
	AgDataFile::Load(*this, ValuesFile);
}

#if WITH_EDITOR
void UAgCharacterData::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(UAgCharacterData, ValuesFile))
	{
		AgDataFile::Load(*this, ValuesFile);
	}
}
#endif

const FAgBossPattern* UAgBossData::FindPattern(const FGameplayTag& Pattern) const
{
	return Patterns.FindByPredicate([&Pattern](const FAgBossPattern& Entry) { return Entry.Pattern == Pattern; });
}

UAnimMontage* UAgBossData::FindPatternMontage(const FGameplayTag& Pattern) const
{
	const TObjectPtr<UAnimMontage>* Montage = PatternMontages.Find(Pattern);
	return Montage ? Montage->Get() : nullptr;
}
