// Copyright Woogle. All Rights Reserved.

#include "Core/AgJsonReader.h"

#include "Ag.h"
#include "AttributeSet.h"
#include "Data/AgHitReward.h"
#include "Data/AgTaskParams.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "StructUtils/InstancedStruct.h"
#include "UObject/PropertyOptional.h"
#include "UObject/UnrealType.h"

namespace AgJsonReaderPrivate
{
	bool Fail(const FString& At, const FString& Reason)
	{
		UE_LOG(LogAg, Error, TEXT("%s: %s"), *At, *Reason);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 15.f, FColor::Red, At + TEXT(": ") + Reason);
		}
		return false;
	}

	FString ChildPath(const FString& Path, const FString& Key)
	{
		return Path.EndsWith(TEXT(".json")) ? Path + TEXT(": ") + Key : Path + TEXT(".") + Key;
	}

	/** 읽지 못한 에셋·클래스·정의 ID·Task는 비어 있습니다. 배열·맵에 남기면 쓰는 코드가 빈 원소를 만나므로 원소째 뺍니다. */
	bool IsEmptyReference(const FProperty& Property, const void* Value)
	{
		if (const FObjectPropertyBase* Object = CastField<FObjectPropertyBase>(&Property))
		{
			return !Object->GetObjectPropertyValue(Value);
		}
		if (const FNameProperty* Name = CastField<FNameProperty>(&Property))
		{
			return Name->GetPropertyValue(Value).IsNone();
		}
		const FStructProperty* Struct = CastField<FStructProperty>(&Property);
		return Struct && Struct->Struct == FInstancedStruct::StaticStruct() && !static_cast<const FInstancedStruct*>(Value)->IsValid();
	}

	bool ReadTagContainer(const FString& Path, const FJsonValue& Value, FGameplayTagContainer& Out)
	{
		const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
		if (!Value.TryGetArray(Items))
		{
			return Fail(Path, TEXT("must be an array of gameplay tags"));
		}
		Out.Reset();
		bool bRead = true;
		for (int32 Index = 0; Index < Items->Num(); ++Index)
		{
			const FString Name = (*Items)[Index]->Type == EJson::String ? (*Items)[Index]->AsString() : FString();
			const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*Name), false);
			if (Tag.IsValid())
			{
				Out.AddTag(Tag);
			}
			else
			{
				bRead = Fail(FString::Printf(TEXT("%s[%d]"), *Path, Index), Name + TEXT(" is not a gameplay tag"));
			}
		}
		return bRead;
	}

	bool ReadTag(const FString& Path, const FJsonValue& Value, FGameplayTag& Out)
	{
		if (Value.Type != EJson::String)
		{
			return Fail(Path, TEXT("must be a gameplay tag string"));
		}
		// 엔진 변환은 등록되지 않은 태그 이름도 그대로 받으므로 직접 찾습니다.
		const FString Name = Value.AsString();
		const FGameplayTag Tag = Name.IsEmpty() ? FGameplayTag() : FGameplayTag::RequestGameplayTag(FName(*Name), false);
		if (!Name.IsEmpty() && !Tag.IsValid())
		{
			return Fail(Path, Name + TEXT(" is not a gameplay tag"));
		}
		Out = Tag;
		return true;
	}

	bool ReadAttribute(const FString& Path, const FJsonValue& Value, FGameplayAttribute& Out)
	{
		// 엔진 형식은 필드 경로를 담은 객체라 길어서, 어트리뷰트 세트의 속성 이름만 적습니다.
		const FString Name = Value.Type == EJson::String ? Value.AsString() : FString();
		TArray<FProperty*> Attributes;
		FGameplayAttribute::GetAllAttributeProperties(Attributes, FString(), false);
		FProperty* const* Attribute = Attributes.FindByPredicate([&Name](const FProperty* Candidate)
		{
			return Candidate->GetFName() == FName(*Name) && FGameplayAttribute::IsGameplayAttributeDataProperty(Candidate);
		});
		if (!Attribute)
		{
			return Fail(Path, Name + TEXT(" is not an attribute"));
		}
		Out = FGameplayAttribute(*Attribute);
		return true;
	}
} // namespace AgJsonReaderPrivate

FAgJsonReader::FAgJsonReader(UObject& InOuter)
	: Outer(InOuter)
{
}

bool FAgJsonReader::ReadFile(const FString& Path, const FProperty& Property, void* Out)
{
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectDir(), Path)))
	{
		return AgJsonReaderPrivate::Fail(Path, TEXT("can't read the file"));
	}
	TSharedPtr<FJsonValue> Value;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
	if (!FJsonSerializer::Deserialize(Reader, Value) || !Value.IsValid())
	{
		return AgJsonReaderPrivate::Fail(Path, Reader->GetErrorMessage());
	}
	const TSharedPtr<FJsonObject>* Root = nullptr;
	if (Value->TryGetObject(Root))
	{
		TSet<FName>& FileIds = Ids.Add(FName(FPaths::GetBaseFilename(Path)));
		for (const TPair<FJsonObject::FStringType, TSharedPtr<FJsonValue>>& Pair : (*Root)->Values)
		{
			if (!Pair.Value->IsNull())
			{
				FileIds.Add(FName(*FString(Pair.Key)));
			}
		}
	}
	return Read(Path, Value, Property, Out);
}

bool FAgJsonReader::IsField(const FProperty& Property)
{
	// `Class`로 만드는 엔진 객체(대상 프리셋의 Task)는 편집 가능한 속성으로 설정합니다.
	const bool bField = Property.HasAnyPropertyFlags(CPF_Transient) ||
	                    (Property.HasAnyPropertyFlags(CPF_Edit) && !Property.HasAnyPropertyFlags(CPF_EditConst | CPF_DisableEditOnInstance));
	return bField && !Property.HasAnyPropertyFlags(CPF_Deprecated);
}

bool FAgJsonReader::Read(const FString& Path, const TSharedPtr<FJsonValue>& Value, const FProperty& Property, void* Out)
{
	if (Value->IsNull())
	{
		return true;
	}

	if (const FOptionalProperty* Optional = CastField<FOptionalProperty>(&Property))
	{
		return Read(Path, Value, *Optional->GetValueProperty(), Optional->MarkSetAndGetInitializedValuePointerToReplace(Out));
	}
	if (const FArrayProperty* Array = CastField<FArrayProperty>(&Property))
	{
		return ReadArray(Path, Value, *Array, Out);
	}
	if (const FMapProperty* Map = CastField<FMapProperty>(&Property))
	{
		return ReadMap(Path, Value, *Map, Out);
	}
	if (const FObjectProperty* Object = CastField<FObjectProperty>(&Property); Object && Value->Type == EJson::Object)
	{
		return ReadObject(Path, Value, *Object, Out);
	}
	if (const FStructProperty* Struct = CastField<FStructProperty>(&Property))
	{
		if (Struct->Struct == FGameplayTagContainer::StaticStruct())
		{
			return AgJsonReaderPrivate::ReadTagContainer(Path, *Value, *static_cast<FGameplayTagContainer*>(Out));
		}
		if (Struct->Struct == FGameplayTag::StaticStruct())
		{
			return AgJsonReaderPrivate::ReadTag(Path, *Value, *static_cast<FGameplayTag*>(Out));
		}
		if (Struct->Struct == FGameplayAttribute::StaticStruct())
		{
			return AgJsonReaderPrivate::ReadAttribute(Path, *Value, *static_cast<FGameplayAttribute*>(Out));
		}
		if (Struct->Struct == FInstancedStruct::StaticStruct())
		{
			return ReadTask(Path, Value, *static_cast<FInstancedStruct*>(Out));
		}
		return ReadStruct(Path, Value, *Struct, Out);
	}
	return ReadValue(Path, Value, Property, Out);
}

bool FAgJsonReader::ReadArray(const FString& Path, const TSharedPtr<FJsonValue>& Value, const FArrayProperty& Property, void* Out)
{
	const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
	if (!Value->TryGetArray(Items))
	{
		return AgJsonReaderPrivate::Fail(Path, TEXT("must be an array"));
	}
	FScriptArrayHelper Helper(&Property, Out);
	Helper.Resize(Items->Num());
	bool bRead = true;
	for (int32 Index = 0, Slot = 0; Index < Items->Num(); ++Index)
	{
		const bool bItem = Read(FString::Printf(TEXT("%s[%d]"), *Path, Index), (*Items)[Index], *Property.Inner, Helper.GetRawPtr(Slot));
		bRead = bItem && bRead;
		if (!bItem && AgJsonReaderPrivate::IsEmptyReference(*Property.Inner, Helper.GetRawPtr(Slot)))
		{
			Helper.RemoveValues(Slot);
		}
		else
		{
			++Slot;
		}
	}
	return bRead;
}

bool FAgJsonReader::ReadMap(const FString& Path, const TSharedPtr<FJsonValue>& Value, const FMapProperty& Property, void* Out)
{
	const TSharedPtr<FJsonObject>* Object = nullptr;
	if (!Value->TryGetObject(Object))
	{
		return AgJsonReaderPrivate::Fail(Path, TEXT("must be an object"));
	}
	FScriptMapHelper Helper(&Property, Out);
	bool bRead = true;
	for (const TPair<FJsonObject::FStringType, TSharedPtr<FJsonValue>>& Entry : (*Object)->Values)
	{
		if (Entry.Value->IsNull())
		{
			continue;
		}
		const FString Key(Entry.Key);
		FDefaultConstructedPropertyElement KeyValue(Property.KeyProp);
		if (!Read(AgJsonReaderPrivate::ChildPath(Path, Key), MakeShared<FJsonValueString>(Key), *Property.KeyProp, KeyValue.GetObjAddress()))
		{
			bRead = false;
			continue;
		}
		void* Item = Helper.FindOrAdd(KeyValue.GetObjAddress());
		if (!Read(AgJsonReaderPrivate::ChildPath(Path, Key), Entry.Value, *Property.ValueProp, Item))
		{
			bRead = false;
			if (AgJsonReaderPrivate::IsEmptyReference(*Property.ValueProp, Item))
			{
				Helper.RemovePair(KeyValue.GetObjAddress());
			}
		}
	}
	return bRead;
}

bool FAgJsonReader::ReadTask(const FString& Path, const TSharedPtr<FJsonValue>& Value, FInstancedStruct& Out)
{
	const TSharedPtr<FJsonObject>* Object = nullptr;
	FString TaskName;
	const UScriptStruct* Task =
		Value->TryGetObject(Object) && (*Object)->TryGetStringField(TEXT("Task"), TaskName) ? AgTaskParams::FindStruct(TaskName) : nullptr;
	if (!Task)
	{
		return AgJsonReaderPrivate::Fail(Path, TEXT("Task must name a task: ") + TaskName);
	}
	Out.InitializeAs(Task);
	return ReadFields(Path, **Object, *Task, Out.GetMutableMemory(), TEXT("Task"));
}

bool FAgJsonReader::ReadObject(const FString& Path, const TSharedPtr<FJsonValue>& Value, const FObjectProperty& Property, void* Out)
{
	const FJsonObject& Json = *Value->AsObject();
	UObject* Object = Property.GetObjectPropertyValue(Out);
	UClass* Class = Property.PropertyClass;
	const TCHAR* TypeKey = nullptr;
	FString ClassPath;
	if (!Object && Json.TryGetStringField(TEXT("Class"), ClassPath))
	{
		Class = LoadClass<UObject>(nullptr, *ClassPath, nullptr, LOAD_NoWarn);
		if (!Class || !Class->IsChildOf(Property.PropertyClass) || Class->HasAnyClassFlags(CLASS_Abstract))
		{
			return AgJsonReaderPrivate::Fail(Path, ClassPath + TEXT(" is not a class of ") + Property.PropertyClass->GetName());
		}
		TypeKey = TEXT("Class");
	}
	if (!Object && !Class->HasAnyClassFlags(CLASS_Abstract))
	{
		// 액터는 월드 밖에서 엔진 스폰 템플릿(FActorSpawnParameters::Template)으로만 쓰므로 엔진의 자식 액터 템플릿처럼
		// 아키타입으로 만듭니다.
		const EObjectFlags Flags = Class->IsChildOf<AActor>() ? RF_ArchetypeObject : RF_NoFlags;
		Object = NewObject<UObject>(&Outer, Class, NAME_None, Flags);
		Property.SetObjectPropertyValue(Out, Object);
	}
	if (!Object)
	{
		return AgJsonReaderPrivate::Fail(Path, TEXT("unknown entry"));
	}
	return ReadFields(Path, Json, *Object->GetClass(), Object, TypeKey);
}

bool FAgJsonReader::ReadStruct(const FString& Path, const TSharedPtr<FJsonValue>& Value, const FStructProperty& Property, void* Out)
{
	// UPROPERTY는 맵 값으로 배열을 담지 못해 배열·맵을 구조체로 감싸므로, 감싸는 키 없이 그 값을 바로 적습니다.
	TArray<const FProperty*> Fields;
	for (TFieldIterator<FProperty> It(Property.Struct); It; ++It)
	{
		Fields.Add(*It);
	}
	if (Fields.Num() == 1 && (Fields[0]->IsA<FArrayProperty>() || Fields[0]->IsA<FMapProperty>()))
	{
		return Read(Path, Value, *Fields[0], Fields[0]->ContainerPtrToValuePtr<void>(Out));
	}
	// 객체로 적지 않은 구조체(벡터, 경로 등)는 엔진 변환이 읽습니다.
	if (Value->Type != EJson::Object)
	{
		return ReadValue(Path, Value, Property, Out);
	}
	if (!ReadFields(Path, *Value->AsObject(), *Property.Struct, Out))
	{
		return false;
	}
	if (Property.Struct == FAgHitReward::StaticStruct())
	{
		const FAgHitReward& Reward = *static_cast<const FAgHitReward*>(Out);
		if (!FMath::IsFinite(Reward.MP) || !FMath::IsFinite(Reward.UP) || Reward.MP < 0.f || Reward.UP < 0.f || (Reward.MP == 0.f && Reward.UP == 0.f))
		{
			return AgJsonReaderPrivate::Fail(Path, TEXT("HitReward must contain finite non-negative amounts with at least one positive amount"));
		}
	}
	return true;
}

bool FAgJsonReader::ReadFields(const FString& Path, const FJsonObject& Object, const UStruct& Type, void* Container, const TCHAR* TypeKey)
{
	const bool bObject = Type.IsChildOf(UObject::StaticClass());
	bool bRead = true;
	for (const TPair<FJsonObject::FStringType, TSharedPtr<FJsonValue>>& Field : Object.Values)
	{
		const FString Key(Field.Key);
		if (TypeKey && Key == TypeKey)
		{
			continue;
		}
		// 쿡한 빌드는 속성 이름의 대소문자를 보존하지 않지만(예: UP → Up) FName 비교는 대소문자를 구분하지 않습니다.
		const FProperty* Found = Type.FindPropertyByName(FName(*Key));
		if (!Found || (bObject && !IsField(*Found)) || Found->HasAnyPropertyFlags(CPF_Deprecated))
		{
			bRead = AgJsonReaderPrivate::Fail(AgJsonReaderPrivate::ChildPath(Path, Key), TEXT("unknown key"));
			continue;
		}
		bRead = Read(AgJsonReaderPrivate::ChildPath(Path, Key), Field.Value, *Found, Found->ContainerPtrToValuePtr<void>(Container)) && bRead;
	}
	return bRead;
}

bool FAgJsonReader::ReadValue(const FString& Path, const TSharedPtr<FJsonValue>& Value, const FProperty& Property, void* Out)
{
	FText Reason;
	if (!FJsonObjectConverter::JsonValueToUProperty(Value, const_cast<FProperty*>(&Property), Out, 0, 0, false, &Reason))
	{
		return AgJsonReaderPrivate::Fail(Path, Reason.ToString());
	}
	// 엔진 변환은 찾지 못한 에셋·클래스를 null로 두고 성공으로 돌려주므로 직접 확인합니다.
	const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(&Property);
	const UObject* Referenced = ObjectProperty ? ObjectProperty->GetObjectPropertyValue(Out) : nullptr;
	if (ObjectProperty && !Referenced && !Value->AsString().IsEmpty() && Value->AsString() != TEXT("None"))
	{
		return AgJsonReaderPrivate::Fail(Path, Value->AsString() + TEXT(" is not an asset or class of ") + ObjectProperty->PropertyClass->GetName());
	}
	// 엔진 로더는 리다이렉터를 따라가 옛 경로도 읽지만, 에디터는 JSON 경로를 참조로 모르므로 리다이렉터가 지워지기 전에 알립니다.
	if (Referenced && !Referenced->GetPathName().Equals(Value->AsString(), ESearchCase::IgnoreCase))
	{
		const FString Message = Path + TEXT(": ") + Value->AsString() + TEXT(" is redirected to ") + Referenced->GetPathName() + TEXT("; use the new path");
		UE_LOG(LogAg, Warning, TEXT("%s"), *Message);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 15.f, FColor::Yellow, Message);
		}
	}
#if WITH_METADATA
	// 배열 원소의 메타데이터는 배열 속성에 있습니다.
	const FString* DataId = CastField<FNameProperty>(&Property) ? Property.GetOwnerProperty()->FindMetaData(TEXT("DataId")) : nullptr;
	const TSet<FName>* FileIds = DataId ? Ids.Find(FName(**DataId)) : nullptr;
	if (DataId && (!FileIds || !FileIds->Contains(*static_cast<const FName*>(Out))))
	{
		*static_cast<FName*>(Out) = NAME_None;
		return AgJsonReaderPrivate::Fail(Path, Value->AsString() + TEXT(" is not an ID in ") + *DataId + TEXT(".json"));
	}
#endif
	return true;
}
