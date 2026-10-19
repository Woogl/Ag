// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "JsonObjectConverter.h"
#include "UObject/UnrealType.h"

struct FAgJsonFieldRules
{
	TArray<FName> Optional;

	TArray<FName> Nullable;

	TArray<FName> Signed;
};

namespace AgJsonRead
{
	using FObject = TSharedPtr<FJsonObject>;

	inline bool Keys(const FJsonObject& Object, const FString& Path, std::initializer_list<const TCHAR*> Required, std::initializer_list<const TCHAR*> Optional, TArray<FString>& Errors)
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

	inline bool Number(const FJsonObject& Object, const FString& Key, const FString& Path, float& Out, TArray<FString>& Errors, bool bSigned = false)
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

	inline FAgJsonFieldRules NestedRules(const FAgJsonFieldRules& Rules, const FString& Field)
	{
		const FString Prefix = Field + TEXT(".");
		FAgJsonFieldRules Result;
		const TPair<const TArray<FName>*, TArray<FName>*> Lists[] = {
			{&Rules.Optional, &Result.Optional}, {&Rules.Nullable, &Result.Nullable}, {&Rules.Signed, &Result.Signed}};
		for (const TPair<const TArray<FName>*, TArray<FName>*>& List : Lists)
		{
			for (FName Name : *List.Key)
			{
				const FString Text = Name.ToString();
				if (Text.StartsWith(Prefix))
				{
					List.Value->Add(FName(*Text.RightChop(Prefix.Len())));
				}
			}
		}
		return Result;
	}

	inline bool ReadStruct(const FJsonObject& Object, const UScriptStruct& Struct, void* Out, const FString& Path, TArray<FString>& Errors, const FAgJsonFieldRules& Rules = FAgJsonFieldRules(), const TArray<FString>& Ignored = TArray<FString>())
	{
		const int32 Before = Errors.Num();
		FJsonObject Normalized = Object;
		TSet<FString> Fields;
		for (const FString& Key : Ignored)
		{
			Fields.Add(Key);
			Normalized.RemoveField(Key);
		}
		for (TFieldIterator<FProperty> It(&Struct); It; ++It)
		{
			const FProperty* Property = *It;
			const FName PropertyName = Property->GetFName();
			const FString Key = Property->GetName();
			Fields.Add(Key);
			const TSharedPtr<FJsonValue> Value = Object.TryGetField(Key);
			const FString Field = Path + TEXT(".") + Key;
			if (!Value || Value->Type == EJson::Null)
			{
				if (!(Value ? Rules.Nullable.Contains(PropertyName) : Rules.Optional.Contains(PropertyName)))
				{
					Errors.Add(Field + (Value ? TEXT(": can't be null") : TEXT(": missing")));
				}
				Normalized.RemoveField(Key);
				continue;
			}
			const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property);
			const FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property);
			const FStructProperty* StructProperty = CastField<FStructProperty>(Property);
			const UEnum* Enum = EnumProperty ? EnumProperty->GetEnum() : NumericProperty ? NumericProperty->GetIntPropertyEnum()
			                                                                             : nullptr;
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
				float Unused = 0.f;
				Number(Object, Key, Path, Unused, Errors, Rules.Signed.Contains(PropertyName));
			}
			else if (Property->IsA<FBoolProperty>())
			{
				if (Value->Type != EJson::Boolean)
				{
					Errors.Add(Field + TEXT(": expected boolean"));
				}
			}
			else if (StructProperty && StructProperty->Struct == FGameplayTagContainer::StaticStruct())
			{
				Tags(Object, Key, Path, *StructProperty->ContainerPtrToValuePtr<FGameplayTagContainer>(Out), Errors);
				Normalized.RemoveField(Key);
			}
			else if (StructProperty && StructProperty->Struct != FGameplayTag::StaticStruct())
			{
				const TSharedPtr<FJsonObject>* Nested = nullptr;
				if (!Value->TryGetObject(Nested))
				{
					Errors.Add(Field + TEXT(": expected object"));
					continue;
				}
				ReadStruct(**Nested, *StructProperty->Struct, StructProperty->ContainerPtrToValuePtr<void>(Out), Field, Errors,
					NestedRules(Rules, Key));
				Normalized.RemoveField(Key);
			}
			else
			{
				if (Value->Type != EJson::String || Value->AsString().IsEmpty())
				{
					Errors.Add(Field + TEXT(": expected nonempty string"));
					continue;
				}
				if (StructProperty && !FGameplayTag::RequestGameplayTag(FName(*Value->AsString()), false).IsValid())
				{
					Errors.Add(Field + TEXT(": unknown native tag"));
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
		// 선택 필드가 빠질 수 있으므로 엄격 모드를 쓰지 않습니다. 형식은 위에서 모두 확인했습니다.
		FText Reason;
		if (!FJsonObjectConverter::JsonObjectToUStruct(MakeShared<FJsonObject>(Normalized), &Struct, Out, 0, 0, false, &Reason))
		{
			Errors.Add(Path + TEXT(": ") + Reason.ToString());
			return false;
		}
		return true;
	}

	template <typename T>
	bool Read(const FJsonObject& Object, T& Out, const FString& Path, TArray<FString>& Errors)
	{
		return ReadStruct(Object, *T::StaticStruct(), &Out, Path, Errors);
	}
} // namespace AgJsonRead
