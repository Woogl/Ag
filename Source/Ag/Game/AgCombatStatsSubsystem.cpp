// Copyright Woogle. All Rights Reserved.

#include "Game/AgCombatStatsSubsystem.h"

#include "Ag.h"
#include "Dom/JsonValue.h"
#include "JsonObjectConverter.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Policies/PrettyJsonPrintPolicy.h"

void UAgCombatStatsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	FilePath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("CombatStats"), FDateTime::Now().ToString(TEXT("%Y-%m-%d_%H-%M-%S")) + TEXT(".json"));
}

void UAgCombatStatsSubsystem::RequestRestart()
{
	bRestartRequested = true;
}

void UAgCombatStatsSubsystem::BeginAttempt()
{
	Attempt = bRestartRequested ? Attempt + 1 : 1;
	bRestartRequested = false;
}

void UAgCombatStatsSubsystem::Save(const FAgCombatStats& Stats)
{
	File.Records.Add(Stats);

	// 엔진 JSON 출력은 실수를 유효 숫자 17자리로 써서(48.3 → 48.299999999999997), 전투 시간은 밀리초 세 자리로 씁니다.
	const FJsonObjectConverter::CustomExportCallback ExportDuration = FJsonObjectConverter::CustomExportCallback::CreateLambda([](FProperty* Property, const void* Value) -> TSharedPtr<FJsonValue>
	{
		if (Property->GetFName() != GET_MEMBER_NAME_CHECKED(FAgCombatStats, Duration) || Property->GetOwnerStruct() != FAgCombatStats::StaticStruct())
		{
			return nullptr;
		}
		return MakeShared<FJsonValueNumberString>(FString::Printf(TEXT("%.3f"), *static_cast<const float*>(Value)));
	});
	FString Json;
	if (!FJsonObjectConverter::UStructToFormattedJsonObjectString<TCHAR, TPrettyJsonPrintPolicy>(FAgCombatStatsFile::StaticStruct(), &File, Json, 0, 0, 0, &ExportDuration, EJsonObjectConversionFlags::SkipStandardizeCase) ||
		!FFileHelper::SaveStringToFile(Json, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogAg, Error, TEXT("Combat stats: could not write %s."), *FilePath);
	}
}
