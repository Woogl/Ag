// Copyright Woogle. All Rights Reserved.

#include "Data/AgCharacterData.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "Ag.h"
#include "Animation/AnimMontage.h"
#include "Core/AgAssetManager.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

void UAgCharacterData::PostLoad()
{
	Super::PostLoad();

	// 기획 값은 로드할 때마다 JSON에서 읽고, 에셋에는 저장하지 않습니다.
	UAgAssetManager::Get().LoadJsonValues(*this, ValuesFile);
}

#if WITH_EDITOR
void UAgCharacterData::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// 전용 값의 구조체는 어빌리티 클래스가 정하므로, 어빌리티 클래스를 바꿔도 JSON을 다시 읽습니다.
	const FName Property = PropertyChangedEvent.GetMemberPropertyName();
	if (Property == GET_MEMBER_NAME_CHECKED(UAgCharacterData, ValuesFile) || Property == GET_MEMBER_NAME_CHECKED(UAgCharacterData, AbilityClasses))
	{
		UAgAssetManager::Get().LoadJsonValues(*this, ValuesFile);
	}
}

void UAgCharacterData::PostEditUndo()
{
	Super::PostEditUndo();

	// 실행 취소·다시 실행은 Transient 속성(파일에서 읽은 값)까지 그 편집 때의 값으로 되돌립니다. 기준은 파일이므로 다시 읽습니다.
	UAgAssetManager::Get().LoadJsonValues(*this, ValuesFile);
}
#endif

const FAgAbilityData* UAgCharacterData::FindAbility(FName Id) const
{
	return Abilities.FindByPredicate([Id](const FAgAbilityData& Ability) { return Ability.Id == Id; });
}

TSubclassOf<UAgGameplayAbility> UAgCharacterData::FindAbilityClass(FName Id) const
{
	return AbilityClasses.FindRef(Id);
}

UAnimMontage* UAgCharacterData::FindMontage(FName Id) const
{
	// 몽타주 ID는 코드에 적은 이름이나 어빌리티 ID라 컴파일러가 확인하지 못하므로, 없으면 바로 알립니다.
	UAnimMontage* Montage = Montages.FindRef(Id);
	if (!Montage)
	{
		UE_LOG(LogAg, Error, TEXT("%s has no montage %s in Montages."), *GetName(), *Id.ToString());
	}
	return Montage;
}

void UAgCharacterData::ResolveJsonStructTypes(FJsonObject& Json, TArray<FString>& Errors) const
{
	// 형식이 틀린 항목은 건너뜁니다. 그 오류는 에셋 매니저의 검사가 알립니다.
	const TCHAR* Field = TEXT("Abilities");
	const TArray<TSharedPtr<FJsonValue>>* Entries = nullptr;
	if (!Json.TryGetArrayField(Field, Entries))
	{
		return;
	}
	for (int32 Index = 0; Index < Entries->Num(); ++Index)
	{
		const TSharedPtr<FJsonObject>* Entry = nullptr;
		const TSharedPtr<FJsonObject>* Params = nullptr;
		FString Id;
		if (!(*Entries)[Index]->TryGetObject(Entry) || !(*Entry)->TryGetStringField(TEXT("Id"), Id) || !(*Entry)->TryGetObjectField(TEXT("Params"), Params))
		{
			continue;
		}

		// 구조체를 정하지 못한 Params는 비워, 검사가 같은 문제를 다시 알리지 않게 합니다. 오류가 있으면 어차피 값을 쓰지 않습니다.
		const TSubclassOf<UAgGameplayAbility> Ability = FindAbilityClass(FName(*Id));
		const UScriptStruct* Struct = Ability ? Ability.GetDefaultObject()->GetAbilityParamsType() : nullptr;
		if (!Ability)
		{
			Errors.Add(FString::Printf(TEXT("%s[%d]: %s has no class in %s's AbilityClasses"), Field, Index, *Id, *GetName()));
			(*Params)->Values.Empty();
		}
		else if (Struct && Struct->IsChildOf(FAgAbilityParams::StaticStruct()))
		{
			UAgAssetManager::SetJsonStructType(**Params, *Struct);
		}
		else if (Struct)
		{
			Errors.Add(FString::Printf(TEXT("%s[%d].Params: %s declares %s, which isn't an AgAbilityParams"), Field, Index, *Ability->GetName(), *Struct->GetName()));
			(*Params)->Values.Empty();
		}
		else if ((*Params)->Values.Num() > 0)
		{
			Errors.Add(FString::Printf(TEXT("%s[%d].Params: must be {}, %s reads no params"), Field, Index, *Ability->GetName()));
			(*Params)->Values.Empty();
		}
	}
}
