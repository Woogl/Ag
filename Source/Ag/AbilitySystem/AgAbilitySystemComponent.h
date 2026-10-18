// Copyright Woogle. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AgAbilitySystemComponent.generated.h"

class UAnimMontage;
class UGameplayEffect;
enum class EAgEffect : uint8;
struct FAnimMontageInstance;

/** 스펙별 동적 태그의 차단·취소와 검색을 엔진 GAS에 연결하고, Effects.json의 효과로 스펙을 만들며, 행동의 몽타주를 재생합니다. */
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
	 * 스펙 태그(GetDynamicSpecSourceTags)에 Tag가 정확히 있는 스펙. 없으면 null입니다. 입력 계층과 HUD가 행동 ID 대신 행동 태그로
	 * 행동을 찾습니다. 엔진 TryActivateAbilitiesByTag는 클래스 태그를 보므로 쓰지 않습니다.
	 */
	FGameplayAbilitySpec* FindSpecByTag(const FGameplayTag& Tag);
	const FGameplayAbilitySpec* FindSpecByTag(const FGameplayTag& Tag) const;

	/**
	 * 엔진 PlayMontage처럼 Ability를 재생 중인 어빌리티로 두고 Montage를 처음부터(StartSection이 있으면 그 섹션부터) 재생하되, 블렌드 인
	 * 시간을 정합니다(음수면 몽타주의 값). 같은 그룹의 이전 재생은 그 블렌드로 끝납니다. 재생한 인스턴스이고, 실패하거나 이전 재생의
	 * 콜백이 Ability를 끝냈으면(새 재생은 멈춤) null입니다.
	 */
	FAnimMontageInstance* PlayMontageWithBlendIn(UGameplayAbility& Ability, UAnimMontage& Montage, float BlendInTime, FName StartSection);

	/**
	 * 엔진 MakeOutgoingSpec이 GE 클래스로 하는 일을 Effects.json의 효과로 합니다. Context가 비어 있으면 새로 만듭니다.
	 * 효과가 없으면 빈 핸들입니다.
	 */
	FGameplayEffectSpecHandle MakeEffectSpec(EAgEffect Effect, float Level, FGameplayEffectContextHandle Context) const;
	FGameplayEffectSpecHandle MakeEffectSpec(const UGameplayEffect& Effect, float Level, FGameplayEffectContextHandle Context) const;
};
