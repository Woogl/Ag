// Copyright Woogle. All Rights Reserved.

#include "Data/AgEffectData.h"

#include "AbilitySystem/AgHitResultGameplayEffectComponent.h"
#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameplayEffectExecutionCalculation.h"

namespace
{
	FGameplayEffectModifierMagnitude MakeMagnitude(const FAgEffectModifier& Modifier, float Period)
	{
		if (Modifier.Value)
		{
			return FGameplayEffectModifierMagnitude(FScalableFloat(*Modifier.Value));
		}
		if (Modifier.PerSecond)
		{
			// 기획서의 초당 값을 그대로 적도록, 주기마다 더할 값으로 바꿉니다.
			return FGameplayEffectModifierMagnitude(FScalableFloat(*Modifier.PerSecond * Period));
		}
		if (Modifier.SetByCaller.IsValid())
		{
			FSetByCallerFloat SetByCaller;
			SetByCaller.DataTag = Modifier.SetByCaller;
			return FGameplayEffectModifierMagnitude(SetByCaller);
		}
		FAttributeBasedFloat AttributeBased;
		AttributeBased.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(FGameplayAttribute(Modifier.FromAttribute.Get()),
			EGameplayEffectAttributeCaptureSource::Target, false);
		return FGameplayEffectModifierMagnitude(AttributeBased);
	}
} // namespace

FName AgEffectData::Id(EAgEffect Effect)
{
	return FName(*StaticEnum<EAgEffect>()->GetNameStringByValue(static_cast<int64>(Effect)));
}

void AgEffectData::Build(const TMap<FName, FAgEffectDefinition>& Definitions, UObject& Outer, int32 Version, TMap<FName, TObjectPtr<UGameplayEffect>>& Out)
{
	for (const TPair<FName, FAgEffectDefinition>& Pair : Definitions)
	{
		const FAgEffectDefinition& Definition = Pair.Value;
		UGameplayEffect* Effect = NewObject<UGameplayEffect>(&Outer, FName(*FString::Printf(TEXT("%s_%d"), *Pair.Key.ToString(), Version)));
		Effect->DurationPolicy = Definition.DurationPolicy;
		if (Definition.Duration)
		{
			Effect->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(*Definition.Duration));
		}
		if (Definition.Period)
		{
			Effect->Period = FScalableFloat(*Definition.Period);
		}
		if (Definition.ExecutePeriodicOnApplication)
		{
			Effect->bExecutePeriodicEffectOnApplication = *Definition.ExecutePeriodicOnApplication;
		}
		if (Definition.PeriodicInhibitionPolicy)
		{
			Effect->PeriodicInhibitionPolicy = *Definition.PeriodicInhibitionPolicy;
		}
		for (const FAgEffectModifier& Modifier : Definition.Modifiers)
		{
			FGameplayModifierInfo& Info = Effect->Modifiers.AddDefaulted_GetRef();
			Info.Attribute = FGameplayAttribute(Modifier.Attribute.Get());
			Info.ModifierOp = Modifier.Op == EAgModifierOp::Override ? EGameplayModOp::Override : EGameplayModOp::AddBase;
			Info.ModifierMagnitude = MakeMagnitude(Modifier, Effect->Period.Value);
		}
		for (const TSubclassOf<UGameplayEffectExecutionCalculation>& Execution : Definition.Executions)
		{
			Effect->Executions.AddDefaulted_GetRef().CalculationClass = Execution;
		}
		if (!Definition.AssetTags.IsEmpty())
		{
			FInheritedTagContainer Changes;
			Changes.Added = Definition.AssetTags;
			Effect->AddComponent<UAssetTagsGameplayEffectComponent>().SetAndApplyAssetTagChanges(Changes);
		}
		if (!Definition.GrantedTags.IsEmpty())
		{
			FInheritedTagContainer Changes;
			Changes.Added = Definition.GrantedTags;
			Effect->AddComponent<UTargetTagsGameplayEffectComponent>().SetAndApplyTargetTagChanges(Changes);
		}
		if (!Definition.ApplicationIgnoredTags.IsEmpty() || !Definition.OngoingRequiredTags.IsEmpty() ||
			!Definition.OngoingIgnoredTags.IsEmpty())
		{
			UTargetTagRequirementsGameplayEffectComponent& Requirements =
				Effect->AddComponent<UTargetTagRequirementsGameplayEffectComponent>();
			Requirements.ApplicationTagRequirements.IgnoreTags = Definition.ApplicationIgnoredTags;
			Requirements.OngoingTagRequirements.RequireTags = Definition.OngoingRequiredTags;
			Requirements.OngoingTagRequirements.IgnoreTags = Definition.OngoingIgnoredTags;
		}
		// 엔진은 컴포넌트 목록을 AddComponent<T>로만 열어 두므로, Components는 이 클래스만 받습니다.
		if (!Definition.Components.IsEmpty())
		{
			Effect->AddComponent<UAgHitResultGameplayEffectComponent>();
		}

		// 에셋은 PostLoad가 부르는 함수로, 컴포넌트의 태그를 GE에 모읍니다.
		Effect->OnGameplayEffectChanged();
		Out.Add(Pair.Key, Effect);
	}
}
