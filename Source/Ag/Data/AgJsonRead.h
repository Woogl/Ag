// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "JsonObjectConverter.h"
#include "UObject/UnrealType.h"

/** 어빌리티, 효과, 몽타주 시간표가 함께 쓰는 고정 스키마 JSON 읽기. 오류는 필드 경로와 함께 Errors에 더합니다. */
namespace AgJsonRead
{
	using FObject = TSharedPtr<FJsonObject>;

	/** Required 키가 모두 있고 Required·Optional 밖의 키가 없는지 확인합니다. */
	inline bool Keys(const FJsonObject& Object, const FString& Path, std::initializer_list<const TCHAR*> Required,
	    std::initializer_list<const TCHAR*> Optional, TArray<FString>& Errors)
	{
		const int32 Before = Errors.Num();
		TSet<FString> Allowed;
		for (const TCHAR* Key : Required)
		{
			Allowed.Add(Key);
			if (!Object.HasField(Key))
			{
				Errors.Add(Path + TEXT(".") + Key + TEXT(": missing"));
			}
		}
		for (const TCHAR* Key : Optional)
		{
			Allowed.Add(Key);
		}
		for (const TPair<FJsonObject::FStringType, TSharedPtr<FJsonValue>>& Pair : Object.Values)
		{
			if (!Allowed.Contains(FString(Pair.Key)))
			{
				Errors.Add(Path + TEXT(".") + Pair.Key + TEXT(": unknown field"));
			}
		}
		return Before == Errors.Num();
	}

	inline FObject ObjectAt(const FJsonObject& Parent, const FString& Key, const FString& Path, TArray<FString>& Errors)
	{
		const TSharedPtr<FJsonObject>* Object = nullptr;
		if (!Parent.TryGetObjectField(Key, Object))
		{
			Errors.Add(Path + TEXT(": expected object"));
			return nullptr;
		}
		return *Object;
	}

	inline bool Number(
	    const FJsonObject& Object, const FString& Key, const FString& Path, float& Out, TArray<FString>& Errors, bool bSigned = false)
	{
		const TSharedPtr<FJsonValue> Value = Object.TryGetField(Key);
		if (!Value || Value->Type != EJson::Number || !FMath::IsFinite(Value->AsNumber()) ||
		    FMath::Abs(Value->AsNumber()) > TNumericLimits<float>::Max() || (!bSigned && Value->AsNumber() < 0))
		{
			Errors.Add(Path + TEXT(".") + Key + TEXT(": expected finite number in range"));
			return false;
		}
		Out = static_cast<float>(Value->AsNumber());
		return true;
	}

	/** 네이티브 태그 이름의 배열을 읽습니다. 모르는 태그와 같은 태그를 두 번 쓴 것은 오류입니다. */
	inline bool Tags(const FJsonObject& Object, const FString& Key, const FString& Path, FGameplayTagContainer& Out, TArray<FString>& Errors)
	{
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (!Object.TryGetArrayField(Key, Values))
		{
			Errors.Add(Path + TEXT(".") + Key + TEXT(": expected tag array"));
			return false;
		}
		for (const TSharedPtr<FJsonValue>& Value : *Values)
		{
			const FGameplayTag Tag =
			    Value->Type == EJson::String ? FGameplayTag::RequestGameplayTag(FName(*Value->AsString()), false) : FGameplayTag();
			if (!Tag.IsValid() || Out.HasTagExact(Tag))
			{
				Errors.Add(Path + TEXT(".") + Key + TEXT(": unknown or duplicate native tag"));
			}
			else
			{
				Out.AddTag(Tag);
			}
		}
		return true;
	}

	/** 고정 Params의 리플렉션 필드와 JSON을 대조합니다. 선택·null 필드는 부르는 쪽에서 명시적으로 처리합니다. */
	template <typename T> bool Read(const FJsonObject& Object, T& Out, const FString& Path, TArray<FString>& Errors)
	{
		const int32 Before = Errors.Num();
		FJsonObject Normalized = Object;
		TSet<FString> Fields;
		for (TFieldIterator<FProperty> It(T::StaticStruct()); It; ++It)
		{
			const FProperty* Property = *It;
			const FString Key = Property->GetName();
			Fields.Add(Key);
			const TSharedPtr<FJsonValue> Value = Object.TryGetField(Key);
			const FString Field = Path + TEXT(".") + Key;
			if (!Value)
			{
				Errors.Add(Field + TEXT(": missing"));
				continue;
			}
			const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property);
			const FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property);
			const UEnum* Enum = EnumProperty ? EnumProperty->GetEnum() : NumericProperty ? NumericProperty->GetIntPropertyEnum() : nullptr;
			if (Enum)
			{
				if (Value->Type != EJson::String)
				{
					Errors.Add(Field + TEXT(": expected enum string"));
					continue;
				}
				FString Name = Value->AsString();
				if (Enum == StaticEnum<ECollisionChannel>())
				{
					Name = TEXT("ECC_") + Name;
				}
				if (Enum == StaticEnum<ECollisionResponse>())
				{
					Name = TEXT("ECR_") + Name;
				}
				const int64 Index = Enum->GetValueByNameString(Name);
				if (Index == INDEX_NONE || Name.EndsWith(TEXT("_MAX")))
				{
					Errors.Add(Field + TEXT(": invalid enum"));
				}
				else
				{
					Normalized.SetStringField(Key, Name);
				}
			}
			else if (NumericProperty)
			{
				float Ignored;
				Number(Object, Key, Path, Ignored, Errors, Key == TEXT("VelocityZ"));
			}
			else if (Property->IsA<FBoolProperty>())
			{
				if (Value->Type != EJson::Boolean)
				{
					Errors.Add(Field + TEXT(": expected boolean"));
				}
			}
			else
			{
				if (Value->Type != EJson::String || Value->AsString().IsEmpty())
				{
					Errors.Add(Field + TEXT(": expected nonempty string"));
					continue;
				}
				if (const FStructProperty* Struct = CastField<FStructProperty>(Property);
				    Struct && Struct->Struct == FGameplayTag::StaticStruct())
				{
					if (!FGameplayTag::RequestGameplayTag(FName(*Value->AsString()), false).IsValid())
					{
						Errors.Add(Field + TEXT(": unknown native tag"));
					}
				}
				if (const FObjectPropertyBase* Reference = CastField<FObjectPropertyBase>(Property))
				{
					UObject* Asset = StaticLoadObject(Reference->PropertyClass, nullptr, *Value->AsString(), nullptr, LOAD_NoWarn);
					if (!Asset || Asset->GetPathName() != Value->AsString())
					{
						Errors.Add(Field + TEXT(": missing asset or redirected path"));
					}
				}
			}
		}
		for (const TPair<FJsonObject::FStringType, TSharedPtr<FJsonValue>>& Pair : Object.Values)
		{
			if (!Fields.Contains(FString(Pair.Key)))
			{
				Errors.Add(Path + TEXT(".") + Pair.Key + TEXT(": unknown field"));
			}
		}
		if (Errors.Num() != Before)
		{
			return false;
		}
		FText Reason;
		if (!FJsonObjectConverter::JsonObjectToUStruct(MakeShared<FJsonObject>(Normalized), T::StaticStruct(), &Out, 0, 0, true, &Reason))
		{
			Errors.Add(Path + TEXT(": ") + Reason.ToString());
			return false;
		}
		return true;
	}
}
