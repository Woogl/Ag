// Copyright Woogle. All Rights Reserved.

#include "Core/AgAssetManager.h"

#include "Ag.h"
#include "Animation/AnimMontage.h"
#include "Data/AgAbilityData.h"
#include "Data/AgCameraData.h"
#include "Data/AgCharacterData.h"
#include "Data/AgCombatRules.h"
#include "Data/AgEffectData.h"
#include "Data/AgJsonRead.h"
#include "Data/AgMontageTimeline.h"
#include "Data/AgProjectileData.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "GameplayEffect.h"
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
#include "HAL/FileManager.h"
#endif

namespace
{
	/** JSON 파일은 Content 밖에 둡니다. Content에 두면 에디터가 에셋으로 가져오려 합니다. */
	const TCHAR* const JsonFolderName = TEXT("Data");

	/** 데이터 파일. 파일마다 그 파일을 읽는 클래스가 정해져 있어 이름을 바꿀 일이 없습니다. */
	const TCHAR* const CombatRulesFile = TEXT("CombatRules.json");
	const TCHAR* const CameraDataFile = TEXT("Camera.json");
	const TCHAR* const EffectsFile = TEXT("Effects.json");
	const TCHAR* const ProjectilesFile = TEXT("Projectiles.json");
	const TCHAR* const MontageTimelinesFile = TEXT("MontageTimelines.json");
	const TCHAR* const AbilitiesFile = TEXT("Abilities.json");
	const TCHAR* const CharactersFile = TEXT("Characters.json");

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

	/** 파일을 읽어 JSON 객체로 바꿉니다. 오류가 있으면 null입니다. */
	TSharedPtr<FJsonObject> ReadJsonFile(const TCHAR* FileName, TArray<FString>& Errors)
	{
		FString Text;
		const FString Path = FPaths::Combine(UAgAssetManager::GetJsonFolder(), FileName);
		if (!FFileHelper::LoadFileToString(Text, *Path))
		{
			Errors.Add(FString::Printf(TEXT("can't read %s"), *Path));
			return nullptr;
		}
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
		TSharedPtr<FJsonObject> Json;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
		if (!FJsonSerializer::Deserialize(Reader, Json) || !Json.IsValid())
		{
			Errors.Add(FString::Printf(TEXT("syntax error: %s"), *Reader->GetErrorMessage()));
			return nullptr;
		}
		return Errors.IsEmpty() ? Json : nullptr;
	}

	/** Json으로 Target의 Transient 속성을 모두 채웁니다. 에디터에서는 키와 값을 속성에 맞춰 먼저 검사합니다. 오류 경로 앞에 Path를 붙입니다. */
	void ReadProperties(const FJsonObject& Json, UObject& Target, const FString& Path, TArray<FString>& Errors)
	{
		TArray<const FProperty*> Properties;
		for (TFieldIterator<FProperty> It(Target.GetClass()); It; ++It)
		{
			if (It->HasAnyPropertyFlags(CPF_Transient))
			{
				Properties.Add(*It);
			}
		}
#if WITH_EDITOR
		const int32 Before = Errors.Num();
		CheckObject(Json, Properties, Path, Errors);
		if (Errors.Num() != Before)
		{
			return;
		}
#endif
		for (const FProperty* Property : Properties)
		{
			FText FailReason;
			const TSharedPtr<FJsonValue> Value = Json.TryGetField(Property->GetName());
			if (!Value.IsValid() || !FJsonObjectConverter::JsonValueToUProperty(Value, const_cast<FProperty*>(Property),
			                            Property->ContainerPtrToValuePtr<void>(&Target), 0, 0, false, &FailReason))
			{
				Errors.Add(Path + Property->GetName() + TEXT(": ") + FailReason.ToString());
			}
		}
	}

	/** 검사를 마친 새 객체의 값(Transient 속성)을 지금 객체에 복사합니다. 지금 객체를 가리키는 쪽은 그대로 새 값을 봅니다. */
	void CopyJsonProperties(const UObject& From, UObject& To)
	{
		for (TFieldIterator<FProperty> It(To.GetClass()); It; ++It)
		{
			if (It->HasAnyPropertyFlags(CPF_Transient))
			{
				It->CopyCompleteValue(It->ContainerPtrToValuePtr<void>(&To), It->ContainerPtrToValuePtr<void>(&From));
			}
		}
	}

	void Report(const TCHAR* FileName, const TArray<FString>& Errors)
	{
		const FString File = FString::Printf(TEXT("Data/%s"), FileName);
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
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 15.f, FColor::Red,
		    FString::Printf(TEXT("%s has %d error(s), so no data file was applied (all in the log):"), *File, Errors.Num()));
	}

}

UAgAssetManager::UAgAssetManager()
{
	CombatRules = CreateDefaultSubobject<UAgCombatRules>(TEXT("CombatRules"));
	CameraData = CreateDefaultSubobject<UAgCameraData>(TEXT("CameraData"));
	Effects = CreateDefaultSubobject<UAgEffectSet>(TEXT("Effects"));
	Projectiles = CreateDefaultSubobject<UAgProjectileSet>(TEXT("Projectiles"));
	MontageTimelines = CreateDefaultSubobject<UAgMontageTimelineData>(TEXT("MontageTimelines"));
	Abilities = CreateDefaultSubobject<UAgAbilitySet>(TEXT("Abilities"));
	Characters.Add(TEXT("Player"), CreateDefaultSubobject<UAgPlayerData>(TEXT("Player")));
	Characters.Add(TEXT("Boss"), CreateDefaultSubobject<UAgBossData>(TEXT("Boss")));
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

	LoadJsonData();
}

const UAgCombatRules& UAgAssetManager::GetCombatRules() const
{
	return *CombatRules;
}

const UAgCameraData& UAgAssetManager::GetCameraData() const
{
	return *CameraData;
}

const UAgMontageTimelineData& UAgAssetManager::GetMontageTimelines() const
{
	return *MontageTimelines;
}

const FAgAbilityData* UAgAssetManager::FindAbility(FName Id) const
{
	return Abilities->Abilities.Find(Id);
}

const TMap<FName, FAgProjectileDefinition>& UAgAssetManager::GetProjectiles() const
{
	return Projectiles->Projectiles;
}

const UGameplayEffect* UAgAssetManager::FindEffect(FName Id) const
{
	const TObjectPtr<UGameplayEffect>* Found = Effects->Effects.Find(Id);
	return Found ? Found->Get() : nullptr;
}

const UGameplayEffect* UAgAssetManager::GetEffect(EAgEffect Effect) const
{
	return FindEffect(AgEffectData::Id(Effect));
}

UAgCharacterData* UAgAssetManager::FindCharacterData(FName Id) const
{
	const TObjectPtr<UAgCharacterData>* Found = Characters.Find(Id);
	return Found ? Found->Get() : nullptr;
}

TArray<FName> UAgAssetManager::GetCharacterDataIds(const UClass* DataClass) const
{
	TArray<FName> Ids;
	for (const TPair<FName, TObjectPtr<UAgCharacterData>>& Pair : Characters)
	{
		if (!DataClass || Pair.Value->IsA(DataClass))
		{
			Ids.Add(Pair.Key);
		}
	}
	return Ids;
}

bool UAgAssetManager::LoadJsonData()
{
	++DataVersion;
	// 새 객체에 모두 읽고, 모두 통과해야 지금 객체에 복사합니다. 실패하면 기존 값이 그대로 남습니다.
	UAgCombatRules* NewRules = NewObject<UAgCombatRules>(this);
	UAgCameraData* NewCamera = NewObject<UAgCameraData>(this);
	UAgEffectSet* NewEffects = NewObject<UAgEffectSet>(this);
	UAgProjectileSet* NewProjectiles = NewObject<UAgProjectileSet>(this);
	UAgMontageTimelineData* NewTimelines = NewObject<UAgMontageTimelineData>(this);
	UAgAbilitySet* NewAbilities = NewObject<UAgAbilitySet>(this);
	JsonLoadTargets.Reset();
	JsonLoadTargets.Add(NewRules);
	JsonLoadTargets.Add(NewCamera);
	JsonLoadTargets.Add(NewEffects);
	JsonLoadTargets.Add(NewProjectiles);
	JsonLoadTargets.Add(NewTimelines);
	JsonLoadTargets.Add(NewAbilities);
	TMap<FName, UAgCharacterData*> NewCharacters;
	for (const TPair<FName, TObjectPtr<UAgCharacterData>>& Pair : Characters)
	{
		UAgCharacterData* Candidate = NewObject<UAgCharacterData>(this, Pair.Value->GetClass());
		NewCharacters.Add(Pair.Key, Candidate);
		JsonLoadTargets.Add(Candidate);
	}
	ON_SCOPE_EXIT
	{
		JsonLoadTargets.Reset();
	};

	TArray<FString> RulesErrors;
	if (const TSharedPtr<FJsonObject> Json = ReadJsonFile(CombatRulesFile, RulesErrors))
	{
		ReadProperties(*Json, *NewRules, FString(), RulesErrors);
	}
	TArray<FString> CameraErrors;
	if (const TSharedPtr<FJsonObject> Json = ReadJsonFile(CameraDataFile, CameraErrors))
	{
		ReadProperties(*Json, *NewCamera, FString(), CameraErrors);
	}
	TArray<FString> EffectErrors;
	if (const TSharedPtr<FJsonObject> Json = ReadJsonFile(EffectsFile, EffectErrors))
	{
		AgEffectData::Parse(*Json, *this, DataVersion, NewEffects->Effects, EffectErrors);
	}
	TArray<FString> ProjectileErrors;
	if (const TSharedPtr<FJsonObject> Json = ReadJsonFile(ProjectilesFile, ProjectileErrors))
	{
		// 파일 전체가 투사체 ID의 맵이라, 그 맵 속성 하나를 가진 객체로 읽습니다.
		FJsonObject File;
		File.SetObjectField(TEXT("Projectiles"), Json);
		ReadProperties(File, *NewProjectiles, FString(), ProjectileErrors);
		for (const TPair<FName, FAgProjectileDefinition>& Pair : NewProjectiles->Projectiles)
		{
			if (Pair.Value.Params.Speed <= 0.f || Pair.Value.Params.Range <= 0.f)
			{
				ProjectileErrors.Add(Pair.Key.ToString() + TEXT(".Params: Speed and Range must be positive"));
			}
		}
	}
	TArray<FString> TimelineErrors;
	if (const TSharedPtr<FJsonObject> Json = ReadJsonFile(MontageTimelinesFile, TimelineErrors))
	{
		AgMontageTimeline::Parse(*Json, NewTimelines->Timelines, TimelineErrors);
#if WITH_EDITOR
		if (TimelineErrors.IsEmpty())
		{
			AgMontageTimeline::ValidateAssets(NewTimelines->Timelines, TimelineErrors);
		}
#endif
	}
	TArray<FString> AbilityErrors;
	if (const TSharedPtr<FJsonObject> Json = ReadJsonFile(AbilitiesFile, AbilityErrors))
	{
		AgAbilityData::Parse(*Json, NewAbilities->Abilities, AbilityErrors);
	}
	TArray<FString> CharacterErrors;
	if (const TSharedPtr<FJsonObject> Json = ReadJsonFile(CharactersFile, CharacterErrors))
	{
		for (const TPair<FJsonObject::FStringType, TSharedPtr<FJsonValue>>& Pair : Json->Values)
		{
			if (!NewCharacters.Contains(FName(*FString(Pair.Key))))
			{
				CharacterErrors.Add(FString(Pair.Key) + TEXT(": unknown character ID"));
			}
		}
		for (const TPair<FName, UAgCharacterData*>& Pair : NewCharacters)
		{
			const FString Path = Pair.Key.ToString();
			if (const AgJsonRead::FObject Entry = AgJsonRead::ObjectAt(*Json, Path, Path, CharacterErrors))
			{
				ReadProperties(*Entry, *Pair.Value, Path + TEXT("."), CharacterErrors);
			}
		}
	}

	// 파일 사이의 참조는 양쪽 파일을 읽었을 때만 검사합니다.
	if (TimelineErrors.IsEmpty() && ProjectileErrors.IsEmpty())
	{
		for (const TPair<FName, FAgMontageTimeline>& Pair : NewTimelines->Timelines)
		{
			for (const FAgTimelineSpawnProjectile& Shot : Pair.Value.SpawnProjectile)
			{
				if (!NewProjectiles->Projectiles.Contains(Shot.ProjectileId))
				{
					TimelineErrors.Add(Pair.Key.ToString() + TEXT(".SpawnProjectile: unknown projectile ") + Shot.ProjectileId.ToString());
				}
			}
		}
	}
	if (CharacterErrors.IsEmpty() && AbilityErrors.IsEmpty() && EffectErrors.IsEmpty())
	{
		for (const TPair<FName, UAgCharacterData*>& Pair : NewCharacters)
		{
			AgAbilityData::ValidateCharacter(
			    *Pair.Value, NewAbilities->Abilities, NewEffects->Effects, Pair.Key.ToString() + TEXT("."), CharacterErrors);
		}
	}

	const TPair<const TCHAR*, const TArray<FString>*> Results[] = {{CombatRulesFile, &RulesErrors}, {CameraDataFile, &CameraErrors},
	    {EffectsFile, &EffectErrors}, {ProjectilesFile, &ProjectileErrors}, {MontageTimelinesFile, &TimelineErrors},
	    {AbilitiesFile, &AbilityErrors}, {CharactersFile, &CharacterErrors}};
	bool bFailed = false;
	for (const TPair<const TCHAR*, const TArray<FString>*>& Result : Results)
	{
		if (!Result.Value->IsEmpty())
		{
			Report(Result.Key, *Result.Value);
			bFailed = true;
		}
	}
	if (bFailed)
	{
		return false;
	}

	CopyJsonProperties(*NewRules, *CombatRules);
	CopyJsonProperties(*NewCamera, *CameraData);
	CopyJsonProperties(*NewEffects, *Effects);
	CopyJsonProperties(*NewProjectiles, *Projectiles);
	CopyJsonProperties(*NewTimelines, *MontageTimelines);
	CopyJsonProperties(*NewAbilities, *Abilities);
	for (const TPair<FName, UAgCharacterData*>& Pair : NewCharacters)
	{
		UAgCharacterData& Target = *Characters.FindChecked(Pair.Key);
		CopyJsonProperties(*Pair.Value, Target);
		Target.MarkJsonDataLoaded();
	}
	return true;
}

#if WITH_EDITOR
void UAgAssetManager::ReloadJsonData()
{
	if (LoadJsonData() && GEngine)
	{
		// 스탯처럼 스폰할 때 한 번 넣는 값은 다음 PIE부터 바뀝니다(구현 계획서 'JSON 데이터' 다시 읽기).
		GEngine->AddOnScreenDebugMessage(
		    INDEX_NONE, 5.f, FColor::Green, TEXT("Data reloaded. Spawn settings apply on the next play start."));
	}
}

void UAgAssetManager::ModifyCook(
    TConstArrayView<const ITargetPlatform*> TargetPlatforms, TArray<FName>& PackagesToCook, TArray<FName>& PackagesToNeverCook)
{
	Super::ModifyCook(TargetPlatforms, PackagesToCook, PackagesToNeverCook);
	// 초기 로딩 이후 파일이 바뀌었더라도 쿡은 최신 JSON 전체를 검사합니다.
	if (!LoadJsonData())
	{
		UE_LOG(LogAg, Error, TEXT("Cannot cook invalid JSON data"));
		return;
	}

	// JSON에서 읽은 Transient 참조는 저장된 에셋 의존성이 아니므로 쿡에 직접 전달합니다. 효과의 GE는 실행 중에 만들고 클래스만 가리킵니다.
	TArray<UObject*> DataObjects = {CombatRules, CameraData, Projectiles, MontageTimelines, Abilities};
	for (const TPair<FName, TObjectPtr<UAgCharacterData>>& Pair : Characters)
	{
		DataObjects.Add(Pair.Value);
	}
	for (UObject* Data : DataObjects)
	{
		TArray<UObject*> References;
		FReferenceFinder Finder(References, nullptr, false, true, false, false);
		Finder.AddPropertyReferencesWithStructARO(Data->GetClass(), Data);
		for (UObject* Reference : References)
		{
			const FString PackageName = Reference->GetOutermost()->GetName();
			if (!PackageName.StartsWith(TEXT("/Script/")) && PackageName != TEXT("/Engine/Transient"))
			{
				PackagesToCook.AddUnique(FName(*PackageName));
			}
		}
	}

	// 쓰는 곳이 없는 정의는 배포하지 않습니다.
	TSet<FName> UsedAbilities;
	TSet<FName> UsedEffects;
	TSet<FName> UsedMontages;
	TSet<FName> UsedProjectiles;
	for (const TPair<FName, TObjectPtr<UAgCharacterData>>& Pair : Characters)
	{
		UsedAbilities.Append(Pair.Value->Abilities);
		UsedEffects.Append(Pair.Value->Effects);
		for (const TPair<FName, TObjectPtr<UAnimMontage>>& Montage : Pair.Value->Montages)
		{
			if (Montage.Value)
			{
				UsedMontages.Add(FName(*Montage.Value->GetPathName()));
			}
		}
	}
	const UEnum* CodeEffects = StaticEnum<EAgEffect>();
	for (int32 Index = 0; Index < CodeEffects->NumEnums() - 1; ++Index)
	{
		UsedEffects.Add(AgEffectData::Id(static_cast<EAgEffect>(CodeEffects->GetValueByIndex(Index))));
	}
	for (const TPair<FName, FAgMontageTimeline>& Pair : MontageTimelines->Timelines)
	{
		if (!UsedMontages.Contains(Pair.Key))
		{
			UE_LOG(LogAg, Error, TEXT("%s: no character data plays this montage (%s)"), MontageTimelinesFile, *Pair.Key.ToString());
		}
		for (const FAgTimelineSpawnProjectile& Shot : Pair.Value.SpawnProjectile)
		{
			UsedProjectiles.Add(Shot.ProjectileId);
		}
	}
	for (const TPair<FName, FAgAbilityData>& Pair : Abilities->Abilities)
	{
		if (!UsedAbilities.Contains(Pair.Key))
		{
			UE_LOG(LogAg, Error, TEXT("%s: no character uses this ability (%s)"), AbilitiesFile, *Pair.Key.ToString());
		}
	}
	for (const TPair<FName, TObjectPtr<UGameplayEffect>>& Pair : Effects->Effects)
	{
		if (!UsedEffects.Contains(Pair.Key))
		{
			UE_LOG(LogAg, Error, TEXT("%s: neither C++ nor a character uses this effect (%s)"), EffectsFile, *Pair.Key.ToString());
		}
	}
	for (const TPair<FName, FAgProjectileDefinition>& Pair : Projectiles->Projectiles)
	{
		if (!UsedProjectiles.Contains(Pair.Key))
		{
			UE_LOG(LogAg, Error, TEXT("%s: no montage timeline launches this projectile (%s)"), ProjectilesFile, *Pair.Key.ToString());
		}
	}

	// UFS에 포함되지만 로더가 읽지 않는 파일도 배포하지 않습니다.
	const TSet<FString> DataFiles = {FString(CombatRulesFile).ToLower(), FString(CameraDataFile).ToLower(), FString(EffectsFile).ToLower(),
	    FString(ProjectilesFile).ToLower(), FString(MontageTimelinesFile).ToLower(), FString(AbilitiesFile).ToLower(),
	    FString(CharactersFile).ToLower()};
	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *FPaths::Combine(GetJsonFolder(), TEXT("*.json")), true, false);
	for (const FString& File : Files)
	{
		if (!DataFiles.Contains(File.ToLower()))
		{
			UE_LOG(LogAg, Error, TEXT("Cannot stage unvalidated JSON: %s"), *File);
		}
	}
}
#endif
