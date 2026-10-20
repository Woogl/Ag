// Copyright Woogle. All Rights Reserved.
#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Core/AgAssetManager.h"
#include "Data/AgEffectData.h"
#include "GameplayEffect.h"

void UAgAbilitySystemComponent::ApplyAbilityBlockAndCancelTags(const FGameplayTagContainer& AbilityTags, UGameplayAbility* RequestingAbility, bool bEnableBlockTags, const FGameplayTagContainer& BlockTags, bool bExecuteCancelTags, const FGameplayTagContainer& CancelTags)
{
	Super::ApplyAbilityBlockAndCancelTags(AbilityTags, RequestingAbility, bEnableBlockTags, BlockTags, false, CancelTags);
	if (bExecuteCancelTags)
	{
		CancelAbilitiesWithSpecTags(CancelTags, RequestingAbility);
	}
}

void UAgAbilitySystemComponent::CancelAbilitiesWithSpecTags(const FGameplayTagContainer& Tags, UGameplayAbility* Ignore)
{
	TArray<FGameplayAbilitySpecHandle> Handles;
	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.IsActive() && Spec.GetPrimaryInstance() != Ignore && Spec.GetDynamicSpecSourceTags().HasAny(Tags))
		{
			Handles.Add(Spec.Handle);
		}
	}
	for (const FGameplayAbilitySpecHandle Handle : Handles)
	{
		CancelAbilityHandle(Handle);
	}
}

FGameplayAbilitySpec* UAgAbilitySystemComponent::FindSpecByTag(const FGameplayTag& Tag)
{
	return const_cast<FGameplayAbilitySpec*>(std::as_const(*this).FindSpecByTag(Tag));
}

const FGameplayAbilitySpec* UAgAbilitySystemComponent::FindSpecByTag(const FGameplayTag& Tag) const
{
	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.GetDynamicSpecSourceTags().HasTagExact(Tag))
		{
			return &Spec;
		}
	}
	return nullptr;
}

FAnimMontageInstance* UAgAbilitySystemComponent::PlayMontageWithBlendIn(UGameplayAbility& Ability, UAnimMontage& Montage, float BlendInTime, FName StartSection)
{
	UAnimInstance* Anim = AbilityActorInfo.IsValid() ? AbilityActorInfo->GetAnimInstance() : nullptr;
	if (!Anim)
	{
		return nullptr;
	}
	FAlphaBlendArgs BlendIn = Montage.GetBlendInArgs();
	if (BlendInTime >= 0.f)
	{
		BlendIn.BlendTime = BlendInTime;
	}
	if (Anim->Montage_PlayWithBlendIn(&Montage, BlendIn) <= 0.f)
	{
		return nullptr;
	}

	// 같은 그룹의 이전 재생이 끊기며 부른 콜백이 이 어빌리티를 끝냈으면, 주인 없는 재생이 남지 않게 멈춥니다.
	FAnimMontageInstance* Instance = Anim->GetActiveInstanceForMontage(&Montage);
	if (!Instance || !Ability.IsActive())
	{
		if (Instance)
		{
			Instance->Stop(FAlphaBlend(Montage.BlendOut));
		}
		return nullptr;
	}

	// 엔진 PlayMontage처럼 재생 중인 어빌리티를 기록합니다. 캐릭터와 Task가 이 기록으로 재생 중인 어빌리티를 찾습니다.
	LocalAnimMontageInfo.AnimMontage = &Montage;
	LocalAnimMontageInfo.AnimatingAbility = &Ability;
	LocalAnimMontageInfo.PlayInstanceId =
		static_cast<uint8>(LocalAnimMontageInfo.PlayInstanceId < UINT8_MAX ? LocalAnimMontageInfo.PlayInstanceId + 1 : 0);
	Ability.SetCurrentMontage(&Montage);
	if (!StartSection.IsNone())
	{
		Anim->Montage_JumpToSection(StartSection, &Montage);
	}
	return Instance;
}

FGameplayEffectSpecHandle UAgAbilitySystemComponent::MakeEffectSpec(EAgEffect Effect, float Level, FGameplayEffectContextHandle Context) const
{
	const UGameplayEffect* Definition = UAgAssetManager::Get().GetEffect(Effect);
	return Definition ? MakeEffectSpec(*Definition, Level, Context) : FGameplayEffectSpecHandle();
}

FGameplayEffectSpecHandle UAgAbilitySystemComponent::MakeEffectSpec(const UGameplayEffect& Effect, float Level, FGameplayEffectContextHandle Context) const
{
	if (!Context.IsValid())
	{
		Context = MakeEffectContext();
	}
	return FGameplayEffectSpecHandle(new FGameplayEffectSpec(&Effect, Context, Level));
}
