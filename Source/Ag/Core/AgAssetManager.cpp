// Copyright Woogle. All Rights Reserved.

#include "Core/AgAssetManager.h"

#include "Ag.h"
#include "Data/AgCameraData.h"
#include "Data/AgCharacterData.h"
#include "Data/AgCombatRules.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "GameplayTagContainer.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "StructUtils/InstancedStruct.h"
#include "UObject/UnrealType.h"

namespace
{
	/** JSON 파일은 Content 밖에 둡니다. Content에 두면 에디터가 에셋으로 가져오려 합니다. */
	const TCHAR* const JsonFolderName = TEXT("Data");

	/** 엔진 JSON 변환기가 인스턴스 구조체를 무슨 구조체로 읽을지 보는 키. 파일에는 적지 않고 SetJsonStructType이 채웁니다. */
	const TCHAR* const StructTypeKey = TEXT("_structType");

	/** 공통 JSON 파일. 캐릭터 JSON은 캐릭터 DataAsset의 ValuesFile이 가리킵니다. */
	const TCHAR* const CombatRulesFile = TEXT("CombatRules.json");
	const TCHAR* const CameraDataFile = TEXT("Camera.json");

	/** 파일마다 화면에 띄우는 오류 수. 로그에는 모두 남깁니다. */
	constexpr int32 MaxErrorsOnScreen = 5;

	bool IsTagProperty(const FProperty* Property)
	{
		const FStructProperty* StructProperty = CastField<FStructProperty>(Property);
		return StructProperty && StructProperty->Struct == FGameplayTag::StaticStruct();
	}

	const UEnum* GetEnum(const FProperty* Property)
	{
		if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
		{
			return EnumProperty->GetEnum();
		}
		const FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property);
		return NumericProperty ? NumericProperty->GetIntPropertyEnum() : nullptr;
	}

	/** 속성 값을 JSON에 적는 형식 */
	EJson GetJsonType(const FProperty* Property)
	{
		if (Property->IsA<FArrayProperty>())
		{
			return EJson::Array;
		}
		if (Property->IsA<FBoolProperty>())
		{
			return EJson::Boolean;
		}
		if (Property->IsA<FNumericProperty>() && !GetEnum(Property))
		{
			return EJson::Number;
		}
		if (Property->IsA<FStructProperty>() && !IsTagProperty(Property))
		{
			return EJson::Object;
		}
		// 열거형, 텍스트, 태그
		return EJson::String;
	}

	const TCHAR* GetJsonTypeName(EJson Type)
	{
		switch (Type)
		{
		case EJson::Array:
			return TEXT("an array");
		case EJson::Boolean:
			return TEXT("true or false");
		case EJson::Number:
			return TEXT("a number");
		case EJson::Object:
			return TEXT("an object");
		default:
			return TEXT("a string");
		}
	}

	void CheckObject(const FJsonObject& Json, const TArray<const FProperty*>& Properties, const FString& Path, TArray<FString>& Errors);

	/** 값을 속성에 맞춰 검사합니다: JSON 형식, 그다음 그 형식에 필요한 것(필드, 태그, 열거형 값, 정수, 범위). */
	void CheckValue(const TSharedPtr<FJsonValue>& Value, const FProperty* Property, const FString& Path, TArray<FString>& Errors)
	{
		const EJson Type = GetJsonType(Property);
		if (!Value.IsValid() || Value->Type != Type)
		{
			Errors.Add(FString::Printf(TEXT("%s: must be %s"), *Path, GetJsonTypeName(Type)));
			return;
		}

		if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
		{
			const TArray<TSharedPtr<FJsonValue>>& Items = Value->AsArray();
			for (int32 Index = 0; Index < Items.Num(); ++Index)
			{
				CheckValue(Items[Index], ArrayProperty->Inner, FString::Printf(TEXT("%s[%d]"), *Path, Index), Errors);
			}
		}
		else if (Type == EJson::Object)
		{
			const UScriptStruct* Struct = CastFieldChecked<FStructProperty>(Property)->Struct;
			FJsonObject Object = *Value->AsObject();
			FString FieldPath = Path + TEXT(".");
			if (Struct == FInstancedStruct::StaticStruct())
			{
				// 인스턴스 구조체: 데이터 객체가 정해 둔 구조체(SetJsonStructType)의 필드로 검사합니다. 정해 둔 구조체가 없으면
				// 값을 읽을 곳이 없으므로 비어 있어야 합니다.
				FString StructPath;
				if (!Object.TryGetStringField(StructTypeKey, StructPath))
				{
					if (!Object.Values.IsEmpty())
					{
						Errors.Add(FString::Printf(TEXT("%s: must be {}, nothing reads these values"), *Path));
					}
					return;
				}
				Struct = FindObject<UScriptStruct>(nullptr, *StructPath);
				if (!Struct)
				{
					Errors.Add(FString::Printf(TEXT("%s: %s is not a struct"), *Path, *StructPath));
					return;
				}
				Object.RemoveField(StructTypeKey);
				// 오류에 구조체 이름을 붙여, 키를 정하는 C++ 구조체를 바로 찾게 합니다.
				FieldPath = FString::Printf(TEXT("%s (%s)."), *Path, *Struct->GetName());
			}

			TArray<const FProperty*> Fields;
			for (TFieldIterator<FProperty> It(Struct); It; ++It)
			{
				Fields.Add(*It);
			}
			CheckObject(Object, Fields, FieldPath, Errors);
		}
		else if (IsTagProperty(Property))
		{
			const FString TagName = Value->AsString();
			if (!TagName.IsEmpty() && !FGameplayTag::RequestGameplayTag(FName(*TagName), /*ErrorIfNotFound*/ false).IsValid())
			{
				Errors.Add(FString::Printf(TEXT("%s: %s is not a gameplay tag"), *Path, *TagName));
			}
		}
		else if (const UEnum* Enum = GetEnum(Property))
		{
			// FJsonObjectConverter와 같은 방법으로 찾습니다.
			if (Enum->GetValueByName(FName(*Value->AsString()), EGetByNameFlags::CheckAuthoredName) == INDEX_NONE)
			{
				Errors.Add(FString::Printf(TEXT("%s: %s is not a value of %s"), *Path, *Value->AsString(), *Enum->GetName()));
			}
		}
		else if (Type == EJson::Number)
		{
			// 정수 속성에 소수를 쓰면 변환이 조용히 잘라 버리므로 오류로 알립니다.
			const double Number = Value->AsNumber();
			const FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property);
			if (NumericProperty && NumericProperty->IsInteger() && Number != FMath::TruncToDouble(Number))
			{
				Errors.Add(FString::Printf(TEXT("%s: %g is not a whole number"), *Path, Number));
			}
#if WITH_METADATA
			// 배열 원소의 허용 범위는 배열 속성에 적혀 있습니다. UHT가 원소 속성에는 메타데이터를 두지 않기 때문입니다.
			const FProperty* RangeProperty = Property->GetOwnerProperty();
			if (RangeProperty->HasMetaData(TEXT("ClampMin")) && Number < FCString::Atod(*RangeProperty->GetMetaData(TEXT("ClampMin"))))
			{
				Errors.Add(FString::Printf(TEXT("%s: %g is below the minimum %s"), *Path, Number, *RangeProperty->GetMetaData(TEXT("ClampMin"))));
			}
			if (RangeProperty->HasMetaData(TEXT("ClampMax")) && Number > FCString::Atod(*RangeProperty->GetMetaData(TEXT("ClampMax"))))
			{
				Errors.Add(FString::Printf(TEXT("%s: %g is above the maximum %s"), *Path, Number, *RangeProperty->GetMetaData(TEXT("ClampMax"))));
			}
#endif
		}
	}

	/**
	 * Json에 이 속성들이 빠짐없이, 다른 키 없이 있는지 확인하고 값마다 검사합니다. 키는 엔진의 이름(FName)과 JSON 객체처럼
	 * 대소문자를 구분하지 않습니다. 쿡한 빌드의 속성 이름은 대소문자를 보존하지 않습니다(예: UP → Up).
	 */
	void CheckObject(const FJsonObject& Json, const TArray<const FProperty*>& Properties, const FString& Path, TArray<FString>& Errors)
	{
		for (const auto& Field : Json.Values)
		{
			const FString Key(*Field.Key);
			const FProperty* const* Property = Properties.FindByPredicate([&Key](const FProperty* Candidate)
			{
				return Candidate->GetName().Equals(Key, ESearchCase::IgnoreCase);
			});
			if (Property)
			{
				CheckValue(Field.Value, *Property, Path + Key, Errors);
			}
			else
			{
				Errors.Add(FString::Printf(TEXT("%s%s: unknown key"), *Path, *Key));
			}
		}
		for (const FProperty* Property : Properties)
		{
			if (!Json.HasField(Property->GetName()))
			{
				Errors.Add(FString::Printf(TEXT("%s%s: missing"), *Path, *Property->GetName()));
			}
		}
	}

	void Report(const UObject& Target, const FString& FileName, const TArray<FString>& Errors)
	{
		const FString File = FileName.IsEmpty() ? Target.GetName() : FString::Printf(TEXT("Data/%s"), *FileName);
		for (const FString& Error : Errors)
		{
			UE_LOG(LogAg, Error, TEXT("%s: %s"), *File, *Error);
		}
		if (!GEngine)
		{
			return;
		}
		// 새 메시지가 위에 쌓이므로, 오류를 뒤에서부터 넣고 요약을 마지막에 넣습니다.
		for (int32 Index = FMath::Min(Errors.Num(), MaxErrorsOnScreen) - 1; Index >= 0; --Index)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 15.f, FColor::Red, FString::Printf(TEXT("    %s"), *Errors[Index]));
		}
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 15.f, FColor::Red, FString::Printf(TEXT("%s was not applied, %d error(s) (all in the log):"), *File, Errors.Num()));
	}
}

UAgAssetManager::UAgAssetManager()
{
	CombatRules = CreateDefaultSubobject<UAgCombatRules>(TEXT("CombatRules"));
	CameraData = CreateDefaultSubobject<UAgCameraData>(TEXT("CameraData"));
}

UAgAssetManager& UAgAssetManager::Get()
{
	// DefaultEngine.ini의 AssetManagerClassName이 이 클래스가 아니면 여기서 멈춥니다.
	check(GEngine);
	return *CastChecked<UAgAssetManager>(GEngine->AssetManager);
}

FString UAgAssetManager::GetJsonFolder()
{
	return FPaths::Combine(FPaths::ProjectDir(), JsonFolderName);
}

void UAgAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();

	LoadJsonValues(*CombatRules, CombatRulesFile);
	LoadJsonValues(*CameraData, CameraDataFile);
}

const UAgCombatRules& UAgAssetManager::GetCombatRules() const
{
	return *CombatRules;
}

const UAgCameraData& UAgAssetManager::GetCameraData() const
{
	return *CameraData;
}

bool UAgAssetManager::LoadJsonValues(UObject& Target, const FString& FileName)
{
#if WITH_EDITOR
	for (auto It = LoadedFiles.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}
	LoadedFiles.Add(&Target, FileName);
#endif

	TArray<FString> Errors;
	TSharedPtr<FJsonObject> Json;
	FString Text;
	const FString Path = FPaths::Combine(GetJsonFolder(), FileName);
	if (FileName.IsEmpty())
	{
		Errors.Add(TEXT("no data file is set"));
	}
	else if (!FFileHelper::LoadFileToString(Text, *Path))
	{
		Errors.Add(FString::Printf(TEXT("can't read %s"), *Path));
	}
	else
	{
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
		if (!FJsonSerializer::Deserialize(Reader, Json) || !Json.IsValid())
		{
			Errors.Add(FString::Printf(TEXT("syntax error: %s"), *Reader->GetErrorMessage()));
		}
	}

	TArray<const FProperty*> Properties;
	for (TFieldIterator<FProperty> It(Target.GetClass()); It; ++It)
	{
		if (It->HasAnyPropertyFlags(CPF_Transient))
		{
			Properties.Add(*It);
		}
	}
	if (Json.IsValid())
	{
		// 인스턴스 구조체 값의 구조체는 데이터 객체가 정합니다(어빌리티의 전용 값).
		if (const UAgCharacterData* CharacterData = Cast<UAgCharacterData>(&Target))
		{
			CharacterData->ResolveJsonStructTypes(*Json, Errors);
		}
		CheckObject(*Json, Properties, FString(), Errors);
	}

	// 값마다 먼저 따로 변환해 두므로, 하나라도 실패하면 대상은 그대로 남습니다.
	TArray<FDefaultConstructedPropertyElement> Values;
	if (Errors.IsEmpty())
	{
		Values.Reserve(Properties.Num());
		for (const FProperty* Property : Properties)
		{
			const FDefaultConstructedPropertyElement& Value = Values.Emplace_GetRef(Property);
			FText FailReason;
			if (!FJsonObjectConverter::JsonValueToUProperty(Json->TryGetField(Property->GetName()), const_cast<FProperty*>(Property), Value.GetObjAddress(), 0, 0, false, &FailReason))
			{
				Errors.Add(FString::Printf(TEXT("%s: %s"), *Property->GetName(), *FailReason.ToString()));
			}
		}
	}

	if (!Errors.IsEmpty())
	{
		Report(Target, FileName, Errors);
		return false;
	}
	for (int32 Index = 0; Index < Properties.Num(); ++Index)
	{
		Properties[Index]->CopyCompleteValue(Properties[Index]->ContainerPtrToValuePtr<void>(&Target), Values[Index].GetObjAddress());
	}
	return true;
}

void UAgAssetManager::SetJsonStructType(FJsonObject& Object, const UScriptStruct& Struct)
{
	Object.SetStringField(StructTypeKey, Struct.GetPathName());
}

#if WITH_EDITOR
void UAgAssetManager::ReloadJsonFiles(const TArray<FString>& FileNames)
{
	for (const FString& FileName : FileNames)
	{
		// 다시 읽으면 기록이 바뀌므로 대상을 먼저 모읍니다.
		TArray<UObject*> Objects;
		for (const TPair<TWeakObjectPtr<UObject>, FString>& Entry : LoadedFiles)
		{
			UObject* Object = Entry.Key.Get();
			if (Object && Entry.Value.Equals(FileName, ESearchCase::IgnoreCase))
			{
				Objects.Add(Object);
			}
		}
		if (Objects.IsEmpty())
		{
			continue;
		}
		bool bSucceeded = true;
		for (UObject* Object : Objects)
		{
			bSucceeded &= LoadJsonValues(*Object, FileName);
		}
		if (bSucceeded && GEngine)
		{
			// 스탯은 스폰할 때 한 번 넣고 다시 넣지 않으므로 다음 PIE부터 바뀝니다(구현 계획서 'JSON과 DataAsset' 다시 읽기).
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Green,
				FString::Printf(TEXT("Data/%s reloaded. Attributes change on the next PIE start."), *FileName));
		}
	}
}
#endif
