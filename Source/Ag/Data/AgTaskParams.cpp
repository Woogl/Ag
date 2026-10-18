// Copyright Woogle. All Rights Reserved.

#include "Data/AgTaskParams.h"
#include "Data/AgJsonRead.h"
#include "Data/AgProjectileData.h"
#include "Dom/JsonObject.h"
#include "GameplayEffect.h"

namespace
{
	/** Task 이름 X의 Params 구조체 이름은 AgXParams입니다(C++ 이름 FAgXParams). */
	const TCHAR* const StructPrefix = TEXT("Ag");
	const TCHAR* const StructSuffix = TEXT("Params");

	const TCHAR* DescribePlacement(EAgTaskPlacement Placement)
	{
		switch (Placement)
		{
			case EAgTaskPlacement::Activation:
				return TEXT("in the ability Tasks");
			case EAgTaskPlacement::Window:
				return TEXT("as a timeline window");
			case EAgTaskPlacement::Point:
				return TEXT("as a timeline point");
			default:
				return TEXT("here");
		}
	}
}

const UScriptStruct* AgTaskParams::FindStruct(const FString& TaskName)
{
	// Task 이름은 C++ 식별자입니다. 경로 문자로 다른 구조체를 찾지 않게 합니다.
	if (TaskName.IsEmpty())
	{
		return nullptr;
	}
	for (const TCHAR Character : TaskName)
	{
		if (!FChar::IsAlnum(Character))
		{
			return nullptr;
		}
	}
	const UScriptStruct* Struct =
		FindObject<UScriptStruct>(nullptr, *FString::Printf(TEXT("/Script/Ag.%s%s%s"), StructPrefix, *TaskName, StructSuffix));
	const UScriptStruct* Base = FAgTaskParams::StaticStruct();
	return Struct && Struct != Base && Struct->IsChildOf(Base) ? Struct : nullptr;
}

FName AgTaskParams::GetTaskName(const UScriptStruct& Struct)
{
	FString Name = Struct.GetName();
	Name.RemoveFromStart(StructPrefix, ESearchCase::CaseSensitive);
	Name.RemoveFromEnd(StructSuffix, ESearchCase::CaseSensitive);
	return FName(*Name);
}

bool AgTaskParams::Read(const FJsonObject& Item, EAgTaskPlacement Placement, FInstancedStruct& Out, const FString& Path, TArray<FString>& Errors, const TArray<FString>& Ignored)
{
	const int32 Before = Errors.Num();
	FString TaskName;
	const UScriptStruct* Struct = Item.TryGetStringField(TEXT("Task"), TaskName) ? FindStruct(TaskName) : nullptr;
	if (!Struct)
	{
		Errors.Add(Path + TEXT(".Task: expected a registered task name"));
		return false;
	}
	FInstancedStruct Params;
	Params.InitializeAs(Struct);
	FAgTaskParams& Task = Params.GetMutable<FAgTaskParams>();
	if (!EnumHasAnyFlags(Task.GetPlacements(), Placement))
	{
		Errors.Add(FString::Printf(TEXT("%s.Task: %s can't be used %s"), *Path, *TaskName, DescribePlacement(Placement)));
		return false;
	}
	FAgJsonFieldRules Rules;
	Task.GetFieldRules(Rules);
	TArray<FString> Skipped = Ignored;
	Skipped.Add(TEXT("Task"));
	if (!AgJsonRead::ReadStruct(Item, *Struct, Params.GetMutableMemory(), Path, Errors, Rules, Skipped))
	{
		return false;
	}
	Task.Validate(Item, Placement, Path, Errors);
	if (Errors.Num() != Before)
	{
		return false;
	}
	Out = MoveTemp(Params);
	return true;
}

void AgTaskParams::ValidateReferences(const FAgTaskParams& Params, const TMap<FName, TObjectPtr<UGameplayEffect>>* Effects, const TMap<FName, FAgProjectileDefinition>* Projectiles, const FString& Path, TArray<FString>& Errors)
{
	FAgTaskReferences References;
	Params.GetReferences(References);
	for (FName Id : References.Effects)
	{
		if (Effects && !Effects->Contains(Id))
		{
			Errors.Add(Path + TEXT(": uses unknown effect ") + Id.ToString());
		}
	}
	for (FName Id : References.Projectiles)
	{
		if (Projectiles && !Projectiles->Contains(Id))
		{
			Errors.Add(Path + TEXT(": uses unknown projectile ") + Id.ToString());
		}
	}
}
