// Copyright Woogle. All Rights Reserved.

#include "Data/AgTaskParams.h"

const UScriptStruct* AgTaskParams::FindStruct(const FString& TaskName)
{
	const UScriptStruct* Struct = FindObject<UScriptStruct>(nullptr, *FString::Printf(TEXT("/Script/Ag.Ag%sParams"), *TaskName));
	const UScriptStruct* Base = FAgTaskParams::StaticStruct();
	return Struct && Struct != Base && Struct->IsChildOf(Base) ? Struct : nullptr;
}
