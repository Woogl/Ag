// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "JsonObjectConverter.h"
#include "UObject/UnrealType.h"

/**
 * 구조체 필드의 JSON 규칙. 적지 않은 필드는 필수이고 null이 아니며, 수는 0 이상입니다. 구조체 필드 안쪽의 필드는
 * '필드.안쪽 필드' 이름으로 적습니다(예: Next.BlendTime).
 */
struct FAgJsonFieldRules
{
	/** 없어도 되는 필드. 없으면 C++ 초기값입니다. 적었으면 값이 있어야 하므로, 적었는지로 쓰는지를 정할 수 있습니다. */
	TArray<FName> Optional;

	/** 키는 있어야 하고 null을 쓸 수 있는 필드. null이면 C++ 초기값입니다. */
	TArray<FName> Nullable;

	/** 음수를 쓸 수 있는 수 필드 */
	TArray<FName> Signed;
};

/** 어빌리티, 효과, 몽타주 시간표가 함께 쓰는 고정 스키마 JSON 읽기. 오류는 필드 경로와 함께 Errors에 더합니다. */
namespace AgJsonRead
{
	using FObject = TSharedPtr<FJsonObject>;

	/** Required 키가 모두 있고 Required·Optional 밖의 키가 없는지 확인합니다. */
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

	/** Rules 가운데 Field의 안쪽 필드에 대한 규칙 */
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

	/**
	 * Struct의 리플렉션 필드와 JSON을 대조해 Out에 읽습니다. 필드마다의 규칙은 Rules를 따르고, Ignored는 부르는 쪽이 읽는
	 * 키입니다. 구조체 필드는 객체로, 태그 모음은 태그 이름의 배열로 적습니다. 실패하면 Out의 일부만 바뀌었을 수 있습니다.
	 */
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
				// 태그 모음은 그 자리에서 읽고 바깥 구조체의 변환에서는 뺍니다.
				Tags(Object, Key, Path, *StructProperty->ContainerPtrToValuePtr<FGameplayTagContainer>(Out), Errors);
				Normalized.RemoveField(Key);
			}
			else if (StructProperty && StructProperty->Struct != FGameplayTag::StaticStruct())
			{
				// 구조체 필드는 그 자리에서 읽고 바깥 구조체의 변환에서는 뺍니다.
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

	/** 구조체 T의 리플렉션 필드를 모두 필수로 읽습니다(비용, 트리거). */
	template <typename T>
	bool Read(const FJsonObject& Object, T& Out, const FString& Path, TArray<FString>& Errors)
	{
		return ReadStruct(Object, *T::StaticStruct(), &Out, Path, Errors);
	}
} // namespace AgJsonRead
