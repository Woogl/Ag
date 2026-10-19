// Copyright Woogle. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AgAbilitySystemComponent.generated.h"

class UAnimMontage;
class UGameplayEffect;
enum class EAgEffect : uint8;
struct FAnimMontageInstance;

UCLASS()
class UAgAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	/** 엔진은 클래스의 어빌리티 태그로 취소할 행동을 찾는데 행동 태그는 스펙에 있으므로, 취소만 스펙 태그로 합니다. */
	virtual void ApplyAbilityBlockAndCancelTags(const FGameplayTagContainer& AbilityTags, UGameplayAbility* RequestingAbility, bool bEnableBlockTags, const FGameplayTagContainer& BlockTags, bool bExecuteCancelTags, const FGameplayTagContainer& CancelTags) override;
	void CancelAbilitiesWithSpecTags(const FGameplayTagContainer& Tags, UGameplayAbility* Ignore = nullptr);

	/** 엔진 TryActivateAbilitiesByTag는 클래스 태그를 보므로 쓰지 않고 스펙 태그로 찾습니다. */
	FGameplayAbilitySpec* FindSpecByTag(const FGameplayTag& Tag);
	const FGameplayAbilitySpec* FindSpecByTag(const FGameplayTag& Tag) const;

	/** 엔진 PlayMontage는 블렌드 인 시간을 정할 수 없으므로 같은 기록을 남기며 직접 재생합니다. */
	FAnimMontageInstance* PlayMontageWithBlendIn(UGameplayAbility& Ability, UAnimMontage& Montage, float BlendInTime, FName StartSection);

	/** 엔진 MakeOutgoingSpec은 GE 클래스를 받는데 Effects.json의 효과는 GE 객체이므로 직접 만듭니다. */
	FGameplayEffectSpecHandle MakeEffectSpec(EAgEffect Effect, float Level, FGameplayEffectContextHandle Context) const;
	FGameplayEffectSpecHandle MakeEffectSpec(const UGameplayEffect& Effect, float Level, FGameplayEffectContextHandle Context) const;
};
