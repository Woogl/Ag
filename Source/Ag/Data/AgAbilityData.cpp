// Copyright Woogle. All Rights Reserved.

#include "Data/AgAbilityData.h"
#include "Data/AgCharacterData.h"
#include "Data/AgJsonRead.h"
#include "Animation/AnimMontage.h"
#include "Dom/JsonObject.h"
#include "JsonObjectConverter.h"
#include "UObject/UnrealType.h"

namespace
{
	using namespace AgJsonRead;

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

	bool HasTask(const FAgAbilityData& Ability, FName Task)
	{
		return Ability.Tasks.ContainsByPredicate(
		    [Task](const FInstancedStruct& Params)
		    {
			    return Params.GetScriptStruct() && AgTaskParams::GetTaskName(*Params.GetScriptStruct()) == Task;
		    });
	}
}

bool FAgAbilityData::HasSameGrantSettings(const FAgAbilityData& Other) const
{
	return Id == Other.Id && Tags == Other.Tags &&
	       FAgAbilityActivation::StaticStruct()->CompareScriptStruct(&Activation, &Other.Activation, 0);
}

namespace
{
	/** C++ 사건이 쓰는 설정이 정의에 있는지 확인합니다. */
	void ValidateRequiredTasks(const FAgAbilityData& Ability, TArray<FString>& Errors)
	{
		const FString Path = Ability.Id.ToString() + TEXT(".Tasks: ");
		TFunction<void(const TCHAR*)> Require = [&](const TCHAR* Task)
		{
			if (!HasTask(Ability, Task))
			{
				Errors.Add(Path + Task + TEXT(" required by C++ event"));
			}
		};
		const FName Id = Ability.Id;
		if (Id == TEXT("Dodge"))
		{
			Require(TEXT("GrantResources"));
			Require(TEXT("TimedTag"));
			Require(TEXT("SlowMotion"));
		}
		if (Id == TEXT("Guard"))
		{
			Require(TEXT("HoldTag"));
			Require(TEXT("TimedTag"));
			Require(TEXT("GrantResources"));
		}
		if (Id == TEXT("KnockBack") || Id == TEXT("KnockUp"))
		{
			Require(TEXT("HoldTag"));
		}
		if (Id == TEXT("Groggy"))
		{
			Require(TEXT("HoldTag"));
			Require(TEXT("SetCollisionResponseToChannel"));
		}
		if (Id == TEXT("Ultimate"))
		{
			Require(TEXT("TimeStop"));
			Require(TEXT("PlayCutscene"));
			Require(TEXT("TargetQuery"));
			Require(TEXT("Attack"));
		}
		if (Id == TEXT("Execution"))
		{
			Require(TEXT("TargetQuery"));
			Require(TEXT("Attack"));
		}
		if (Id == TEXT("LeapSlam"))
		{
			Require(TEXT("LaunchToTarget"));
			Require(TEXT("TargetQuery"));
			Require(TEXT("Attack"));
		}
		if (Id == TEXT("JumpAttack"))
		{
			Require(TEXT("LaunchVertical"));
		}
		if (Id == TEXT("LockOn"))
		{
			Require(TEXT("LockOn"));
		}
	}
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
		Keys(*Entry, Path, {TEXT("Tags"), TEXT("Cooldown"), TEXT("Activation"), TEXT("Cost"), TEXT("Tasks")},
		    {TEXT("Montage"), TEXT("PatternSelection")}, Errors);
		Tags(*Entry, TEXT("Tags"), Path, Data.Tags, Errors);
		Number(*Entry, TEXT("Cooldown"), Path, Data.Cooldown, Errors);
		if (FObject Activation = ObjectAt(*Entry, TEXT("Activation"), Path + TEXT(".Activation"), Errors))
		{
			Keys(*Activation, Path + TEXT(".Activation"),
			    {TEXT("RequiredTags"), TEXT("BlockedTags"), TEXT("OwnedTags"), TEXT("BlockTags"), TEXT("CancelTags"), TEXT("TriggerEvents"),
			        TEXT("bAllowRetrigger")},
			    {}, Errors);
			Tags(*Activation, TEXT("RequiredTags"), Path + TEXT(".Activation"), Data.Activation.RequiredTags, Errors);
			Tags(*Activation, TEXT("BlockedTags"), Path + TEXT(".Activation"), Data.Activation.BlockedTags, Errors);
			Tags(*Activation, TEXT("OwnedTags"), Path + TEXT(".Activation"), Data.Activation.OwnedTags, Errors);
			Tags(*Activation, TEXT("BlockTags"), Path + TEXT(".Activation"), Data.Activation.BlockTags, Errors);
			Tags(*Activation, TEXT("CancelTags"), Path + TEXT(".Activation"), Data.Activation.CancelTags, Errors);
			Tags(*Activation, TEXT("TriggerEvents"), Path + TEXT(".Activation"), Data.Activation.TriggerEvents, Errors);
			const TSharedPtr<FJsonValue> Retrigger = Activation->TryGetField(TEXT("bAllowRetrigger"));
			if (!Retrigger || Retrigger->Type != EJson::Boolean)
			{
				Errors.Add(Path + TEXT(".Activation.bAllowRetrigger: expected boolean"));
			}
			else
			{
				Data.Activation.bAllowRetrigger = Retrigger->AsBool();
			}
		}
		const TSharedPtr<FJsonValue> Cost = Entry->TryGetField(TEXT("Cost"));
		Data.bHasCost = Cost && Cost->Type != EJson::Null;
		if (Data.bHasCost)
		{
			if (const FObject Params = ObjectAt(*Entry, TEXT("Cost"), Path + TEXT(".Cost"), Errors))
			{
				Read(*Params, Data.Cost, Path + TEXT(".Cost"), Errors);
			}
		}
		ReadMontage(*Entry, Data, Path, Errors);
		ReadTasks(*Entry, Data, Path, Errors);
		Data.bHasPatternSelection = Entry->HasField(TEXT("PatternSelection"));
		if (Data.bHasPatternSelection)
		{
			const FString Field = Path + TEXT(".PatternSelection");
			if (const FObject Pattern = ObjectAt(*Entry, TEXT("PatternSelection"), Field, Errors))
			{
				Keys(*Pattern, Field,
				    {TEXT("Phases"), TEXT("WeightNear"), TEXT("WeightMid"), TEXT("WeightFar"), TEXT("ResponseRange"),
				        TEXT("ResponseChance")},
				    {}, Errors);
				Number(*Pattern, TEXT("WeightNear"), Field, Data.PatternSelection.WeightNear, Errors);
				Number(*Pattern, TEXT("WeightMid"), Field, Data.PatternSelection.WeightMid, Errors);
				Number(*Pattern, TEXT("WeightFar"), Field, Data.PatternSelection.WeightFar, Errors);
				Number(*Pattern, TEXT("ResponseRange"), Field, Data.PatternSelection.ResponseRange, Errors);
				Number(*Pattern, TEXT("ResponseChance"), Field, Data.PatternSelection.ResponseChance, Errors);
				if (Data.PatternSelection.ResponseChance > 1.f)
				{
					Errors.Add(Field + TEXT(".ResponseChance: must be <= 1"));
				}
				const TArray<TSharedPtr<FJsonValue>>* Phases = nullptr;
				if (!Pattern->TryGetArrayField(TEXT("Phases"), Phases))
				{
					Errors.Add(Field + TEXT(".Phases: expected array"));
				}
				else
				{
					for (const TSharedPtr<FJsonValue>& Phase : *Phases)
					{
						if (Phase->Type != EJson::Number || (Phase->AsNumber() != 1 && Phase->AsNumber() != 2))
						{
							Errors.Add(Field + TEXT(".Phases: expected phase 1 or 2"));
						}
						else
						{
							Data.PatternSelection.Phases.Add(static_cast<int32>(Phase->AsNumber()));
						}
					}
				}
			}
		}
		Candidate.Add(Data.Id, MoveTemp(Data));
	}
	if (Before == Errors.Num())
	{
		for (const TPair<FName, FAgAbilityData>& Pair : Candidate)
		{
			ValidateRequiredTasks(Pair.Value, Errors);
		}
	}
	if (Before != Errors.Num())
	{
		return false;
	}
	OutAbilities = MoveTemp(Candidate);
	return true;
}

void AgAbilityData::ValidateCharacter(const UAgCharacterData& Data, const TMap<FName, FAgAbilityData>& Abilities,
    const TMap<FName, TObjectPtr<UGameplayEffect>>& Effects, const FString& Path, TArray<FString>& Errors)
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
	if (const UAgPlayerData* Player = Cast<UAgPlayerData>(&Data);
	    Player && Data.Abilities.Contains(TEXT("BasicAttack")) && Player->BasicAttackMontageIds.IsEmpty())
	{
		Errors.Add(Path + TEXT("BasicAttackMontageIds: requires at least one montage"));
	}
	for (FName Id : Data.Abilities)
	{
		const FAgAbilityData* Ability = Abilities.Find(Id);
		if (!Ability)
		{
			continue;
		}
		const FString AbilityPath = Path + TEXT("Abilities.") + Id.ToString() + TEXT(": ");
		for (FName MontageId : MontageIds(Data, *Ability))
		{
			if (!Data.Montages.FindRef(MontageId))
			{
				Errors.Add(AbilityPath + TEXT("missing montage ") + MontageId.ToString());
			}
		}
		const FAgAbilityMontage& Params = Ability->Montage;
		const UAnimMontage* Montage = Ability->bHasMontage ? Data.Montages.FindRef(Params.MontageId) : nullptr;
		if (Montage && Params.bHasRootMotionDistance &&
		    Montage->ExtractRootMotionFromTrackRange(0.f, Montage->GetPlayLength(), FAnimExtractContext()).GetTranslation().Size2D() <=
		        UE_SMALL_NUMBER)
		{
			Errors.Add(AbilityPath + TEXT("Montage.RootMotionDistance: montage has no horizontal root motion"));
		}
	}
}

TArray<FName> AgAbilityData::MontageIds(const UAgCharacterData& Data, const FAgAbilityData& Ability)
{
	TArray<FName> Result;
	if (Ability.bHasMontage)
	{
		Result.AddUnique(Ability.Montage.MontageId);
	}
	const FName Id = Ability.Id;
	if (Id == TEXT("BasicAttack"))
	{
		if (const UAgPlayerData* Player = Cast<UAgPlayerData>(&Data))
		{
			Result.Append(Player->BasicAttackMontageIds);
		}
	}
	if (Id == TEXT("Dodge"))
	{
		Result.Append({TEXT("Roll"), TEXT("Backstep")});
	}
	if (Id == TEXT("Jump"))
	{
		Result.Append({TEXT("Jump"), TEXT("Land")});
	}
	if (Id == TEXT("Guard"))
	{
		Result.Add(TEXT("Parry"));
	}
	if (Id == TEXT("KnockUp"))
	{
		Result.Append({TEXT("KnockUp"), TEXT("KnockUpFall"), TEXT("GetUp")});
	}
	if (Id == TEXT("Groggy"))
	{
		Result.Append({TEXT("Groggy"), TEXT("Executed")});
	}
	if (Id == TEXT("Death") && Data.IsA<UAgBossData>())
	{
		Result.Append({TEXT("Executed"), TEXT("ExecutedDeath")});
	}
	return Result;
}
