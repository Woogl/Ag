// Copyright Woogle. All Rights Reserved.

#include "Data/AgEffectData.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/AgDamageExecution.h"
#include "AbilitySystem/AgDamageResultGameplayEffectComponent.h"
#include "AbilitySystem/AgFallDamageMagnitude.h"
#include "AbilitySystem/AgRegenDelayGameplayEffectComponent.h"
#include "AbilitySystem/AgSlowMotionGameplayEffectComponent.h"
#include "Core/AgGameplayTags.h"
#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

namespace AgEffectDataPrivate
{
	constexpr float PeriodicEffectInterval = 0.1f;

	void AddModifier(UGameplayEffect& Effect, FGameplayAttribute Attribute, EGameplayModOp::Type Op, const FGameplayEffectModifierMagnitude& Magnitude)
	{
		FGameplayModifierInfo& Modifier = Effect.Modifiers.AddDefaulted_GetRef();
		Modifier.Attribute = Attribute;
		Modifier.ModifierOp = Op;
		Modifier.ModifierMagnitude = Magnitude;
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

	FCustomCalculationBasedFloat MakeCustomCalculation(TSubclassOf<UGameplayModMagnitudeCalculation> CalculationClass)
	{
		FCustomCalculationBasedFloat CustomCalculation;
		CustomCalculation.CalculationClassMagnitude = CalculationClass;
		return CustomCalculation;
	}

	FGameplayEffectModifierMagnitude MakeMagnitude(const FAgEffectModifier& Modifier)
	{
		if (Modifier.PerSecond)
		{
			// 기획서의 초당 값을 그대로 적도록, 주기마다 더할 값으로 바꿉니다.
			return FGameplayEffectModifierMagnitude(FScalableFloat(*Modifier.PerSecond * PeriodicEffectInterval));
		}
		if (Modifier.Multiplier)
		{
			return FGameplayEffectModifierMagnitude(FScalableFloat(*Modifier.Multiplier));
		}
		if (Modifier.Coefficient)
		{
			return FGameplayEffectModifierMagnitude(MakeTargetAttribute(Modifier.BackingAttribute, *Modifier.Coefficient));
		}
		return FGameplayEffectModifierMagnitude(FScalableFloat(Modifier.Value.Get(0.f)));
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
} // namespace AgEffectDataPrivate

const UGameplayEffect* UAgEffectSet::FindEffect(FName Id)
{
	return UAgAssetManager::Get().GetData<UAgEffectSet>().Effects.FindRef(Id);
}

const UGameplayEffect* UAgEffectSet::GetEffect(EAgEffect Effect)
{
	return UAgAssetManager::Get().GetData<UAgEffectSet>().NativeEffects.FindChecked(Effect).Get();
}

bool UAgEffectSet::ReadJson(FAgJsonReader& Reader, FName Key)
{
	return ReadJsonInto(Reader, GET_MEMBER_NAME_CHECKED(UAgEffectSet, Definitions));
}

void UAgEffectSet::CopyJson(const UAgJsonData& Loaded)
{
	Super::CopyJson(Loaded);

	// 걸려 있는 효과가 옛 GE를 그대로 쓰도록 다시 읽을 때마다 새 객체로 만듭니다.
	Effects.Reset();
	for (const TPair<FName, FAgEffectDefinition>& Pair : Definitions)
	{
		Effects.Add(Pair.Key, AgEffectData::MakeEffect(Pair.Value, *this));
	}
	// 코드 전용 효과는 JSON과 관계없으므로 한 번만 만듭니다.
	if (NativeEffects.IsEmpty())
	{
		const UEnum* NativeEffectEnum = StaticEnum<EAgEffect>();
		// 마지막 값은 UHT가 붙이는 _MAX입니다.
		for (int32 Index = 0; Index < NativeEffectEnum->NumEnums() - 1; ++Index)
		{
			const EAgEffect Effect = static_cast<EAgEffect>(NativeEffectEnum->GetValueByIndex(Index));
			NativeEffects.Add(Effect, AgEffectData::MakeNativeEffect(Effect, *this));
		}
	}
}

UGameplayEffect* AgEffectData::MakeNativeEffect(EAgEffect Id, UObject& Outer)
{
	UGameplayEffect* Effect = NewObject<UGameplayEffect>(&Outer);
	switch (Id)
	{
		case EAgEffect::Damage:
			AgEffectDataPrivate::AddAssetTag(*Effect, AgGameplayTags::Effect_Damage);
			Effect->AddComponent<UTargetTagRequirementsGameplayEffectComponent>().ApplicationTagRequirements.IgnoreTags.AddTag(AgGameplayTags::State_Dead);
			Effect->Executions.AddDefaulted_GetRef().CalculationClass = UAgDamageExecution::StaticClass();
			Effect->AddComponent<UAgDamageResultGameplayEffectComponent>();
			break;
		case EAgEffect::Resource:
			AgEffectDataPrivate::AddModifier(*Effect, UAgAttributeSet::GetMPAttribute(), EGameplayModOp::AddBase, AgEffectDataPrivate::MakeSetByCaller(AgGameplayTags::SetByCaller_MP));
			AgEffectDataPrivate::AddModifier(*Effect, UAgAttributeSet::GetUPAttribute(), EGameplayModOp::AddBase, AgEffectDataPrivate::MakeSetByCaller(AgGameplayTags::SetByCaller_UP));
			break;
		case EAgEffect::Cooldown:
			Effect->DurationPolicy = EGameplayEffectDurationType::HasDuration;
			AgEffectDataPrivate::AddAssetTag(*Effect, AgGameplayTags::Effect_Cooldown);
			break;
		case EAgEffect::Kill:
			Effect->AddComponent<UTargetTagRequirementsGameplayEffectComponent>().ApplicationTagRequirements.IgnoreTags.AddTag(AgGameplayTags::State_Dead);
			AgEffectDataPrivate::AddModifier(*Effect, UAgAttributeSet::GetHPAttribute(), EGameplayModOp::Override, FScalableFloat(0.f));
			break;
		case EAgEffect::Hitstop:
			Effect->DurationPolicy = EGameplayEffectDurationType::HasDuration;
			AgEffectDataPrivate::AddGrantedTags(*Effect, FGameplayTagContainer(AgGameplayTags::State_Hitstop));
			break;
		case EAgEffect::PPRefill:
			AgEffectDataPrivate::AddModifier(*Effect, UAgAttributeSet::GetPPAttribute(), EGameplayModOp::Override, AgEffectDataPrivate::MakeTargetAttribute(UAgAttributeSet::GetMaxPPAttribute(), 1.f));
			break;
		case EAgEffect::FallDamage:
			Effect->AddComponent<UTargetTagRequirementsGameplayEffectComponent>().ApplicationTagRequirements.IgnoreTags.AddTag(AgGameplayTags::State_Dead);
			AgEffectDataPrivate::AddModifier(*Effect, UAgAttributeSet::GetHPAttribute(), EGameplayModOp::AddBase, AgEffectDataPrivate::MakeCustomCalculation(UAgFallDamageMagnitude::StaticClass()));
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
		Effect->Period = FScalableFloat(AgEffectDataPrivate::PeriodicEffectInterval);
		Effect->bExecutePeriodicEffectOnApplication = false;
		Effect->PeriodicInhibitionPolicy = EGameplayEffectPeriodInhibitionRemovedPolicy::ResetPeriod;
	}
	for (const FAgEffectModifier& Modifier : Definition.Modifiers)
	{
		const EGameplayModOp::Type Op = Modifier.Multiplier ? EGameplayModOp::MultiplyCompound : EGameplayModOp::AddBase;
		AgEffectDataPrivate::AddModifier(*Effect, Modifier.Attribute, Op, AgEffectDataPrivate::MakeMagnitude(Modifier));
	}
	if (!Definition.GrantedTags.IsEmpty())
	{
		AgEffectDataPrivate::AddGrantedTags(*Effect, Definition.GrantedTags);
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

void AgEffectData::SetResourceAmounts(FGameplayEffectSpec& Spec, float MP, float UP)
{
	Spec.SetSetByCallerMagnitude(AgGameplayTags::SetByCaller_MP, MP);
	Spec.SetSetByCallerMagnitude(AgGameplayTags::SetByCaller_UP, UP);
}
