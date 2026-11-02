// Copyright Woogle. All Rights Reserved.

#include "Character/AgSandbagCharacter.h"

#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Core/AgAssetManager.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgEffectData.h"

namespace
{
	bool IsPattern(const UGameplayAbility* Ability)
	{
		const UAgGameplayAbility* AgAbility = Cast<UAgGameplayAbility>(Ability);
		return AgAbility && AgAbility->GetDefinition().AbilityTags.HasTag(AgGameplayTags::Ability_Enemy_Pattern);
	}
} // namespace

AAgSandbagCharacter::AAgSandbagCharacter()
{
	Team = EAgTeam::Neutral;
}

void AAgSandbagCharacter::BeginPlay()
{
	Super::BeginPlay();

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	ASC->AbilityActivatedCallbacks.AddUObject(this, &ThisClass::HandlePatternActivated);
	ASC->OnAbilityEnded.AddUObject(this, &ThisClass::HandlePatternEnded);
}

void AAgSandbagCharacter::HandleDeathEvent(const FGameplayEventData* Payload)
{
	// 사망 처리 없이 HP만 되돌립니다. 사망 태그가 붙지 않으므로 대미지 결과 처리가 이 타격의 반응을 그대로 정합니다.
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	ASC->ApplyGameplayEffectToSelf(UAgAssetManager::Get().GetEffect(EAgEffect::HPRefill), 1.f, ASC->MakeEffectContext());
}

void AAgSandbagCharacter::HandlePatternActivated(UGameplayAbility* Ability)
{
	// 놓인 자리에서 공격하도록 모션의 이동만 끕니다. 회전 보정과 피격 반응의 이동은 그대로 받습니다.
	if (IsPattern(Ability))
	{
		SetAnimRootMotionTranslationScale(0.f);
	}
}

void AAgSandbagCharacter::HandlePatternEnded(const FAbilityEndedData& Data)
{
	if (IsPattern(Data.AbilityThatEnded))
	{
		SetAnimRootMotionTranslationScale(1.f);
	}
}
