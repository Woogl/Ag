// Copyright Woogle. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AgAbilitySystemComponent.generated.h"

/** 스펙별 동적 태그의 차단·취소를 엔진 GAS에 연결합니다. */
UCLASS()
class UAgAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	virtual void ApplyAbilityBlockAndCancelTags(const FGameplayTagContainer& AbilityTags, UGameplayAbility* RequestingAbility,
	    bool bEnableBlockTags, const FGameplayTagContainer& BlockTags, bool bExecuteCancelTags,
	    const FGameplayTagContainer& CancelTags) override;
	void CancelAbilitiesWithSpecTags(const FGameplayTagContainer& Tags, UGameplayAbility* Ignore = nullptr);
};
