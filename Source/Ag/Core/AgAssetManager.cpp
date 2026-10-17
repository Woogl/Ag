// Copyright Woogle. All Rights Reserved.

#include "Core/AgAssetManager.h"

#include "Ag.h"
#include "Data/AgCameraData.h"
#include "Data/AgAbilityData.h"
#include "Data/AgCharacterData.h"
#include "Data/AgCombatRules.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "GameplayTagContainer.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "Misc/ScopeExit.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/UnrealType.h"
#include "UObject/Package.h"
#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#endif

namespace
{
	/** JSON 파일은 Content 밖에 둡니다. Content에 두면 에디터가 에셋으로 가져오려 합니다. */
	const TCHAR* const JsonFolderName = TEXT("Data");

	/** 공통 JSON 파일. 캐릭터 JSON은 캐릭터 DataAsset의 JsonFileName이 가리킵니다. */
	const TCHAR* const CombatRulesFile = TEXT("CombatRules.json");
	const TCHAR* const CameraDataFile = TEXT("Camera.json");

	/** 파일마다 화면에 띄우는 오류 수. 로그에는 모두 남깁니다. */
	constexpr int32 MaxErrorsOnScreen = 5;

#if WITH_EDITOR
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
		if (Property->IsA<FMapProperty>() || (Property->IsA<FStructProperty>() && !IsTagProperty(Property)))
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
		else if (const FMapProperty* MapProperty = CastField<FMapProperty>(Property))
		{
			TSet<FName> Keys;
			for (const TPair<FJsonObject::FStringType, TSharedPtr<FJsonValue>>& Entry : Value->AsObject()->Values)
			{
				const FString Key(Entry.Key);
				const FName Name(*Key);
				if (Name.IsNone() || Keys.Contains(Name))
				{
					Errors.Add(FString::Printf(TEXT("%s.%s: empty or duplicate name"), *Path, *Key));
				}
				Keys.Add(Name);
				if (const FNumericProperty* KeyNumber = CastField<FNumericProperty>(MapProperty->KeyProp);
				    KeyNumber && KeyNumber->IsInteger())
				{
					int32 Index = 0;
					if (!LexTryParseString(Index, *Key) || FString::FromInt(Index) != Key)
					{
						Errors.Add(Path + TEXT(".") + Key + TEXT(": must be a canonical integer key"));
					}
				}
				CheckValue(Entry.Value, MapProperty->ValueProp, Path + TEXT(".") + Key, Errors);
			}
		}
		else if (const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
		{
			const FString ObjectPath = Value->AsString();
			// JSON은 전체 객체 경로만 받습니다. null이나 리다이렉터에 기대는 오래된 경로는 허용하지 않습니다.
			if (!FPackageName::IsValidObjectPath(ObjectPath))
			{
				Errors.Add(FString::Printf(TEXT("%s: invalid object path %s"), *Path, *ObjectPath));
				return;
			}
			UObject* Object = StaticLoadObject(UObject::StaticClass(), nullptr, *ObjectPath, nullptr, LOAD_NoWarn);
			const FClassProperty* ClassProperty = CastField<FClassProperty>(Property);
			const UClass* Class = Cast<UClass>(Object);
			if (!Object || !Object->IsA(ObjectProperty->PropertyClass) ||
			    (ClassProperty && (!Class || !Class->IsChildOf(ClassProperty->MetaClass) || Class->HasAnyClassFlags(CLASS_Abstract))))
			{
				Errors.Add(FString::Printf(TEXT("%s: %s is missing or has an invalid type"), *Path, *ObjectPath));
			}
			else if (!Object->GetPathName().Equals(ObjectPath, ESearchCase::IgnoreCase))
			{
				Errors.Add(FString::Printf(TEXT("%s: use the current path %s"), *Path, *Object->GetPathName()));
			}
		}
		else if (Type == EJson::Object)
		{
			const UScriptStruct* Struct = CastFieldChecked<FStructProperty>(Property)->Struct;
			TArray<const FProperty*> Fields;
			for (TFieldIterator<FProperty> It(Struct); It; ++It)
			{
				if (!It->HasAnyPropertyFlags(CPF_Deprecated))
				{
					Fields.Add(*It);
				}
			}
			CheckObject(*Value->AsObject(), Fields, Path + TEXT("."), Errors);
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
			if (!FMath::IsFinite(Number))
			{
				Errors.Add(Path + TEXT(": must be a finite number"));
				return;
			}
			if (NumericProperty->IsInteger())
			{
				const bool bUnsigned = Property->IsA<FByteProperty>() || Property->IsA<FUInt16Property>() ||
				                       Property->IsA<FUInt32Property>() || Property->IsA<FUInt64Property>();
				// 상한을 배타적인 2의 거듭제곱으로 비교해 int64 최댓값의 double 반올림도 피합니다.
				const double UpperBound = FMath::Pow(2.0, NumericProperty->GetSize() * 8 - (bUnsigned ? 0 : 1));
				if (Number < (bUnsigned ? 0.0 : -UpperBound) || Number >= UpperBound)
				{
					Errors.Add(Path + TEXT(": outside numeric storage range"));
					return;
				}
			}
			else if (Property->IsA<FFloatProperty>() && FMath::Abs(Number) > static_cast<double>(MAX_flt))
			{
				Errors.Add(Path + TEXT(": outside numeric storage range"));
				return;
			}
			if (NumericProperty && NumericProperty->IsInteger() && Number != FMath::TruncToDouble(Number))
			{
				Errors.Add(FString::Printf(TEXT("%s: %g is not a whole number"), *Path, Number));
			}
#if WITH_METADATA
			// 배열 원소의 허용 범위는 배열 속성에 적혀 있습니다. UHT가 원소 속성에는 메타데이터를 두지 않기 때문입니다.
			const FProperty* RangeProperty = Property->GetOwnerProperty();
			if (RangeProperty->HasMetaData(TEXT("ClampMin")) && Number < FCString::Atod(*RangeProperty->GetMetaData(TEXT("ClampMin"))))
			{
				Errors.Add(
				    FString::Printf(TEXT("%s: %g is below the minimum %s"), *Path, Number, *RangeProperty->GetMetaData(TEXT("ClampMin"))));
			}
			if (RangeProperty->HasMetaData(TEXT("ClampMax")) && Number > FCString::Atod(*RangeProperty->GetMetaData(TEXT("ClampMax"))))
			{
				Errors.Add(
				    FString::Printf(TEXT("%s: %g is above the maximum %s"), *Path, Number, *RangeProperty->GetMetaData(TEXT("ClampMax"))));
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
		for (const TPair<FJsonObject::FStringType, TSharedPtr<FJsonValue>>& Field : Json.Values)
		{
			const FString Key(*Field.Key);
			const FProperty* const* Property = Properties.FindByPredicate(
			    [&Key](const FProperty* Candidate)
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

#endif // WITH_EDITOR

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
		GEngine->AddOnScreenDebugMessage(
		    INDEX_NONE, 15.f, FColor::Red, FString::Printf(TEXT("%s was not applied, %d error(s) (all in the log):"), *File, Errors.Num()));
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
	for (TMap<TWeakObjectPtr<UObject>, FString>::TIterator It = LoadedFiles.CreateIterator(); It; ++It)
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
#if WITH_EDITOR
		// JSON DOM은 중복 키를 덮어쓸 수 있으므로 원문에서 FName 기준으로 먼저 검사합니다.
		const TSharedRef<TJsonReader<>> KeyReader = TJsonReaderFactory<>::Create(Text);
		TArray<TSet<FName>> Containers;
		EJsonNotation Notation;
		while (KeyReader->ReadNext(Notation))
		{
			if (Notation == EJsonNotation::ObjectEnd || Notation == EJsonNotation::ArrayEnd)
			{
				if (!Containers.IsEmpty())
				{
					Containers.Pop();
				}
				continue;
			}
			const FString Key = KeyReader->GetIdentifier();
			if (!Key.IsEmpty() && !Containers.IsEmpty())
			{
				const FName Name(*Key);
				if (Containers.Last().Contains(Name))
				{
					Errors.Add(FString::Printf(TEXT("duplicate JSON name: %s (line %d)"), *Key, KeyReader->GetLineNumber()));
				}
				Containers.Last().Add(Name);
			}
			if (Notation == EJsonNotation::ObjectStart || Notation == EJsonNotation::ArrayStart)
			{
				Containers.AddDefaulted();
			}
		}
#endif
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
	// 대상과 같은 타입의 임시 객체에만 변환합니다. 실패하면 원래 객체는 바뀌지 않습니다.
	UObject* Candidate = NewObject<UObject>(this, Target.GetClass());
	JsonLoadTargets.Add(Candidate);
	ON_SCOPE_EXIT
	{
		JsonLoadTargets.RemoveSingle(Candidate);
	};

	TFunction<void(const FProperty*)> ConvertProperty = [&](const FProperty* Property)
	{
		FText FailReason;
		const TSharedPtr<FJsonValue> Value = Json->TryGetField(Property->GetName());
		if (!Value.IsValid() || !FJsonObjectConverter::JsonValueToUProperty(Value, const_cast<FProperty*>(Property),
		                            Property->ContainerPtrToValuePtr<void>(Candidate), 0, 0, false, &FailReason))
		{
			Errors.Add(FString::Printf(TEXT("%s: %s"), *Property->GetName(), *FailReason.ToString()));
		}
	};

	const FProperty* AbilitiesProperty =
	    Cast<UAgCharacterData>(Candidate) ? FindFProperty<FProperty>(Target.GetClass(), TEXT("Abilities")) : nullptr;
	if (Json.IsValid())
	{
		if (UAgCharacterData* Character = Cast<UAgCharacterData>(Candidate))
		{
			AgAbilityData::Parse(*Json, Character->Abilities, Errors);
		}
#if WITH_EDITOR
		FJsonObject OtherFields = *Json;
		TArray<const FProperty*> OtherProperties = Properties;
		if (AbilitiesProperty)
		{
			OtherFields.RemoveField(TEXT("Abilities"));
			OtherProperties.Remove(AbilitiesProperty);
		}
		if (Errors.IsEmpty())
		{
			CheckObject(OtherFields, OtherProperties, FString(), Errors);
		}
#endif
	}
	if (Errors.IsEmpty())
	{
		for (const FProperty* Property : Properties)
		{
			if (Property != AbilitiesProperty)
			{
				ConvertProperty(Property);
			}
		}
	}
	if (Errors.IsEmpty())
	{
		if (const UAgCharacterData* Character = Cast<UAgCharacterData>(Candidate))
		{
			Character->ValidateProjectiles(Errors);
			AgAbilityData::ValidateReferences(*Character, Errors);
		}
	}
	if (!Errors.IsEmpty())
	{
		Report(Target, FileName, Errors);
		return false;
	}
	for (const FProperty* Property : Properties)
	{
		Property->CopyCompleteValue(Property->ContainerPtrToValuePtr<void>(&Target), Property->ContainerPtrToValuePtr<void>(Candidate));
	}
	if (UAgCharacterData* CharacterData = Cast<UAgCharacterData>(&Target))
	{
		CharacterData->MarkJsonDataLoaded();
	}
	return true;
}

#if WITH_EDITOR
void UAgAssetManager::ModifyCook(
    TConstArrayView<const ITargetPlatform*> TargetPlatforms, TArray<FName>& PackagesToCook, TArray<FName>& PackagesToNeverCook)
{
	Super::ModifyCook(TargetPlatforms, PackagesToCook, PackagesToNeverCook);
	// 초기 로딩 이후 파일이 바뀌었더라도 쿡은 최신 공통 JSON 전체를 검사합니다.
	TSet<FString> ValidatedFiles;
	TFunction<void(UObject&, const FString&)> LoadForCook = [&](UObject& Data, const FString& FileName)
	{
		if (!LoadJsonValues(Data, FileName))
		{
			UE_LOG(LogAg, Error, TEXT("Cannot cook invalid JSON: %s"), *FileName);
			return;
		}
		ValidatedFiles.Add(FileName.ToLower());
		TArray<UObject*> References;
		FReferenceFinder Finder(References, nullptr, false, true, false, false);
		Finder.AddPropertyReferencesWithStructARO(Data.GetClass(), &Data);
		for (UObject* Reference : References)
		{
			const FString PackageName = Reference->GetOutermost()->GetName();
			if (!PackageName.StartsWith(TEXT("/Script/")) && PackageName != TEXT("/Engine/Transient"))
			{
				PackagesToCook.AddUnique(FName(*PackageName));
			}
		}
	};
	LoadForCook(*CombatRules, CombatRulesFile);
	LoadForCook(*CameraData, CameraDataFile);

	// JSON에서 읽은 Transient 참조는 저장된 에셋 의존성이 아니므로 쿡에 직접 전달합니다.
	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	Registry.SearchAllAssets(true);
	TArray<FAssetData> Assets;
	Registry.GetAssetsByClass(UAgCharacterData::StaticClass()->GetClassPathName(), Assets, true);
	for (const FAssetData& Asset : Assets)
	{
		UAgCharacterData* Data = Cast<UAgCharacterData>(Asset.GetAsset());
		if (!Data)
		{
			UE_LOG(LogAg, Error, TEXT("Cannot load character data for cook: %s"), *Asset.GetObjectPathString());
			continue;
		}
		LoadForCook(*Data, Data->JsonFileName);
	}
	// UFS에 포함되지만 로더에 연결되지 않은 파일도 배포하지 않습니다.
	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *FPaths::Combine(GetJsonFolder(), TEXT("*.json")), true, false);
	for (const FString& File : Files)
	{
		if (!ValidatedFiles.Contains(File.ToLower()))
		{
			UE_LOG(LogAg, Error, TEXT("Cannot stage unvalidated JSON: %s"), *File);
		}
	}
}

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
			    FString::Printf(TEXT("Data/%s reloaded. Spawn settings apply on the next play start."), *FileName));
		}
	}
}

#endif
