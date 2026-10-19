// Copyright Woogle. All Rights Reserved.

#include "Data/AgTaskParams.h"

namespace
{
	const TCHAR* const StructPrefix = TEXT("Ag");
	const TCHAR* const StructSuffix = TEXT("Params");
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
