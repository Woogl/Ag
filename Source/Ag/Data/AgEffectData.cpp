// Copyright Woogle. All Rights Reserved.

#include "Data/AgEffectData.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/AgHitResultGameplayEffectComponent.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgJsonRead.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameplayEffectExecutionCalculation.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

namespace
{
	using namespace AgJsonRead;

	struct FRequiredShape
	{
		EAgEffect Effect;
		EGameplayEffectDurationType DurationPolicy;

		bool bDurationFromCode;

		bool bPeriodic;
		TArray<FGameplayTag> SetByCallerTags;
		FGameplayTag AssetTag;
		FGameplayTag GrantedTag;
	};

	/** 네이티브 태그는 모듈이 시작한 뒤에 쓸 수 있으므로 표를 부를 때 만듭니다. */
	TArray<FRequiredShape> RequiredShapes()
	{
		using EPolicy = EGameplayEffectDurationType;
		const TArray<FGameplayTag> Resources = {AgGameplayTags::SetByCaller_SP, AgGameplayTags::SetByCaller_MP,
			AgGameplayTags::SetByCaller_UP};
		return {
			{EAgEffect::Damage, EPolicy::Instant, false, false, {}, AgGameplayTags::Effect_Damage, FGameplayTag()},
			{EAgEffect::Resource, EPolicy::Instant, false, false, Resources, FGameplayTag(), FGameplayTag()},
			{EAgEffect::Cooldown, EPolicy::HasDuration, true, false, {}, AgGameplayTags::Effect_Cooldown, FGameplayTag()},
			{EAgEffect::Hitstop, EPolicy::HasDuration, false, false, {}, FGameplayTag(), AgGameplayTags::State_Hitstop},
			{EAgEffect::SPRegenDelay, EPolicy::HasDuration, false, false, {}, FGameplayTag(), AgGameplayTags::State_Regen_SPDelay},
			{EAgEffect::SPRegenDelayEmpty, EPolicy::HasDuration, false, false, {}, FGameplayTag(), AgGameplayTags::State_Regen_SPDelay},
			{EAgEffect::PPRegenDelay, EPolicy::HasDuration, false, false, {}, FGameplayTag(), AgGameplayTags::State_Regen_PPDelay},
			{EAgEffect::Kill, EPolicy::Instant, false, false, {}, FGameplayTag(), FGameplayTag()},
			{EAgEffect::PPRefill, EPolicy::Instant, false, false, {}, FGameplayTag(), FGameplayTag()},
			{EAgEffect::ParryPPLoss, EPolicy::Instant, false, false, {}, FGameplayTag(), FGameplayTag()},
			{EAgEffect::ParryReward, EPolicy::Instant, false, false, {}, FGameplayTag(), FGameplayTag()},
			{EAgEffect::PerfectDodgeReward, EPolicy::Instant, false, false, {}, FGameplayTag(), FGameplayTag()},
			{EAgEffect::DodgeCounterChance,
				EPolicy::HasDuration,
				false,
				false,
				{},
				FGameplayTag(),
				AgGameplayTags::State_DodgeCounterChance},
		};
	}

	template <typename TEnum>
	bool EnumValue(const FJsonObject& Object, const TCHAR* Key, const FString& Path, TEnum& Out, TArray<FString>& Errors)
	{
		FString Name;
		const int64 Value = Object.TryGetStringField(Key, Name) && !Name.EndsWith(TEXT("_MAX"))
		                        ? StaticEnum<TEnum>()->GetValueByNameString(Name)
		                        : INDEX_NONE;
		if (Value == INDEX_NONE)
		{
			Errors.Add(Path + TEXT(".") + Key + TEXT(": invalid enum"));
			return false;
		}
		Out = static_cast<TEnum>(Value);
		return true;
	}

	bool Attribute(const FJsonObject& Object, const TCHAR* Key, const FString& Path, FGameplayAttribute& Out, TArray<FString>& Errors)
	{
		FString Name;
		FProperty* Property =
			Object.TryGetStringField(Key, Name) ? FindFProperty<FProperty>(UAgAttributeSet::StaticClass(), FName(*Name)) : nullptr;
		if (!Property || !FGameplayAttribute::IsGameplayAttributeDataProperty(Property))
		{
			Errors.Add(Path + TEXT(".") + Key + TEXT(": unknown attribute ") + Name);
			return false;
		}
		Out = FGameplayAttribute(Property);
		return true;
	}

	UClass* ClassAt(const TSharedPtr<FJsonValue>& Value, const UClass* Base, const FString& Path, TArray<FString>& Errors)
	{
		const FString ClassPath = Value->Type == EJson::String ? Value->AsString() : FString();
		UClass* Class = ClassPath.IsEmpty() ? nullptr : LoadClass<UObject>(nullptr, *ClassPath, nullptr, LOAD_NoWarn);
		if (!Class || !Class->IsChildOf(Base) || Class->HasAnyClassFlags(CLASS_Abstract) || Class->GetPathName() != ClassPath)
		{
			Errors.Add(Path + TEXT(": missing class, wrong type or redirected path ") + ClassPath);
			return nullptr;
		}
		return Class;
	}

	bool Modifier(const FJsonObject& Object, const FString& Path, float Period, FGameplayModifierInfo& Out, TArray<FString>& Errors)
	{
		const int32 Before = Errors.Num();
		Keys(Object, Path, {TEXT("Attribute"), TEXT("Op")}, {TEXT("Value"), TEXT("PerSecond"), TEXT("SetByCaller"), TEXT("FromAttribute")},
			Errors);
		Attribute(Object, TEXT("Attribute"), Path, Out.Attribute, Errors);
		FString Op;
		Object.TryGetStringField(TEXT("Op"), Op);
		if (Op == TEXT("Add"))
		{
			Out.ModifierOp = EGameplayModOp::AddBase;
		}
		else if (Op == TEXT("Override"))
		{
			Out.ModifierOp = EGameplayModOp::Override;
		}
		else
		{
			Errors.Add(Path + TEXT(".Op: expected Add or Override"));
		}
		int32 Magnitudes = 0;
		for (const TCHAR* Key : {TEXT("Value"), TEXT("PerSecond"), TEXT("SetByCaller"), TEXT("FromAttribute")})
		{
			Magnitudes += Object.HasField(Key) ? 1 : 0;
		}
		if (Magnitudes != 1)
		{
			Errors.Add(Path + TEXT(": needs exactly one of Value, PerSecond, SetByCaller, FromAttribute"));
			return false;
		}
		float Number = 0.f;
		if (Object.HasField(TEXT("Value")))
		{
			if (AgJsonRead::Number(Object, TEXT("Value"), Path, Number, Errors, true))
			{
				Out.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Number));
			}
		}
		else if (Object.HasField(TEXT("PerSecond")))
		{
			if (Period <= 0.f)
			{
				Errors.Add(Path + TEXT(".PerSecond: needs Period"));
			}
			else if (AgJsonRead::Number(Object, TEXT("PerSecond"), Path, Number, Errors, true))
			{
				Out.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Number * Period));
			}
		}
		else if (Object.HasField(TEXT("SetByCaller")))
		{
			FString TagName;
			Object.TryGetStringField(TEXT("SetByCaller"), TagName);
			FSetByCallerFloat SetByCaller;
			SetByCaller.DataTag = FGameplayTag::RequestGameplayTag(FName(*TagName), false);
			if (!SetByCaller.DataTag.IsValid())
			{
				Errors.Add(Path + TEXT(".SetByCaller: unknown native tag ") + TagName);
			}
			Out.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
		}
		else
		{
			FGameplayAttribute Backing;
			if (Attribute(Object, TEXT("FromAttribute"), Path, Backing, Errors))
			{
				FAttributeBasedFloat AttributeBased;
				AttributeBased.BackingAttribute =
					FGameplayEffectAttributeCaptureDefinition(Backing, EGameplayEffectAttributeCaptureSource::Target, false);
				Out.ModifierMagnitude = FGameplayEffectModifierMagnitude(AttributeBased);
			}
		}
		return Errors.Num() == Before;
	}

	bool OptionalTags(const FJsonObject& Object, const TCHAR* Key, const FString& Path, FGameplayTagContainer& Out, TArray<FString>& Errors)
	{
		return !Object.HasField(Key) || Tags(Object, Key, Path, Out, Errors);
	}

	UGameplayEffect* Build(const FJsonObject& Entry, UObject& Outer, FName Id, int32 Version, const FString& Path, TArray<FString>& Errors)
	{
		const int32 Before = Errors.Num();
		Keys(Entry, Path, {TEXT("DurationPolicy")},
			{TEXT("Duration"), TEXT("Period"), TEXT("ExecutePeriodicOnApplication"), TEXT("PeriodicInhibitionPolicy"), TEXT("Modifiers"),
				TEXT("Executions"), TEXT("AssetTags"), TEXT("GrantedTags"), TEXT("ApplicationIgnoredTags"), TEXT("OngoingRequiredTags"),
				TEXT("OngoingIgnoredTags"), TEXT("Components")},
			Errors);
		EGameplayEffectDurationType Policy = EGameplayEffectDurationType::Instant;
		EnumValue(Entry, TEXT("DurationPolicy"), Path, Policy, Errors);
		// 즉시 효과는 걸려 있지 않으므로 주기, 부여 태그, 지속 조건을 쓸 수 없습니다.
		if (Policy == EGameplayEffectDurationType::Instant)
		{
			for (const TCHAR* Key : {TEXT("Period"), TEXT("ExecutePeriodicOnApplication"), TEXT("PeriodicInhibitionPolicy"),
					 TEXT("GrantedTags"), TEXT("OngoingRequiredTags"), TEXT("OngoingIgnoredTags")})
			{
				if (Entry.HasField(Key))
				{
					Errors.Add(Path + TEXT(".") + Key + TEXT(": not for Instant"));
				}
			}
		}
		if (Entry.HasField(TEXT("Duration")) && Policy != EGameplayEffectDurationType::HasDuration)
		{
			Errors.Add(Path + TEXT(".Duration: only for HasDuration"));
		}
		if ((Entry.HasField(TEXT("ExecutePeriodicOnApplication")) || Entry.HasField(TEXT("PeriodicInhibitionPolicy"))) &&
			!Entry.HasField(TEXT("Period")))
		{
			Errors.Add(Path + TEXT(": periodic settings need Period"));
		}

		UGameplayEffect* Effect = NewObject<UGameplayEffect>(&Outer, FName(*FString::Printf(TEXT("%s_%d"), *Id.ToString(), Version)));
		Effect->DurationPolicy = Policy;
		float Seconds = 0.f;
		if (Entry.HasField(TEXT("Duration")) && AgJsonRead::Number(Entry, TEXT("Duration"), Path, Seconds, Errors))
		{
			if (Seconds <= 0.f)
			{
				Errors.Add(Path + TEXT(".Duration: must be positive"));
			}
			Effect->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Seconds));
		}
		if (Entry.HasField(TEXT("Period")) && AgJsonRead::Number(Entry, TEXT("Period"), Path, Seconds, Errors))
		{
			if (Seconds <= 0.f)
			{
				Errors.Add(Path + TEXT(".Period: must be positive"));
			}
			Effect->Period = FScalableFloat(Seconds);
		}
		if (Entry.HasField(TEXT("ExecutePeriodicOnApplication")) &&
			!Entry.TryGetBoolField(TEXT("ExecutePeriodicOnApplication"), Effect->bExecutePeriodicEffectOnApplication))
		{
			Errors.Add(Path + TEXT(".ExecutePeriodicOnApplication: expected boolean"));
		}
		if (Entry.HasField(TEXT("PeriodicInhibitionPolicy")))
		{
			EnumValue(Entry, TEXT("PeriodicInhibitionPolicy"), Path, Effect->PeriodicInhibitionPolicy, Errors);
		}
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (Entry.HasField(TEXT("Modifiers")))
		{
			if (!Entry.TryGetArrayField(TEXT("Modifiers"), Values))
			{
				Errors.Add(Path + TEXT(".Modifiers: expected array"));
			}
			else
			{
				for (int32 Index = 0; Index < Values->Num(); ++Index)
				{
					const FString Field = FString::Printf(TEXT("%s.Modifiers[%d]"), *Path, Index);
					const TSharedPtr<FJsonObject>* Object = nullptr;
					FGameplayModifierInfo Info;
					if (!(*Values)[Index]->TryGetObject(Object))
					{
						Errors.Add(Field + TEXT(": expected object"));
					}
					else if (Modifier(**Object, Field, Effect->Period.Value, Info, Errors))
					{
						Effect->Modifiers.Add(Info);
					}
				}
			}
		}
		if (Entry.HasField(TEXT("Executions")))
		{
			if (!Entry.TryGetArrayField(TEXT("Executions"), Values))
			{
				Errors.Add(Path + TEXT(".Executions: expected array"));
			}
			else
			{
				for (int32 Index = 0; Index < Values->Num(); ++Index)
				{
					const FString Field = FString::Printf(TEXT("%s.Executions[%d]"), *Path, Index);
					if (UClass* Class = ClassAt((*Values)[Index], UGameplayEffectExecutionCalculation::StaticClass(), Field, Errors))
					{
						Effect->Executions.AddDefaulted_GetRef().CalculationClass = Class;
					}
				}
			}
		}

		FGameplayTagContainer AssetTags;
		FGameplayTagContainer GrantedTags;
		FGameplayTagContainer ApplicationIgnoredTags;
		FGameplayTagContainer OngoingRequiredTags;
		FGameplayTagContainer OngoingIgnoredTags;
		OptionalTags(Entry, TEXT("AssetTags"), Path, AssetTags, Errors);
		OptionalTags(Entry, TEXT("GrantedTags"), Path, GrantedTags, Errors);
		OptionalTags(Entry, TEXT("ApplicationIgnoredTags"), Path, ApplicationIgnoredTags, Errors);
		OptionalTags(Entry, TEXT("OngoingRequiredTags"), Path, OngoingRequiredTags, Errors);
		OptionalTags(Entry, TEXT("OngoingIgnoredTags"), Path, OngoingIgnoredTags, Errors);
		if (!AssetTags.IsEmpty())
		{
			FInheritedTagContainer Changes;
			Changes.Added = AssetTags;
			Effect->AddComponent<UAssetTagsGameplayEffectComponent>().SetAndApplyAssetTagChanges(Changes);
		}
		if (!GrantedTags.IsEmpty())
		{
			FInheritedTagContainer Changes;
			Changes.Added = GrantedTags;
			Effect->AddComponent<UTargetTagsGameplayEffectComponent>().SetAndApplyTargetTagChanges(Changes);
		}
		if (!ApplicationIgnoredTags.IsEmpty() || !OngoingRequiredTags.IsEmpty() || !OngoingIgnoredTags.IsEmpty())
		{
			UTargetTagRequirementsGameplayEffectComponent& Requirements =
				Effect->AddComponent<UTargetTagRequirementsGameplayEffectComponent>();
			Requirements.ApplicationTagRequirements.IgnoreTags = ApplicationIgnoredTags;
			Requirements.OngoingTagRequirements.RequireTags = OngoingRequiredTags;
			Requirements.OngoingTagRequirements.IgnoreTags = OngoingIgnoredTags;
		}
		if (Entry.HasField(TEXT("Components")))
		{
			if (!Entry.TryGetArrayField(TEXT("Components"), Values))
			{
				Errors.Add(Path + TEXT(".Components: expected array"));
			}
			else
			{
				for (int32 Index = 0; Index < Values->Num(); ++Index)
				{
					const FString Field = FString::Printf(TEXT("%s.Components[%d]"), *Path, Index);
					UClass* Class = ClassAt((*Values)[Index], UGameplayEffectComponent::StaticClass(), Field, Errors);
					// 엔진은 컴포넌트 목록을 AddComponent<T>로만 열어 두므로 프로젝트 컴포넌트를 클래스별로 더합니다.
					if (Class == UAgHitResultGameplayEffectComponent::StaticClass())
					{
						Effect->AddComponent<UAgHitResultGameplayEffectComponent>();
					}
					else if (Class)
					{
						Errors.Add(Field + TEXT(": unsupported component ") + Class->GetPathName());
					}
				}
			}
		}

		// 에셋은 PostLoad가 부르는 함수로, 컴포넌트의 태그를 GE에 모읍니다.
		Effect->OnGameplayEffectChanged();
#if WITH_EDITOR
		FDataValidationContext Context;
		Effect->IsDataValid(Context);
		TArray<FText> Warnings;
		TArray<FText> Issues;
		Context.SplitIssues(Warnings, Issues);
		Issues.Append(Warnings);
		for (const FText& Issue : Issues)
		{
			Errors.Add(Path + TEXT(": ") + Issue.ToString());
		}
#endif
		return Errors.Num() == Before ? Effect : nullptr;
	}

	void ValidateRequired(const TMap<FName, TObjectPtr<UGameplayEffect>>& Effects, TArray<FString>& Errors)
	{
		for (const FRequiredShape& Shape : RequiredShapes())
		{
			const FName Id = AgEffectData::Id(Shape.Effect);
			const TObjectPtr<UGameplayEffect>* Found = Effects.Find(Id);
			if (!Found)
			{
				Errors.Add(Id.ToString() + TEXT(": required by C++"));
				continue;
			}
			const UGameplayEffect& Effect = **Found;
			const FString Path = Id.ToString();
			if (Effect.DurationPolicy != Shape.DurationPolicy)
			{
				Errors.Add(Path + TEXT(".DurationPolicy: C++ needs ") +
						   StaticEnum<EGameplayEffectDurationType>()->GetNameStringByValue(static_cast<int64>(Shape.DurationPolicy)));
			}
			float Duration = 0.f;
			Effect.DurationMagnitude.GetStaticMagnitudeIfPossible(1.f, Duration);
			if (Shape.DurationPolicy == EGameplayEffectDurationType::HasDuration && Shape.bDurationFromCode != (Duration <= 0.f))
			{
				Errors.Add(Path + (Shape.bDurationFromCode ? TEXT(".Duration: C++ sets the duration") : TEXT(".Duration: required")));
			}
			if (Shape.bPeriodic && Effect.Period.Value <= 0.f)
			{
				Errors.Add(Path + TEXT(".Period: required"));
			}
			for (const FGameplayTag& Tag : Shape.SetByCallerTags)
			{
				const bool bFound = Effect.Modifiers.ContainsByPredicate([&Tag](const FGameplayModifierInfo& Modifier)
				{
					return Modifier.ModifierMagnitude.GetMagnitudeCalculationType() ==
					           EGameplayEffectMagnitudeCalculation::SetByCaller &&
					       Modifier.ModifierMagnitude.GetSetByCallerFloat().DataTag == Tag;
				});
				if (!bFound)
				{
					Errors.Add(Path + TEXT(".Modifiers: C++ sets ") + Tag.ToString());
				}
			}
			if (Shape.AssetTag.IsValid() && !Effect.GetAssetTags().HasTagExact(Shape.AssetTag))
			{
				Errors.Add(Path + TEXT(".AssetTags: C++ looks for ") + Shape.AssetTag.ToString());
			}
			if (Shape.GrantedTag.IsValid() && !Effect.GetGrantedTags().HasTagExact(Shape.GrantedTag))
			{
				Errors.Add(Path + TEXT(".GrantedTags: C++ relies on ") + Shape.GrantedTag.ToString());
			}
		}
	}
} // namespace

FName AgEffectData::Id(EAgEffect Effect)
{
	return FName(*StaticEnum<EAgEffect>()->GetNameStringByValue(static_cast<int64>(Effect)));
}

bool AgEffectData::Parse(const FJsonObject& Json, UObject& Outer, int32 Version, TMap<FName, TObjectPtr<UGameplayEffect>>& Out, TArray<FString>& Errors)
{
	const int32 Before = Errors.Num();
	TMap<FName, TObjectPtr<UGameplayEffect>> Candidate;
	TSet<FName> Ids;
	for (const TPair<FJsonObject::FStringType, TSharedPtr<FJsonValue>>& Pair : Json.Values)
	{
		const FString Key(Pair.Key);
		const FName EffectId(*Key);
		if (EffectId.IsNone() || Ids.Contains(EffectId))
		{
			Errors.Add(Key + TEXT(": empty or duplicate effect ID"));
			continue;
		}
		Ids.Add(EffectId);
		if (const FObject Entry = ObjectAt(Json, Key, Key, Errors))
		{
			if (UGameplayEffect* Effect = Build(*Entry, Outer, EffectId, Version, Key, Errors))
			{
				Candidate.Add(EffectId, Effect);
			}
		}
	}
	if (Errors.Num() == Before)
	{
		ValidateRequired(Candidate, Errors);
	}
	if (Errors.Num() != Before)
	{
		return false;
	}
	Out = MoveTemp(Candidate);
	return true;
}
