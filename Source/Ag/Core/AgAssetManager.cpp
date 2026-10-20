// Copyright Woogle. All Rights Reserved.

#include "Core/AgAssetManager.h"

#include "Ag.h"
#include "AttributeSet.h"
#include "Data/AgAbilityData.h"
#include "Data/AgCameraData.h"
#include "Data/AgCharacterData.h"
#include "Data/AgCombatRules.h"
#include "Data/AgEffectData.h"
#include "Data/AgMontageTimeline.h"
#include "Data/AgProjectileData.h"
#include "Data/AgTargetingPresetData.h"
#include "Data/AgTaskParams.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "StructUtils/InstancedStruct.h"
#include "TargetingSystem/TargetingPreset.h"
#include "UObject/Package.h"
#include "UObject/PropertyOptional.h"
#include "UObject/UnrealType.h"

namespace
{
	/** JSON 파일은 Content 밖에 둡니다. Content에 두면 에디터가 에셋으로 가져오려 합니다. */
	const TCHAR* const JsonFolderName = TEXT("Data");

	const TCHAR* const CombatRulesFile = TEXT("CombatRules.json");
	const TCHAR* const CameraDataFile = TEXT("Camera.json");
	const TCHAR* const EffectsFile = TEXT("Effects.json");
	const TCHAR* const ProjectilesFile = TEXT("Projectiles.json");
	const TCHAR* const MontageTimelinesFile = TEXT("MontageTimelines.json");
	const TCHAR* const TargetingPresetsFile = TEXT("TargetingPresets.json");
	const TCHAR* const AbilitiesFile = TEXT("Abilities.json");
	const TCHAR* const CharactersFile = TEXT("Characters.json");

	/** 지금 객체를 가리키는 쪽이 그대로 새 값을 보도록 객체를 바꾸지 않고 값만 복사합니다. */
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
} // namespace

UAgAssetManager::UAgAssetManager()
{
	CombatRules = CreateDefaultSubobject<UAgCombatRules>(TEXT("CombatRules"));
	CameraData = CreateDefaultSubobject<UAgCameraData>(TEXT("CameraData"));
	Effects = CreateDefaultSubobject<UAgEffectSet>(TEXT("Effects"));
	Projectiles = CreateDefaultSubobject<UAgProjectileSet>(TEXT("Projectiles"));
	MontageTimelines = CreateDefaultSubobject<UAgMontageTimelineData>(TEXT("MontageTimelines"));
	TargetingPresets = CreateDefaultSubobject<UAgTargetingPresetSet>(TEXT("TargetingPresets"));
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

const UTargetingPreset* UAgAssetManager::FindTargetingPreset(FName Id) const
{
	const TObjectPtr<UTargetingPreset>* Found = TargetingPresets->Presets.Find(Id);
	return Found ? Found->Get() : nullptr;
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
	return NativeEffects.FindChecked(Effect).Get();
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

#if WITH_EDITOR
void UAgAssetManager::ReloadJsonData()
{
	if (LoadJsonData() && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Green,
			TEXT("Data reloaded. Spawn settings apply on the next play start."));
	}
}

void UAgAssetManager::ModifyCook(TConstArrayView<const ITargetPlatform*> TargetPlatforms, TArray<FName>& PackagesToCook, TArray<FName>& PackagesToNeverCook)
{
	Super::ModifyCook(TargetPlatforms, PackagesToCook, PackagesToNeverCook);
	// 초기 로딩 이후 파일이 바뀌었더라도 쿡은 최신 JSON 전체를 검사합니다. 읽기 오류의 Error 로그가 쿡을 실패시킵니다.
	if (!LoadJsonData())
	{
		return;
	}

	// JSON에서 읽은 Transient 참조는 저장된 에셋 의존성이 아니므로 쿡에 직접 전달합니다. 효과의 GE와 대상 프리셋은
	// 실행 중에 만들고 클래스만 가리킵니다.
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
}
#endif

void UAgAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();

	const UEnum* NativeEffectEnum = StaticEnum<EAgEffect>();
	// 마지막 값은 UHT가 붙이는 _MAX입니다.
	for (int32 Index = 0; Index < NativeEffectEnum->NumEnums() - 1; ++Index)
	{
		const EAgEffect Effect = static_cast<EAgEffect>(NativeEffectEnum->GetValueByIndex(Index));
		NativeEffects.Add(Effect, AgEffectData::MakeNativeEffect(Effect, *this));
	}
	LoadJsonData();
}

bool UAgAssetManager::LoadJsonData()
{
	++DataVersion;
	TObjectPtr<UAgCombatRules> NewRules = NewObject<UAgCombatRules>(this);
	TObjectPtr<UAgCameraData> NewCamera = NewObject<UAgCameraData>(this);
	UAgEffectSet* NewEffects = NewObject<UAgEffectSet>(this);
	UAgProjectileSet* NewProjectiles = NewObject<UAgProjectileSet>(this);
	UAgMontageTimelineData* NewTimelines = NewObject<UAgMontageTimelineData>(this);
	UAgTargetingPresetSet* NewTargetingPresets = NewObject<UAgTargetingPresetSet>(this);
	UAgAbilitySet* NewAbilities = NewObject<UAgAbilitySet>(this);
	JsonLoadTargets.Reset();
	JsonLoadTargets.Add(NewRules);
	JsonLoadTargets.Add(NewCamera);
	JsonLoadTargets.Add(NewEffects);
	JsonLoadTargets.Add(NewProjectiles);
	JsonLoadTargets.Add(NewTimelines);
	JsonLoadTargets.Add(NewTargetingPresets);
	JsonLoadTargets.Add(NewAbilities);
	TMap<FName, TObjectPtr<UAgCharacterData>> NewCharacters;
	for (const TPair<FName, TObjectPtr<UAgCharacterData>>& Pair : Characters)
	{
		UAgCharacterData* Candidate = NewObject<UAgCharacterData>(this, Pair.Value->GetClass());
		NewCharacters.Add(Pair.Key, Candidate);
		JsonLoadTargets.Add(Candidate);
	}
	JsonIds.Reset();
	ON_SCOPE_EXIT
	{
		JsonLoadTargets.Reset();
	};

	const auto Read = [this](const TCHAR* FileName, const UStruct* Owner, FName PropertyName, void* Out)
	{
		return ReadJson(FString(JsonFolderName) / FileName, nullptr, *FindFProperty<FProperty>(Owner, PropertyName), Out);
	};
	// 다른 파일이 ID로 가리키는 정의 파일을 먼저 읽습니다. 오류를 모두 알리도록 실패해도 다음 파일을 읽습니다.
	bool bRead = Read(EffectsFile, UAgEffectSet::StaticClass(), GET_MEMBER_NAME_CHECKED(UAgEffectSet, Definitions), &NewEffects->Definitions);
	bRead = Read(CombatRulesFile, StaticClass(), GET_MEMBER_NAME_CHECKED(UAgAssetManager, CombatRules), &NewRules) && bRead;
	bRead = Read(CameraDataFile, StaticClass(), GET_MEMBER_NAME_CHECKED(UAgAssetManager, CameraData), &NewCamera) && bRead;
	bRead = Read(ProjectilesFile, UAgProjectileSet::StaticClass(), GET_MEMBER_NAME_CHECKED(UAgProjectileSet, Projectiles), &NewProjectiles->Projectiles) && bRead;
	bRead = Read(TargetingPresetsFile, UAgTargetingPresetSet::StaticClass(), GET_MEMBER_NAME_CHECKED(UAgTargetingPresetSet, Definitions), &NewTargetingPresets->Definitions) && bRead;
	bRead = Read(AbilitiesFile, UAgAbilitySet::StaticClass(), GET_MEMBER_NAME_CHECKED(UAgAbilitySet, Abilities), &NewAbilities->Abilities) && bRead;
	bRead = Read(MontageTimelinesFile, UAgMontageTimelineData::StaticClass(), GET_MEMBER_NAME_CHECKED(UAgMontageTimelineData, Timelines), &NewTimelines->Timelines) && bRead;
	bRead = Read(CharactersFile, StaticClass(), GET_MEMBER_NAME_CHECKED(UAgAssetManager, Characters), &NewCharacters) && bRead;
	if (!bRead)
	{
		return false;
	}

	for (const TPair<FName, FAgEffectDefinition>& Pair : NewEffects->Definitions)
	{
		NewEffects->Effects.Add(Pair.Key, AgEffectData::MakeEffect(Pair.Key, Pair.Value, *this, DataVersion));
	}
	for (const TPair<FName, FAgTargetingPresetDefinition>& Pair : NewTargetingPresets->Definitions)
	{
		UTargetingPreset* Preset = NewObject<UTargetingPreset>(this, FName(*FString::Printf(TEXT("%s_%d"), *Pair.Key.ToString(), DataVersion)));
		Preset->GetMutableTargetingTaskSet().Tasks = Pair.Value.Tasks;
		NewTargetingPresets->Presets.Add(Pair.Key, Preset);
	}

	CopyJsonProperties(*NewRules, *CombatRules);
	CopyJsonProperties(*NewCamera, *CameraData);
	CopyJsonProperties(*NewEffects, *Effects);
	CopyJsonProperties(*NewProjectiles, *Projectiles);
	CopyJsonProperties(*NewTimelines, *MontageTimelines);
	CopyJsonProperties(*NewTargetingPresets, *TargetingPresets);
	CopyJsonProperties(*NewAbilities, *Abilities);
	for (const TPair<FName, TObjectPtr<UAgCharacterData>>& Pair : NewCharacters)
	{
		UAgCharacterData& Target = *Characters.FindChecked(Pair.Key);
		CopyJsonProperties(*Pair.Value, Target);
		Target.MarkJsonDataLoaded();
	}
	return true;
}

bool UAgAssetManager::ReadJson(const FString& Path, TSharedPtr<FJsonValue> Value, const FProperty& Property, void* Out)
{
	const auto Fail = [](const FString& At, const FString& Reason)
	{
		UE_LOG(LogAg, Error, TEXT("%s: %s"), *At, *Reason);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 15.f, FColor::Red, At + TEXT(": ") + Reason);
		}
		return false;
	};
	const auto Child = [&Path](const FString& Key)
	{
		return Path.EndsWith(TEXT(".json")) ? Path + TEXT(": ") + Key : Path + TEXT(".") + Key;
	};

	if (!Value)
	{
		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectDir(), Path)))
		{
			return Fail(Path, TEXT("can't read the file"));
		}
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
		if (!FJsonSerializer::Deserialize(Reader, Value) || !Value.IsValid())
		{
			return Fail(Path, Reader->GetErrorMessage());
		}
		const TSharedPtr<FJsonObject>* Root = nullptr;
		if (Value->TryGetObject(Root))
		{
			TSet<FName>& Ids = JsonIds.Add(FName(FPaths::GetBaseFilename(Path)));
			for (const TPair<FJsonObject::FStringType, TSharedPtr<FJsonValue>>& Pair : (*Root)->Values)
			{
				Ids.Add(FName(*FString(Pair.Key)));
			}
		}
	}
	if (Value->IsNull())
	{
		return true;
	}

	if (const FOptionalProperty* Optional = CastField<FOptionalProperty>(&Property))
	{
		return ReadJson(Path, Value, *Optional->GetValueProperty(), Optional->MarkSetAndGetInitializedValuePointerToReplace(Out));
	}
	if (const FArrayProperty* Array = CastField<FArrayProperty>(&Property))
	{
		const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
		if (!Value->TryGetArray(Items))
		{
			return Fail(Path, TEXT("must be an array"));
		}
		FScriptArrayHelper Helper(Array, Out);
		Helper.Resize(Items->Num());
		bool bRead = true;
		for (int32 Index = 0; Index < Items->Num(); ++Index)
		{
			bRead = ReadJson(FString::Printf(TEXT("%s[%d]"), *Path, Index), (*Items)[Index], *Array->Inner, Helper.GetRawPtr(Index)) && bRead;
		}
		return bRead;
	}
	if (const FMapProperty* Map = CastField<FMapProperty>(&Property))
	{
		const TSharedPtr<FJsonObject>* Object = nullptr;
		if (!Value->TryGetObject(Object))
		{
			return Fail(Path, TEXT("must be an object"));
		}
		FScriptMapHelper Helper(Map, Out);
		bool bRead = true;
		for (const TPair<FJsonObject::FStringType, TSharedPtr<FJsonValue>>& Entry : (*Object)->Values)
		{
			const FString Key(Entry.Key);
			FDefaultConstructedPropertyElement KeyValue(Map->KeyProp);
			if (!ReadJson(Child(Key), MakeShared<FJsonValueString>(Key), *Map->KeyProp, KeyValue.GetObjAddress()))
			{
				bRead = false;
				continue;
			}
			// 이미 있는 항목(캐릭터 데이터)에는 그 객체에 읽습니다.
			bRead = ReadJson(Child(Key), Entry.Value, *Map->ValueProp, Helper.FindOrAdd(KeyValue.GetObjAddress())) && bRead;
		}
		return bRead;
	}

	const FStructProperty* StructProperty = CastField<FStructProperty>(&Property);
	const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(&Property);
	const UStruct* Type = nullptr;
	void* Container = nullptr;
	const TCHAR* TypeKey = nullptr;
	if (StructProperty && StructProperty->Struct == FGameplayTagContainer::StaticStruct())
	{
		const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
		if (!Value->TryGetArray(Items))
		{
			return Fail(Path, TEXT("must be an array of gameplay tags"));
		}
		FGameplayTagContainer& Tags = *static_cast<FGameplayTagContainer*>(Out);
		Tags.Reset();
		bool bRead = true;
		for (int32 Index = 0; Index < Items->Num(); ++Index)
		{
			const FString Name = (*Items)[Index]->Type == EJson::String ? (*Items)[Index]->AsString() : FString();
			const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*Name), false);
			if (Tag.IsValid())
			{
				Tags.AddTag(Tag);
			}
			else
			{
				bRead = Fail(FString::Printf(TEXT("%s[%d]"), *Path, Index), Name + TEXT(" is not a gameplay tag"));
			}
		}
		return bRead;
	}
	if (StructProperty && StructProperty->Struct == FGameplayTag::StaticStruct())
	{
		// 엔진 변환은 등록되지 않은 태그 이름도 그대로 받으므로 직접 찾습니다.
		const FString Name = Value->Type == EJson::String ? Value->AsString() : FString();
		const FGameplayTag Tag = Name.IsEmpty() ? FGameplayTag() : FGameplayTag::RequestGameplayTag(FName(*Name), false);
		if (!Name.IsEmpty() && !Tag.IsValid())
		{
			return Fail(Path, Name + TEXT(" is not a gameplay tag"));
		}
		*static_cast<FGameplayTag*>(Out) = Tag;
		return true;
	}
	if (StructProperty && StructProperty->Struct == FGameplayAttribute::StaticStruct())
	{
		// 엔진 형식은 필드 경로를 담은 객체라 길어서, 어트리뷰트 세트의 속성 이름만 적습니다.
		const FString Name = Value->Type == EJson::String ? Value->AsString() : FString();
		TArray<FProperty*> Attributes;
		FGameplayAttribute::GetAllAttributeProperties(Attributes, FString(), false);
		FProperty* const* Attribute = Attributes.FindByPredicate([&Name](const FProperty* Property)
		{
			return Property->GetFName() == FName(*Name) && FGameplayAttribute::IsGameplayAttributeDataProperty(Property);
		});
		if (!Attribute)
		{
			return Fail(Path, Name + TEXT(" is not an attribute"));
		}
		*static_cast<FGameplayAttribute*>(Out) = FGameplayAttribute(*Attribute);
		return true;
	}
	if (StructProperty && StructProperty->Struct == FInstancedStruct::StaticStruct())
	{
		const TSharedPtr<FJsonObject>* Object = nullptr;
		FString TaskName;
		const UScriptStruct* Task =
			Value->TryGetObject(Object) && (*Object)->TryGetStringField(TEXT("Task"), TaskName) ? AgTaskParams::FindStruct(TaskName) : nullptr;
		if (!Task)
		{
			return Fail(Path, TEXT("Task must name a task: ") + TaskName);
		}
		FInstancedStruct& Instance = *static_cast<FInstancedStruct*>(Out);
		Instance.InitializeAs(Task);
		Type = Task;
		Container = Instance.GetMutableMemory();
		TypeKey = TEXT("Task");
	}
	else if (ObjectProperty && Value->Type == EJson::Object)
	{
		UObject* Object = ObjectProperty->GetObjectPropertyValue(Out);
		FString ClassPath;
		if (!Object && Value->AsObject()->TryGetStringField(TEXT("Class"), ClassPath))
		{
			UClass* Class = LoadClass<UObject>(nullptr, *ClassPath, nullptr, LOAD_NoWarn);
			if (!Class || !Class->IsChildOf(ObjectProperty->PropertyClass) || Class->HasAnyClassFlags(CLASS_Abstract))
			{
				return Fail(Path, ClassPath + TEXT(" is not a class of ") + ObjectProperty->PropertyClass->GetName());
			}
			Object = NewObject<UObject>(this, Class);
			ObjectProperty->SetObjectPropertyValue(Out, Object);
			TypeKey = TEXT("Class");
		}
		if (!Object)
		{
			return Fail(Path, TEXT("unknown entry"));
		}
		Type = Object->GetClass();
		Container = Object;
	}
	else if (StructProperty)
	{
		// UPROPERTY는 맵 값으로 배열을 담지 못해 배열·맵을 구조체로 감싸므로, 감싸는 키 없이 그 값을 바로 적습니다.
		TArray<const FProperty*> Fields;
		for (TFieldIterator<FProperty> It(StructProperty->Struct); It; ++It)
		{
			Fields.Add(*It);
		}
		if (Fields.Num() == 1 && (Fields[0]->IsA<FArrayProperty>() || Fields[0]->IsA<FMapProperty>()))
		{
			return ReadJson(Path, Value, *Fields[0], Fields[0]->ContainerPtrToValuePtr<void>(Out));
		}
		if (Value->Type == EJson::Object)
		{
			Type = StructProperty->Struct;
			Container = Out;
		}
	}

	if (Type)
	{
		const TSharedPtr<FJsonObject>* Object = nullptr;
		if (!Value->TryGetObject(Object))
		{
			return Fail(Path, TEXT("must be an object"));
		}
		const bool bObject = Type->IsChildOf(UObject::StaticClass());
		bool bRead = true;
		for (const TPair<FJsonObject::FStringType, TSharedPtr<FJsonValue>>& Field : (*Object)->Values)
		{
			const FString Key(Field.Key);
			if (TypeKey && Key == TypeKey)
			{
				continue;
			}
			// 쿡한 빌드는 속성 이름의 대소문자를 보존하지 않으므로(예: UP → Up) 키는 대소문자를 구분하지 않습니다.
			const FProperty* Found = nullptr;
			for (TFieldIterator<FProperty> It(Type); It && !Found; ++It)
			{
				const bool bReadable = !bObject || It->HasAnyPropertyFlags(CPF_Transient) ||
				                       (It->HasAnyPropertyFlags(CPF_Edit) && !It->HasAnyPropertyFlags(CPF_EditConst | CPF_DisableEditOnInstance));
				if (bReadable && !It->HasAnyPropertyFlags(CPF_Deprecated) && It->GetName().Equals(Key, ESearchCase::IgnoreCase))
				{
					Found = *It;
				}
			}
			if (!Found)
			{
				bRead = Fail(Child(Key), TEXT("unknown key"));
				continue;
			}
			bRead = ReadJson(Child(Key), Field.Value, *Found, Found->ContainerPtrToValuePtr<void>(Container)) && bRead;
		}
		return bRead;
	}

	const FByteProperty* Byte = CastField<FByteProperty>(&Property);
	if (Byte && Byte->Enum == StaticEnum<EObjectTypeQuery>())
	{
		// 엔진 값(ObjectTypeQuery3)은 프로젝트 충돌 설정의 순서로 정해지므로, 에디터에 보이는 채널 이름을 그 값으로 바꿉니다.
		const FString Name = Value->Type == EJson::String ? Value->AsString() : FString();
		const int64 Channel = StaticEnum<ECollisionChannel>()->GetValueByNameString(TEXT("ECC_") + Name);
		const EObjectTypeQuery ObjectType =
			Channel == INDEX_NONE ? ObjectTypeQuery_MAX : UEngineTypes::ConvertToObjectType(static_cast<ECollisionChannel>(Channel));
		if (ObjectType == ObjectTypeQuery_MAX)
		{
			return Fail(Path, Name + TEXT(" is not an object channel"));
		}
		Byte->SetPropertyValue(Out, static_cast<uint8>(ObjectType));
		return true;
	}

	FText Reason;
	if (!FJsonObjectConverter::JsonValueToUProperty(Value, const_cast<FProperty*>(&Property), Out, 0, 0, false, &Reason))
	{
		return Fail(Path, Reason.ToString());
	}
	// 엔진 변환은 찾지 못한 에셋·클래스를 null로 두고 성공으로 돌려주므로 직접 확인합니다.
	const FObjectProperty* Reference = CastField<FObjectProperty>(&Property);
	if (Reference && !Reference->GetObjectPropertyValue(Out) && !Value->AsString().IsEmpty() && Value->AsString() != TEXT("None"))
	{
		return Fail(Path, Value->AsString() + TEXT(" is not an asset or class of ") + Reference->PropertyClass->GetName());
	}
#if WITH_METADATA
	// 배열 원소의 메타데이터는 배열 속성에 있습니다.
	const FString* DataId = CastField<FNameProperty>(&Property) ? Property.GetOwnerProperty()->FindMetaData(TEXT("DataId")) : nullptr;
	const TSet<FName>* Ids = DataId ? JsonIds.Find(FName(**DataId)) : nullptr;
	if (DataId && (!Ids || !Ids->Contains(*static_cast<const FName*>(Out))))
	{
		return Fail(Path, Value->AsString() + TEXT(" is not an ID in ") + *DataId + TEXT(".json"));
	}
#endif
	return true;
}
