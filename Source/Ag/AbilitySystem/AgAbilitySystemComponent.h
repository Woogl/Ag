// Copyright Woogle. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AgAbilitySystemComponent.generated.h"

enum class EAgEffect : uint8;

/** 스펙별 동적 태그의 차단·취소를 엔진 GAS에 연결하고, Effects.json의 효과로 스펙을 만듭니다. */
UCLASS()
class UAgAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	virtual void ApplyAbilityBlockAndCancelTags(const FGameplayTagContainer& AbilityTags, UGameplayAbility* RequestingAbility,
	    bool bEnableBlockTags, const FGameplayTagContainer& BlockTags, bool bExecuteCancelTags,
	    const FGameplayTagContainer& CancelTags) override;
	void CancelAbilitiesWithSpecTags(const FGameplayTagContainer& Tags, UGameplayAbility* Ignore = nullptr);

	/**
	 * 엔진 MakeOutgoingSpec이 GE 클래스로 하는 일을 Effects.json의 효과로 합니다. Context가 비어 있으면 새로 만듭니다.
	 * 효과가 없으면 빈 핸들입니다.
	 */
	FGameplayEffectSpecHandle MakeEffectSpec(EAgEffect Effect, float Level, FGameplayEffectContextHandle Context) const;
};
