// Copyright Woogle. All Rights Reserved.

#include "Data/AgCharacterData.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "Animation/AnimMontage.h"
#include "Core/AgAssetManager.h"

void UAgCharacterData::PostLoad()
{
	Super::PostLoad();

	// 기획 값은 로드할 때마다 JSON에서 읽고, 에셋에는 저장하지 않습니다.
	UAgAssetManager::Get().LoadJsonValues(*this, ValuesFile);
}

#if WITH_EDITOR
void UAgCharacterData::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(UAgCharacterData, ValuesFile))
	{
		UAgAssetManager::Get().LoadJsonValues(*this, ValuesFile);
	}
}

void UAgCharacterData::PostEditUndo()
{
	Super::PostEditUndo();

	// 실행 취소·다시 실행은 Transient 속성(파일에서 읽은 값)까지 그 편집 때의 값으로 되돌립니다. 기준은 파일이므로 다시 읽습니다.
	UAgAssetManager::Get().LoadJsonValues(*this, ValuesFile);
}
#endif

const FAgBossPattern* UAgBossData::FindPattern(FName Id) const
{
	return Patterns.FindByPredicate([Id](const FAgBossPattern& Entry) { return Entry.Id == Id; });
}

UAnimMontage* UAgBossData::FindPatternMontage(FName Id) const
{
	const TObjectPtr<UAnimMontage>* Montage = PatternMontages.Find(Id);
	return Montage ? Montage->Get() : nullptr;
}

TSubclassOf<UAgGameplayAbility> UAgBossData::FindPatternAbility(FName Id) const
{
	return PatternAbilities.FindRef(Id);
}
