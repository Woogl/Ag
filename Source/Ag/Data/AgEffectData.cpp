// Copyright Woogle. All Rights Reserved.

#include "Data/AgEffectData.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/AgDamageExecution.h"
#include "AbilitySystem/AgDamageResultGameplayEffectComponent.h"
#include "AbilitySystem/AgRegenDelayGameplayEffectComponent.h"
#include "AbilitySystem/AgSlowMotionGameplayEffectComponent.h"
#include "Core/AgGameplayTags.h"
#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

namespace
{
	constexpr float PeriodicEffectInterval = 0.1f;

	FGameplayModifierInfo& AddModifier(UGameplayEffect& Effect, FGameplayAttribute Attribute, EGameplayModOp::Type Op, const FGameplayEffectModifierMagnitude& Magnitude)
	{
		FGameplayModifierInfo& Modifier = Effect.Modifiers.AddDefaulted_GetRef();
		Modifier.Attribute = Attribute;
		Modifier.ModifierOp = Op;
		Modifier.ModifierMagnitude = Magnitude;
		return Modifier;
	}

	FAttributeBasedFloat MakeTargetAttribute(FGameplayAttribute Attribute, float Coefficient)
	{
		FAttributeBasedFloat AttributeBased;
		AttributeBased.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(Attribute, EGameplayEffectAttributeCaptureSource::Target, false);
		AttributeBased.Coefficient = FScalableFloat(Coefficient);
		return AttributeBased;
	}

	FSetByCallerFloat MakeSetByCaller(FGameplayTag Tag)
	{
		FSetByCallerFloat SetByCaller;
		SetByCaller.DataTag = Tag;
		return SetByCaller;
	}

	FGameplayEffectModifierMagnitude MakeMagnitude(const FAgEffectModifier& Modifier)
	{
		if (Modifier.Value)
		{
			return FGameplayEffectModifierMagnitude(FScalableFloat(*Modifier.Value));
		}
		if (Modifier.PerSecond)
		{
			// 기획서의 초당 값을 그대로 적도록, 주기마다 더할 값으로 바꿉니다.
			return FGameplayEffectModifierMagnitude(FScalableFloat(*Modifier.PerSecond * PeriodicEffectInterval));
		}
		if (Modifier.Multiplier)
		{
			return FGameplayEffectModifierMagnitude(FScalableFloat(*Modifier.Multiplier));
		}
		return FGameplayEffectModifierMagnitude(MakeTargetAttribute(Modifier.FromAttribute, Modifier.Coefficient));
	}

	void AddAssetTag(UGameplayEffect& Effect, FGameplayTag Tag)
	{
		FInheritedTagContainer Changes;
		Changes.Added.AddTag(Tag);
		Effect.AddComponent<UAssetTagsGameplayEffectComponent>().SetAndApplyAssetTagChanges(Changes);
	}

	void AddGrantedTags(UGameplayEffect& Effect, const FGameplayTagContainer& Tags)
	{
		FInheritedTagContainer Changes;
		Changes.Added = Tags;
		Effect.AddComponent<UTargetTagsGameplayEffectComponent>().SetAndApplyTargetTagChanges(Changes);
	}
} // namespace

UGameplayEffect* AgEffectData::MakeNativeEffect(EAgEffect Id, UObject& Outer)
{
	UGameplayEffect* Effect = NewObject<UGameplayEffect>(&Outer);
	switch (Id)
	{
		case EAgEffect::Damage:
			AddAssetTag(*Effect, AgGameplayTags::Effect_Damage);
			Effect->AddComponent<UTargetTagRequirementsGameplayEffectComponent>().ApplicationTagRequirements.IgnoreTags.AddTag(AgGameplayTags::State_Dead);
			Effect->Executions.AddDefaulted_GetRef().CalculationClass = UAgDamageExecution::StaticClass();
			Effect->AddComponent<UAgDamageResultGameplayEffectComponent>();
			break;
		case EAgEffect::Resource:
			AddModifier(*Effect, UAgAttributeSet::GetSPAttribute(), EGameplayModOp::AddBase, MakeSetByCaller(AgGameplayTags::SetByCaller_SP));
			AddModifier(*Effect, UAgAttributeSet::GetMPAttribute(), EGameplayModOp::AddBase, MakeSetByCaller(AgGameplayTags::SetByCaller_MP));
			AddModifier(*Effect, UAgAttributeSet::GetUPAttribute(), EGameplayModOp::AddBase, MakeSetByCaller(AgGameplayTags::SetByCaller_UP));
			break;
		case EAgEffect::Cooldown:
			Effect->DurationPolicy = EGameplayEffectDurationType::HasDuration;
			AddAssetTag(*Effect, AgGameplayTags::Effect_Cooldown);
			break;
		case EAgEffect::Kill:
			AddModifier(*Effect, UAgAttributeSet::GetHPAttribute(), EGameplayModOp::Override, FScalableFloat(0.f));
			break;
		case EAgEffect::Hitstop:
			Effect->DurationPolicy = EGameplayEffectDurationType::HasDuration;
			AddGrantedTags(*Effect, FGameplayTagContainer(AgGameplayTags::State_Hitstop));
			break;
		case EAgEffect::PPRefill:
			AddModifier(*Effect, UAgAttributeSet::GetPPAttribute(), EGameplayModOp::Override, MakeTargetAttribute(UAgAttributeSet::GetMaxPPAttribute(), 1.f));
			break;
	}

	// 에셋은 PostLoad가 부르는 함수로, 컴포넌트의 태그를 GE에 모읍니다.
	Effect->OnGameplayEffectChanged();
	return Effect;
}

UGameplayEffect* AgEffectData::MakeEffect(const FAgEffectDefinition& Definition, UObject& Outer)
{
	UGameplayEffect* Effect = NewObject<UGameplayEffect>(&Outer);
	Effect->DurationPolicy = Definition.DurationPolicy;
	if (Definition.Duration)
	{
		Effect->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(*Definition.Duration));
	}
	if (Definition.Modifiers.ContainsByPredicate([](const FAgEffectModifier& Modifier)
	{ return Modifier.PerSecond.IsSet(); }))
	{
		Effect->Period = FScalableFloat(PeriodicEffectInterval);
		Effect->bExecutePeriodicEffectOnApplication = false;
		Effect->PeriodicInhibitionPolicy = EGameplayEffectPeriodInhibitionRemovedPolicy::ResetPeriod;
	}
	for (const FAgEffectModifier& Modifier : Definition.Modifiers)
	{
		const EGameplayModOp::Type Op = Modifier.Multiplier ? EGameplayModOp::MultiplyCompound : EGameplayModOp::AddBase;
		FGameplayModifierInfo& Info = AddModifier(*Effect, Modifier.Attribute, Op, MakeMagnitude(Modifier));
		Info.TargetTags.RequireTags = Modifier.RequiredTags;
		Info.TargetTags.IgnoreTags = Modifier.IgnoredTags;
	}
	if (!Definition.GrantedTags.IsEmpty())
	{
		AddGrantedTags(*Effect, Definition.GrantedTags);
	}
	if (!Definition.OngoingIgnoredTags.IsEmpty())
	{
		Effect->FindOrAddComponent<UTargetTagRequirementsGameplayEffectComponent>().OngoingTagRequirements.IgnoreTags = Definition.OngoingIgnoredTags;
	}
	if (!Definition.RemovalTags.IsEmpty())
	{
		// 엔진 RequireTags는 모두 있어야 지우므로, 하나라도 있으면 지우도록 쿼리를 씁니다.
		Effect->FindOrAddComponent<UTargetTagRequirementsGameplayEffectComponent>().RemovalTagRequirements.TagQuery =
			FGameplayTagQuery::MakeQuery_MatchAnyTags(Definition.RemovalTags);
	}
	if (Definition.RegenDelay)
	{
		Effect->AddComponent<UAgRegenDelayGameplayEffectComponent>().Delay = *Definition.RegenDelay;
	}
	if (Definition.SlowMotionDuration > 0.f)
	{
		UAgSlowMotionGameplayEffectComponent& SlowMotion = Effect->AddComponent<UAgSlowMotionGameplayEffectComponent>();
		SlowMotion.Speed = Definition.SlowMotionSpeed;
		SlowMotion.Duration = Definition.SlowMotionDuration;
	}

	Effect->OnGameplayEffectChanged();
	return Effect;
}

void AgEffectData::SetResourceAmounts(FGameplayEffectSpec& Spec, float SP, float MP, float UP)
{
	Spec.SetSetByCallerMagnitude(AgGameplayTags::SetByCaller_SP, SP);
	Spec.SetSetByCallerMagnitude(AgGameplayTags::SetByCaller_MP, MP);
	Spec.SetSetByCallerMagnitude(AgGameplayTags::SetByCaller_UP, UP);
}
