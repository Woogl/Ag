// Copyright Woogle. All Rights Reserved.

#include "Data/AgCharacterData.h"

#include "AbilitySystem/AgGameplayAbility.h"
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

void UAgCharacterData::PostEditUndo()
{
	Super::PostEditUndo();

	// 실행 취소·다시 실행은 Transient 속성(파일에서 읽은 값)까지 그 편집 때의 값으로 되돌립니다. 기준은 파일이므로 다시 읽습니다.
	AgDataFile::Load(*this, ValuesFile);
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

TSubclassOf<UAgGameplayAbility> UAgBossData::FindPatternAbility(const FGameplayTag& Pattern) const
{
	return PatternAbilities.FindRef(Pattern);
}

FGameplayTag UAgBossData::FindPatternCooldownTag(const FGameplayTag& Pattern) const
{
	return PatternCooldownTags.FindRef(Pattern);
}
