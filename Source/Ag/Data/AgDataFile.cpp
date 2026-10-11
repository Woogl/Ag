// Copyright Woogle. All Rights Reserved.

#include "Data/AgDataFile.h"

#include "Ag.h"
#include "Data/AgCharacterData.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "GameplayTagContainer.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/UnrealType.h"

namespace AgDataFile
{
	namespace
	{
		/** The files sit outside Content, where the editor would try to import them as assets. */
		const TCHAR* const FolderName = TEXT("Data");

		/** Errors shown on screen per file. The log has all of them. */
		constexpr int32 MaxErrorsOnScreen = 5;

#if WITH_EDITOR
		struct FAgLoadedObject
		{
			TWeakObjectPtr<UObject> Object;
			FString FileName;
		};

		/** 파일에서 채운 모든 객체(ReloadFiles용). */
		TArray<FAgLoadedObject>& GetLoadedObjects()
		{
			static TArray<FAgLoadedObject> LoadedObjects;
			return LoadedObjects;
		}
#endif

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

		/** The JSON type a property's value is written as. */
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
			// Enums, names, strings, text, tags and classes.
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

		/** 값을 속성에 맞춰 검사합니다: JSON 형식, 그다음 그 형식에 필요한 것(필드, 태그, 클래스, 열거형 값, 정수, 범위). */
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
				TArray<const FProperty*> Fields;
				for (TFieldIterator<FProperty> It(CastFieldChecked<FStructProperty>(Property)->Struct); It; ++It)
				{
					Fields.Add(*It);
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
			else if (const FClassProperty* ClassProperty = CastField<FClassProperty>(Property))
			{
				// 데이터 파일에는 C++ 클래스만 둡니다. 에디터에서 만든 클래스는 에셋 참조라 데이터 에셋에 둡니다.
				const FString ClassPath = Value->AsString();
				const UClass* Class = ClassPath.IsEmpty() ? nullptr : FindObject<UClass>(nullptr, *ClassPath);
				if (!ClassPath.IsEmpty() && (!Class || !Class->HasAnyClassFlags(CLASS_Native) || !Class->IsChildOf(ClassProperty->MetaClass)))
				{
					Errors.Add(FString::Printf(TEXT("%s: %s is not a C++ class of %s"), *Path, *ClassPath, *ClassProperty->MetaClass->GetName()));
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
		 * 대소문자를 구분하지 않습니다. 쿠킹 빌드의 속성 이름은 대소문자를 보존하지 않습니다(예: UP → Up).
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

		/** Tags, classes and names from plain strings. CheckValue has validated them already. */
		bool ImportString(const TSharedPtr<FJsonValue>& JsonValue, FProperty* Property, void* Value)
		{
			if (IsTagProperty(Property))
			{
				const FString TagName = JsonValue->AsString();
				*static_cast<FGameplayTag*>(Value) = TagName.IsEmpty() ? FGameplayTag() : FGameplayTag::RequestGameplayTag(FName(*TagName));
				return true;
			}
			if (const FClassProperty* ClassProperty = CastField<FClassProperty>(Property))
			{
				const FString ClassPath = JsonValue->AsString();
				ClassProperty->SetObjectPropertyValue(Value, ClassPath.IsEmpty() ? nullptr : FindObject<UClass>(nullptr, *ClassPath));
				return true;
			}
			if (const FNameProperty* NameProperty = CastField<FNameProperty>(Property))
			{
				NameProperty->SetPropertyValue(Value, FName(*JsonValue->AsString()));
				return true;
			}
			return false;
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
			// Newer messages show on top, so the errors go in last to first and the summary after them.
			for (int32 Index = FMath::Min(Errors.Num(), MaxErrorsOnScreen) - 1; Index >= 0; --Index)
			{
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 15.f, FColor::Red, FString::Printf(TEXT("    %s"), *Errors[Index]));
			}
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 15.f, FColor::Red, FString::Printf(TEXT("%s was not applied, %d error(s) (all in the log):"), *File, Errors.Num()));
		}
	}

	FString GetFolder()
	{
		return FPaths::Combine(FPaths::ProjectDir(), FolderName);
	}

	FString GetPath(const FString& FileName)
	{
		return FPaths::Combine(GetFolder(), FileName);
	}

	bool Load(UObject& Target, const FString& FileName)
	{
#if WITH_EDITOR
		TArray<FAgLoadedObject>& LoadedObjects = GetLoadedObjects();
		LoadedObjects.RemoveAll([&Target](const FAgLoadedObject& Entry) { return !Entry.Object.IsValid() || Entry.Object.Get() == &Target; });
		LoadedObjects.Add({ &Target, FileName });
#endif

		TArray<FString> Errors;
		TSharedPtr<FJsonObject> Json;
		FString Text;
		if (FileName.IsEmpty())
		{
			Errors.Add(TEXT("no data file is set"));
		}
		else if (!FFileHelper::LoadFileToString(Text, *GetPath(FileName)))
		{
			Errors.Add(FString::Printf(TEXT("can't read %s"), *GetPath(FileName)));
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
			CheckObject(*Json, Properties, FString(), Errors);
		}

		// 값마다 먼저 따로 변환해 두므로, 하나라도 실패하면 대상은 그대로 남습니다.
		TArray<FDefaultConstructedPropertyElement> Values;
		if (Errors.IsEmpty())
		{
			const FJsonObjectConverter::CustomImportCallback ImportCallback = FJsonObjectConverter::CustomImportCallback::CreateStatic(&ImportString);
			Values.Reserve(Properties.Num());
			for (const FProperty* Property : Properties)
			{
				const FDefaultConstructedPropertyElement& Value = Values.Emplace_GetRef(Property);
				FText FailReason;
				if (!FJsonObjectConverter::JsonValueToUProperty(Json->TryGetField(Property->GetName()), const_cast<FProperty*>(Property), Value.GetObjAddress(), 0, 0, false, &FailReason, &ImportCallback))
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

#if WITH_EDITOR
	void ReloadFiles(const TArray<FString>& FileNames)
	{
		bool bReloaded = false;
		for (const FString& FileName : FileNames)
		{
			TArray<UObject*> Objects;
			for (const FAgLoadedObject& Entry : GetLoadedObjects())
			{
				UObject* Object = Entry.Object.Get();
				if (Object && Entry.FileName.Equals(FileName, ESearchCase::IgnoreCase))
				{
					Objects.Add(Object);
				}
			}
			if (Objects.IsEmpty())
			{
				continue;
			}
			bReloaded = true;

			bool bSucceeded = true;
			for (UObject* Object : Objects)
			{
				bSucceeded &= Load(*Object, FileName);
			}
			if (bSucceeded && GEngine)
			{
				// Stats are set once at spawn and are not applied again (구현 계획서 '데이터' 다시 읽기).
				const bool bCharacterData = Objects.ContainsByPredicate([](const UObject* Object) { return Object->IsA<UAgCharacterData>(); });
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Green, FString::Printf(TEXT("Data/%s reloaded.%s"), *FileName,
					bCharacterData ? TEXT(" Stats change on the next PIE start.") : TEXT("")));
			}
		}

		if (bReloaded)
		{
			OnReloaded().Broadcast();
		}
	}

	FSimpleMulticastDelegate& OnReloaded()
	{
		static FSimpleMulticastDelegate Delegate;
		return Delegate;
	}
#endif
}
