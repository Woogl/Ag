// Copyright Woogle. All Rights Reserved.

#include "Data/AgAbilityData.h"
#include "AbilitySystem/AgTargeting.h"
#include "AbilitySystem/Tasks/AgAbilityTask_ApplyEffect.h"
#include "AbilitySystem/Tasks/AgAbilityTask_Dash.h"
#include "AbilitySystem/Tasks/AgAbilityTask_Launch.h"
#include "Character/AgPlayerCharacter.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "Data/AgJsonRead.h"
#include "Data/AgMontageTimeline.h"
#include "Animation/AnimMontage.h"
#include "Dom/JsonObject.h"
#include "GameplayEffect.h"
#include "JsonObjectConverter.h"
#include "TargetingSystem/TargetingPreset.h"
#include "Types/TargetingSystemTypes.h"
#include "UObject/UnrealType.h"

namespace
{
	using namespace AgJsonRead;

	void ReadActivation(const FJsonObject& Entry, FAgAbilityData& Out, const FString& Path, TArray<FString>& Errors)
	{
		const FString Field = Path + TEXT(".Activation");
		const FObject Activation = ObjectAt(Entry, TEXT("Activation"), Field, Errors);
		if (!Activation)
		{
			return;
		}
		Keys(*Activation, Field,
			{TEXT("RequiredTags"), TEXT("BlockedTags"), TEXT("OwnedTags"), TEXT("BlockTags"), TEXT("CancelTags"), TEXT("Triggers"),
				TEXT("bAllowRetrigger")},
			{}, Errors);
		Tags(*Activation, TEXT("RequiredTags"), Field, Out.Activation.RequiredTags, Errors);
		Tags(*Activation, TEXT("BlockedTags"), Field, Out.Activation.BlockedTags, Errors);
		Tags(*Activation, TEXT("OwnedTags"), Field, Out.Activation.OwnedTags, Errors);
		Tags(*Activation, TEXT("BlockTags"), Field, Out.Activation.BlockTags, Errors);
		Tags(*Activation, TEXT("CancelTags"), Field, Out.Activation.CancelTags, Errors);
		const TArray<TSharedPtr<FJsonValue>>* Triggers = nullptr;
		if (!Activation->TryGetArrayField(TEXT("Triggers"), Triggers))
		{
			Errors.Add(Field + TEXT(".Triggers: expected array"));
		}
		else
		{
			for (int32 Index = 0; Index < Triggers->Num(); ++Index)
			{
				const FString TriggerPath = FString::Printf(TEXT("%s.Triggers[%d]"), *Field, Index);
				const TSharedPtr<FJsonObject>* Trigger = nullptr;
				FAgAbilityTrigger Value;
				if (!(*Triggers)[Index]->TryGetObject(Trigger))
				{
					Errors.Add(TriggerPath + TEXT(": expected object"));
				}
				else if (Read(**Trigger, Value, TriggerPath, Errors))
				{
					Out.Activation.Triggers.Add(Value);
				}
			}
		}
		const TSharedPtr<FJsonValue> Retrigger = Activation->TryGetField(TEXT("bAllowRetrigger"));
		if (!Retrigger || Retrigger->Type != EJson::Boolean)
		{
			Errors.Add(Field + TEXT(".bAllowRetrigger: expected boolean"));
		}
		else
		{
			Out.Activation.bAllowRetrigger = Retrigger->AsBool();
		}
	}

	void ReadTargeting(const FJsonObject& Entry, FAgAbilityData& Out, const FString& Path, TArray<FString>& Errors)
	{
		Out.bHasTargeting = Entry.HasField(TEXT("Targeting"));
		if (!Out.bHasTargeting)
		{
			return;
		}
		const FString Field = Path + TEXT(".Targeting");
		const FObject Targeting = ObjectAt(Entry, TEXT("Targeting"), Field, Errors);
		if (!Targeting)
		{
			return;
		}
		FAgJsonFieldRules Rules;
		Rules.Optional = {
			GET_MEMBER_NAME_CHECKED(FAgAbilityTargeting, Range), GET_MEMBER_NAME_CHECKED(FAgAbilityTargeting, TargetRequiredTags),
			GET_MEMBER_NAME_CHECKED(FAgAbilityTargeting, TargetBlockedTags), GET_MEMBER_NAME_CHECKED(FAgAbilityTargeting, bRequired),
			GET_MEMBER_NAME_CHECKED(FAgAbilityTargeting, StopDistance)};
		if (!ReadStruct(*Targeting, *FAgAbilityTargeting::StaticStruct(), &Out.Targeting, Field, Errors, Rules))
		{
			return;
		}
		Out.Targeting.bHasRange = Targeting->HasField(TEXT("Range"));
		Out.Targeting.bHasStopDistance = Targeting->HasField(TEXT("StopDistance"));
		if (Out.Targeting.bHasStopDistance && Out.Targeting.StopDistance <= 0.f)
		{
			Errors.Add(Field + TEXT(".StopDistance: must be positive"));
		}

		// 적대하는 살아 있는 캐릭터만 남기고 Range와 대상 태그로 거르는 일은 프리셋 안의 프로젝트 필터가 합니다. 없으면 그 조건이
		// 모두 무시되므로 읽을 때 막습니다.
		const FTargetingTaskSet* TaskSet = Out.Targeting.Preset ? Out.Targeting.Preset->GetTargetingTaskSet() : nullptr;
		const bool bHasFilter = TaskSet && TaskSet->Tasks.ContainsByPredicate([](const TObjectPtr<UTargetingTask>& Task)
		{
			return Task && Task->IsA<UAgTargetingFilterTask_ActionTarget>();
		});
		if (!bHasFilter)
		{
			Errors.Add(Field + TEXT(".Preset: needs the filter task AgTargetingFilterTask_ActionTarget"));
		}
	}

	void ReadMontage(const FJsonObject& Entry, FAgAbilityData& Out, const FString& Path, TArray<FString>& Errors)
	{
		Out.bHasMontage = Entry.HasField(TEXT("Montage"));
		if (!Out.bHasMontage)
		{
			return;
		}
		const FString Field = Path + TEXT(".Montage");
		const FObject Montage = ObjectAt(Entry, TEXT("Montage"), Field, Errors);
		if (!Montage)
		{
			return;
		}
		Keys(*Montage, Field, {TEXT("MontageId")}, {TEXT("RootMotionDistance")}, Errors);
		FString Id;
		if (!Montage->TryGetStringField(TEXT("MontageId"), Id) || FName(*Id).IsNone())
		{
			Errors.Add(Field + TEXT(".MontageId: missing name"));
		}
		Out.Montage.MontageId = FName(*Id);
		Out.Montage.bHasRootMotionDistance = Montage->HasField(TEXT("RootMotionDistance"));
		if (Out.Montage.bHasRootMotionDistance &&
			Number(*Montage, TEXT("RootMotionDistance"), Field, Out.Montage.RootMotionDistance, Errors) &&
			Out.Montage.RootMotionDistance <= 0.f)
		{
			Errors.Add(Field + TEXT(".RootMotionDistance: must be positive"));
		}
	}

	void ReadEnd(const FJsonObject& Entry, FAgAbilityData& Out, const FString& Path, TArray<FString>& Errors)
	{
		FString Name;
		const int64 Value =
			Entry.TryGetStringField(TEXT("End"), Name) ? StaticEnum<EAgAbilityEnd>()->GetValueByNameString(Name) : INDEX_NONE;
		if (Value == INDEX_NONE)
		{
			Errors.Add(Path + TEXT(".End: expected Montage or TriggerTag"));
			return;
		}
		Out.End = static_cast<EAgAbilityEnd>(Value);
	}

	void ReadTasks(const FJsonObject& Entry, FAgAbilityData& Out, const FString& Path, TArray<FString>& Errors)
	{
		const FString Field = Path + TEXT(".Tasks");
		const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
		if (!Entry.TryGetArrayField(TEXT("Tasks"), Items))
		{
			Errors.Add(Field + TEXT(": expected array"));
			return;
		}
		for (int32 Index = 0; Index < Items->Num(); ++Index)
		{
			const FString ItemPath = FString::Printf(TEXT("%s[%d]"), *Field, Index);
			const TSharedPtr<FJsonObject>* Item = nullptr;
			if (!(*Items)[Index]->TryGetObject(Item))
			{
				Errors.Add(ItemPath + TEXT(": expected object"));
				continue;
			}
			FInstancedStruct Params;
			if (AgTaskParams::Read(**Item, EAgTaskPlacement::Activation, Params, ItemPath, Errors))
			{
				Out.Tasks.Add(MoveTemp(Params));
			}
		}
	}

	void ReadPatternSelection(const FJsonObject& Entry, FAgAbilityData& Out, const FString& Path, TArray<FString>& Errors)
	{
		Out.bHasPatternSelection = Entry.HasField(TEXT("PatternSelection"));
		if (!Out.bHasPatternSelection)
		{
			return;
		}
		const FString Field = Path + TEXT(".PatternSelection");
		const FObject Pattern = ObjectAt(Entry, TEXT("PatternSelection"), Field, Errors);
		if (!Pattern)
		{
			return;
		}
		Keys(*Pattern, Field,
			{TEXT("Phases"), TEXT("WeightNear"), TEXT("WeightMid"), TEXT("WeightFar"), TEXT("ResponseRange"), TEXT("ResponseChance")}, {},
			Errors);
		Number(*Pattern, TEXT("WeightNear"), Field, Out.PatternSelection.WeightNear, Errors);
		Number(*Pattern, TEXT("WeightMid"), Field, Out.PatternSelection.WeightMid, Errors);
		Number(*Pattern, TEXT("WeightFar"), Field, Out.PatternSelection.WeightFar, Errors);
		Number(*Pattern, TEXT("ResponseRange"), Field, Out.PatternSelection.ResponseRange, Errors);
		Number(*Pattern, TEXT("ResponseChance"), Field, Out.PatternSelection.ResponseChance, Errors);
		if (Out.PatternSelection.ResponseChance > 1.f)
		{
			Errors.Add(Field + TEXT(".ResponseChance: must be <= 1"));
		}
		const TArray<TSharedPtr<FJsonValue>>* Phases = nullptr;
		if (!Pattern->TryGetArrayField(TEXT("Phases"), Phases))
		{
			Errors.Add(Field + TEXT(".Phases: expected array"));
			return;
		}
		for (const TSharedPtr<FJsonValue>& Phase : *Phases)
		{
			if (Phase->Type != EJson::Number || (Phase->AsNumber() != 1 && Phase->AsNumber() != 2))
			{
				Errors.Add(Field + TEXT(".Phases: expected phase 1 or 2"));
			}
			else
			{
				Out.PatternSelection.Phases.Add(static_cast<int32>(Phase->AsNumber()));
			}
		}
	}

	/** 정의 안에서 정할 수 있는 규칙: 종료 방식에 필요한 것, 발동 Task의 행동 대상 */
	void ValidateDefinition(const FAgAbilityData& Data, const FString& Path, TArray<FString>& Errors)
	{
		int32 TagTriggers = 0;
		for (const FAgAbilityTrigger& Trigger : Data.Activation.Triggers)
		{
			TagTriggers += Trigger.Source == EAgTriggerSource::OwnedTagPresent ? 1 : 0;
		}
		if (Data.End == EAgAbilityEnd::Montage && !Data.bHasMontage)
		{
			Errors.Add(Path + TEXT(".End: Montage needs a Montage"));
		}
		if (Data.End == EAgAbilityEnd::TriggerTag && TagTriggers != 1)
		{
			Errors.Add(Path + TEXT(".End: TriggerTag needs exactly one OwnedTagPresent trigger"));
		}
		const bool bNeedsTarget = Data.Tasks.ContainsByPredicate([](const FInstancedStruct& Task)
		{
			return Task.Get<FAgTaskParams>().NeedsTarget();
		});
		if (bNeedsTarget && !Data.bHasTargeting && !Data.HasEventTrigger())
		{
			Errors.Add(Path + TEXT(".Tasks: a task uses the ability target, so the ability needs Targeting or a GameplayEvent trigger"));
		}
	}
} // namespace

void AgResource::SetAmounts(FGameplayEffectSpec& Spec, float SP, float MP, float UP)
{
	Spec.SetSetByCallerMagnitude(AgGameplayTags::SetByCaller_SP, SP);
	Spec.SetSetByCallerMagnitude(AgGameplayTags::SetByCaller_MP, MP);
	Spec.SetSetByCallerMagnitude(AgGameplayTags::SetByCaller_UP, UP);
}

void AgResource::SetAmount(FGameplayEffectSpec& Spec, EAgResource Resource, float Amount)
{
	SetAmounts(Spec, Resource == EAgResource::SP ? Amount : 0.f, Resource == EAgResource::MP ? Amount : 0.f,
		Resource == EAgResource::UP ? Amount : 0.f);
}

void FAgResourceCostParams::AddToSpec(FGameplayEffectSpec& Spec) const
{
	AgResource::SetAmount(Spec, Resource, -Amount);
}

bool FAgAbilityData::HasSameGrantSettings(const FAgAbilityData& Other) const
{
	return Id == Other.Id && Tags == Other.Tags &&
	       FAgAbilityActivation::StaticStruct()->CompareScriptStruct(&Activation, &Other.Activation, 0);
}

bool FAgAbilityData::HasEventTrigger() const
{
	return Activation.Triggers.ContainsByPredicate([](const FAgAbilityTrigger& Trigger)
	{
		return Trigger.Source == EAgTriggerSource::GameplayEvent;
	});
}

bool AgAbilityData::Parse(const FJsonObject& Json, TMap<FName, FAgAbilityData>& OutAbilities, TArray<FString>& Errors)
{
	const int32 Before = Errors.Num();
	TMap<FName, FAgAbilityData> Candidate;
	for (const TPair<FJsonObject::FStringType, TSharedPtr<FJsonValue>>& Pair : Json.Values)
	{
		const FString Path(Pair.Key);
		FAgAbilityData Data;
		Data.Id = FName(*Path);
		if (Data.Id.IsNone() || Candidate.Contains(Data.Id))
		{
			Errors.Add(Path + TEXT(": empty or duplicate ability ID"));
			continue;
		}
		const FObject Entry = ObjectAt(Json, Path, Path, Errors);
		if (!Entry)
		{
			continue;
		}
		const int32 EntryErrors = Errors.Num();
		Keys(*Entry, Path, {TEXT("Tags"), TEXT("Cooldown"), TEXT("Activation"), TEXT("Cost"), TEXT("End"), TEXT("Tasks")},
			{TEXT("Targeting"), TEXT("Montage"), TEXT("PatternSelection")}, Errors);
		Tags(*Entry, TEXT("Tags"), Path, Data.Tags, Errors);
		Number(*Entry, TEXT("Cooldown"), Path, Data.Cooldown, Errors);
		ReadActivation(*Entry, Data, Path, Errors);
		const TSharedPtr<FJsonValue> Cost = Entry->TryGetField(TEXT("Cost"));
		Data.bHasCost = Cost && Cost->Type != EJson::Null;
		if (Data.bHasCost)
		{
			if (const FObject Params = ObjectAt(*Entry, TEXT("Cost"), Path + TEXT(".Cost"), Errors))
			{
				Read(*Params, Data.Cost, Path + TEXT(".Cost"), Errors);
			}
		}
		ReadTargeting(*Entry, Data, Path, Errors);
		ReadMontage(*Entry, Data, Path, Errors);
		ReadEnd(*Entry, Data, Path, Errors);
		ReadTasks(*Entry, Data, Path, Errors);
		ReadPatternSelection(*Entry, Data, Path, Errors);
		if (Errors.Num() == EntryErrors)
		{
			ValidateDefinition(Data, Path, Errors);
		}
		Candidate.Add(Data.Id, MoveTemp(Data));
	}
	if (Before != Errors.Num())
	{
		return false;
	}
	OutAbilities = MoveTemp(Candidate);
	return true;
}

void AgAbilityData::ValidateCharacter(const UAgCharacterData& Data, const TMap<FName, FAgAbilityData>& Abilities, const TMap<FName, TObjectPtr<UGameplayEffect>>& Effects, const TMap<FName, FAgMontageTimeline>& Timelines, const FString& Path, TArray<FString>& Errors)
{
	TSet<FName> Seen;
	for (FName Id : Data.Abilities)
	{
		if (!Abilities.Contains(Id) || Seen.Contains(Id))
		{
			Errors.Add(Path + TEXT("Abilities: unknown or duplicate ability ") + Id.ToString());
		}
		Seen.Add(Id);
	}
	for (FName Id : Data.Effects)
	{
		if (!Effects.Contains(Id))
		{
			Errors.Add(Path + TEXT("Effects: unknown effect ") + Id.ToString());
		}
	}
	for (int32 Index = 0; Index < Data.DeathMotions.Num(); ++Index)
	{
		const FAgDeathMotion& Motion = Data.DeathMotions[Index];
		if (!Data.Montages.FindRef(Motion.Montage) || !Data.Montages.FindRef(Motion.Death))
		{
			Errors.Add(FString::Printf(TEXT("%sDeathMotions[%d]: unknown montage ID"), *Path, Index));
		}
	}
	if (Data.IsA<UAgPlayerData>())
	{
		// 플레이어는 점프할 때와 착지할 때 점프 몽타주의 두 섹션을 재생합니다.
		const UAnimMontage* Jump = Data.Montages.FindRef(AAgPlayerCharacter::JumpMontageId);
		if (!Jump || Jump->GetSectionIndex(AAgPlayerCharacter::JumpStartSection) == INDEX_NONE ||
			Jump->GetSectionIndex(AAgPlayerCharacter::JumpLandSection) == INDEX_NONE)
		{
			Errors.Add(Path + TEXT("Montages.Jump: needs sections Start and Land"));
		}

		// 입력 계층은 행동 태그로 스펙을 찾으므로 태그마다 행동이 하나 이하입니다.
		for (const FGameplayTag& Tag : AAgPlayerCharacter::GetInputAbilityTags())
		{
			int32 Count = 0;
			for (FName Id : Data.Abilities)
			{
				const FAgAbilityData* Ability = Abilities.Find(Id);
				Count += Ability && Ability->Tags.HasTagExact(Tag) ? 1 : 0;
			}
			if (Count > 1)
			{
				Errors.Add(Path + TEXT("Abilities: more than one ability has the input tag ") + Tag.ToString());
			}
		}
	}
	for (FName Id : Data.Abilities)
	{
		const FAgAbilityData* Ability = Abilities.Find(Id);
		if (!Ability)
		{
			continue;
		}
		const FString AbilityPath = Path + TEXT("Abilities.") + Id.ToString() + TEXT(": ");
		const UAnimMontage* Montage = Ability->bHasMontage ? Data.Montages.FindRef(Ability->Montage.MontageId) : nullptr;
		if (Ability->bHasMontage && !Montage)
		{
			Errors.Add(AbilityPath + TEXT("missing montage ") + Ability->Montage.MontageId.ToString());
		}
		if (Montage && Ability->Montage.bHasRootMotionDistance &&
			Montage->ExtractRootMotionFromTrackRange(0.f, Montage->GetPlayLength(), FAnimExtractContext()).GetTranslation().Size2D() <=
				UE_SMALL_NUMBER)
		{
			Errors.Add(AbilityPath + TEXT("Montage.RootMotionDistance: montage has no horizontal root motion"));
		}

		// 시간표 항목이 행동 대상을 쓰면 그 시간표를 재생하는 행동이 대상을 정해야 합니다. Launch와 Dash는 같은 우선순위로 속도를
		// 덮어써 함께 쓰면 나중에 건 쪽이 움직이지 못하므로, 한 행동에 함께 두지 않습니다.
		const FAgMontageTimeline* Timeline = Montage ? Timelines.Find(FName(*Montage->GetPathName())) : nullptr;
		bool bTimelineNeedsTarget = false;
		bool bLaunch = Ability->Tasks.ContainsByPredicate([](const FInstancedStruct& Task)
		{
			return Task.GetPtr<FAgLaunchParams>() != nullptr;
		});
		bool bDash = false;
		if (Timeline)
		{
			for (const FAgTimelineSection& Section : Timeline->Sections)
			{
				for (const FAgTimelineItem& Item : Section.Items)
				{
					bTimelineNeedsTarget |= Item.Params.Get<FAgTaskParams>().NeedsTarget();
					bLaunch |= Item.Params.GetPtr<FAgLaunchParams>() != nullptr;
					bDash |= Item.Params.GetPtr<FAgDashParams>() != nullptr;
				}
			}
		}
		if (bTimelineNeedsTarget && !Ability->bHasTargeting && !Ability->HasEventTrigger())
		{
			Errors.Add(AbilityPath +
					   TEXT("its montage timeline uses the ability target, so it needs Targeting or a GameplayEvent trigger"));
		}
		if (bLaunch && bDash)
		{
			Errors.Add(AbilityPath + TEXT("Launch and Dash both override the velocity, so one ability can't use both"));
		}
	}
}

void AgAbilityData::ValidateEffects(const TMap<FName, FAgAbilityData>& Abilities, const TMap<FName, TObjectPtr<UGameplayEffect>>& Effects, TArray<FString>& Errors)
{
	for (const TPair<FName, FAgAbilityData>& Pair : Abilities)
	{
		for (int32 Index = 0; Index < Pair.Value.Tasks.Num(); ++Index)
		{
			if (const FAgApplyEffectParams* Apply = Pair.Value.Tasks[Index].GetPtr<FAgApplyEffectParams>())
			{
				Apply->ValidateEffect(Effects, EAgTaskPlacement::Activation,
					FString::Printf(TEXT("%s.Tasks[%d]"), *Pair.Key.ToString(), Index), Errors);
			}
		}
	}
}
