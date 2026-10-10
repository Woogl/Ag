// Copyright Woogle. All Rights Reserved.

#include "Data/AgCharacterData.h"

#include "Animation/AnimMontage.h"

const FAgBossPattern* UAgBossData::FindPattern(const FGameplayTag& Pattern) const
{
	return Patterns.FindByPredicate([&Pattern](const FAgBossPattern& Entry) { return Entry.Pattern == Pattern; });
}

UAnimMontage* UAgBossData::FindPatternMontage(const FGameplayTag& Pattern) const
{
	const TObjectPtr<UAnimMontage>* Montage = PatternMontages.Find(Pattern);
	return Montage ? Montage->Get() : nullptr;
}
