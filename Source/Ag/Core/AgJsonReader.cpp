// Copyright Woogle. All Rights Reserved.

#include "Core/AgJsonReader.h"

#include "Ag.h"
#include "AttributeSet.h"
#include "Data/AgHitReward.h"
#include "Data/AgTaskParams.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "GameplayTagContainer.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "StructUtils/InstancedStruct.h"
#include "UObject/PropertyOptional.h"
#include "UObject/UnrealType.h"

namespace
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
} // namespace

FAgJsonReader::FAgJsonReader(UObject& InOuter)
	: Outer(InOuter)
{
}

bool FAgJsonReader::ReadFile(const FString& Path, const FProperty& Property, void* Out)
{
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectDir(), Path)))
	{
		return Fail(Path, TEXT("can't read the file"));
	}
	TSharedPtr<FJsonValue> Value;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
	if (!FJsonSerializer::Deserialize(Reader, Value) || !Value.IsValid())
	{
		return Fail(Path, Reader->GetErrorMessage());
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
		const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
		if (!Value->TryGetArray(Items))
		{
			return Fail(Path, TEXT("must be an array"));
		}
		FScriptArrayHelper Helper(Array, Out);
		Helper.Resize(Items->Num());
		bool bRead = true;
		for (int32 Index = 0, Slot = 0; Index < Items->Num(); ++Index)
		{
			const bool bItem = Read(FString::Printf(TEXT("%s[%d]"), *Path, Index), (*Items)[Index], *Array->Inner, Helper.GetRawPtr(Slot));
			bRead = bItem && bRead;
			if (!bItem && IsEmptyReference(*Array->Inner, Helper.GetRawPtr(Slot)))
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
			if (Entry.Value->IsNull())
			{
				continue;
			}
			const FString Key(Entry.Key);
			FDefaultConstructedPropertyElement KeyValue(Map->KeyProp);
			if (!Read(ChildPath(Path, Key), MakeShared<FJsonValueString>(Key), *Map->KeyProp, KeyValue.GetObjAddress()))
			{
				bRead = false;
				continue;
			}
			void* Item = Helper.FindOrAdd(KeyValue.GetObjAddress());
			if (!Read(ChildPath(Path, Key), Entry.Value, *Map->ValueProp, Item))
			{
				bRead = false;
				if (IsEmptyReference(*Map->ValueProp, Item))
				{
					Helper.RemovePair(KeyValue.GetObjAddress());
				}
			}
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
		if (Value->Type != EJson::String)
		{
			return Fail(Path, TEXT("must be a gameplay tag string"));
		}
		// 엔진 변환은 등록되지 않은 태그 이름도 그대로 받으므로 직접 찾습니다.
		const FString Name = Value->AsString();
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
		UClass* Class = ObjectProperty->PropertyClass;
		FString ClassPath;
		if (!Object && Value->AsObject()->TryGetStringField(TEXT("Class"), ClassPath))
		{
			Class = LoadClass<UObject>(nullptr, *ClassPath, nullptr, LOAD_NoWarn);
			if (!Class || !Class->IsChildOf(ObjectProperty->PropertyClass) || Class->HasAnyClassFlags(CLASS_Abstract))
			{
				return Fail(Path, ClassPath + TEXT(" is not a class of ") + ObjectProperty->PropertyClass->GetName());
			}
			TypeKey = TEXT("Class");
		}
		if (!Object && !Class->HasAnyClassFlags(CLASS_Abstract))
		{
			Object = NewObject<UObject>(&Outer, Class);
			ObjectProperty->SetObjectPropertyValue(Out, Object);
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
			return Read(Path, Value, *Fields[0], Fields[0]->ContainerPtrToValuePtr<void>(Out));
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
				if ((!bObject || IsField(**It)) && !It->HasAnyPropertyFlags(CPF_Deprecated) && It->GetName().Equals(Key, ESearchCase::IgnoreCase))
				{
					Found = *It;
				}
			}
			if (!Found)
			{
				bRead = Fail(ChildPath(Path, Key), TEXT("unknown key"));
				continue;
			}
			bRead = Read(ChildPath(Path, Key), Field.Value, *Found, Found->ContainerPtrToValuePtr<void>(Container)) && bRead;
		}
		if (bRead && Type == FAgHitReward::StaticStruct())
		{
			const FAgHitReward& Reward = *static_cast<const FAgHitReward*>(Container);
			if (!FMath::IsFinite(Reward.MP) || !FMath::IsFinite(Reward.UP) || Reward.MP < 0.f || Reward.UP < 0.f || (Reward.MP == 0.f && Reward.UP == 0.f))
			{
				return Fail(Path, TEXT("HitReward must contain finite non-negative amounts with at least one positive amount"));
			}
		}
		return bRead;
	}

	FText Reason;
	if (!FJsonObjectConverter::JsonValueToUProperty(Value, const_cast<FProperty*>(&Property), Out, 0, 0, false, &Reason))
	{
		return Fail(Path, Reason.ToString());
	}
	// 엔진 변환은 찾지 못한 에셋·클래스를 null로 두고 성공으로 돌려주므로 직접 확인합니다.
	const UObject* Referenced = ObjectProperty ? ObjectProperty->GetObjectPropertyValue(Out) : nullptr;
	if (ObjectProperty && !Referenced && !Value->AsString().IsEmpty() && Value->AsString() != TEXT("None"))
	{
		return Fail(Path, Value->AsString() + TEXT(" is not an asset or class of ") + ObjectProperty->PropertyClass->GetName());
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
		return Fail(Path, Value->AsString() + TEXT(" is not an ID in ") + *DataId + TEXT(".json"));
	}
#endif
	return true;
}
