// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgEffect_InfinitePP.h"

#include "Core/AgGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UAgEffect_InfinitePP::UAgEffect_InfinitePP()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	// 네이티브 GameplayEffect는 PostLoad를 거치지 않으므로, 부여 태그를 생성자에서 바로 GameplayEffect에 넣습니다.
	UTargetTagsGameplayEffectComponent* TargetTags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	FInheritedTagContainer GrantedTags;
	GrantedTags.Added.AddTag(AgGameplayTags::State_InfinitePP);
	TargetTags->SetAndApplyTargetTagChanges(GrantedTags);
	GEComponents.Add(TargetTags);
}
