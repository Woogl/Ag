// Copyright Woogle. All Rights Reserved.

#include "Data/AgCharacterData.h"

#include "AbilitySystem/AgGameplayAbility.h"
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

	if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(UAgCharacterData, ValuesFile))
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

const FAgAction* UAgCharacterData::FindAction(FName Id) const
{
	TArray<const FAgAction*> Actions;
	GetActions(Actions);
	const FAgAction* const* Found = Actions.FindByPredicate([Id](const FAgAction* Action) { return Action->Id == Id; });
	return Found ? *Found : nullptr;
}

void UAgCharacterData::ResolveActionParamsTypes(FJsonObject& Json, const TCHAR* Field, const TCHAR* AbilityMap, TArray<FString>& Errors) const
{
	// 형식이 틀린 항목은 건너뜁니다. 그 오류는 에셋 매니저의 검사가 알립니다.
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
		const TSubclassOf<UAgGameplayAbility> Ability = FindActionAbility(FName(*Id));
		const UScriptStruct* Struct = Ability ? Ability.GetDefaultObject()->GetActionParamsType() : nullptr;
		if (!Ability)
		{
			Errors.Add(FString::Printf(TEXT("%s[%d]: %s has no ability in %s's %s"), Field, Index, *Id, *GetName(), AbilityMap));
			(*Params)->Values.Empty();
		}
		else if (Struct && Struct->IsChildOf(FAgActionParams::StaticStruct()))
		{
			UAgAssetManager::SetJsonStructType(**Params, *Struct);
		}
		else if (Struct)
		{
			Errors.Add(FString::Printf(TEXT("%s[%d].Params: %s declares %s, which isn't an AgActionParams"), Field, Index, *Ability->GetName(), *Struct->GetName()));
			(*Params)->Values.Empty();
		}
		else if ((*Params)->Values.Num() > 0)
		{
			Errors.Add(FString::Printf(TEXT("%s[%d].Params: must be {}, %s reads no params"), Field, Index, *Ability->GetName()));
			(*Params)->Values.Empty();
		}
	}
}

void UAgPlayerData::GetActions(TArray<const FAgAction*>& OutActions) const
{
	for (const FAgAction& Action : Actions)
	{
		OutActions.Add(&Action);
	}
}

TSubclassOf<UAgGameplayAbility> UAgPlayerData::FindActionAbility(FName Id) const
{
	return ActionAbilities.FindRef(Id);
}

void UAgPlayerData::ResolveJsonStructTypes(FJsonObject& Json, TArray<FString>& Errors) const
{
	ResolveActionParamsTypes(Json, TEXT("Actions"), TEXT("ActionAbilities"), Errors);
}

#if WITH_EDITOR
void UAgPlayerData::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// 행동 전용 값의 구조체는 행동 어빌리티가 정하므로, 어빌리티를 바꾸면 JSON을 다시 읽습니다.
	if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(UAgPlayerData, ActionAbilities))
	{
		UAgAssetManager::Get().LoadJsonValues(*this, ValuesFile);
	}
}
#endif

void UAgBossData::GetActions(TArray<const FAgAction*>& OutActions) const
{
	for (const FAgBossPattern& Pattern : Patterns)
	{
		OutActions.Add(&Pattern);
	}
}

TSubclassOf<UAgGameplayAbility> UAgBossData::FindActionAbility(FName Id) const
{
	return PatternAbilities.FindRef(Id);
}

const FAgBossPattern* UAgBossData::FindPattern(FName Id) const
{
	return Patterns.FindByPredicate([Id](const FAgBossPattern& Entry) { return Entry.Id == Id; });
}

UAnimMontage* UAgBossData::FindPatternMontage(FName Id) const
{
	const TObjectPtr<UAnimMontage>* Montage = PatternMontages.Find(Id);
	return Montage ? Montage->Get() : nullptr;
}

void UAgBossData::ResolveJsonStructTypes(FJsonObject& Json, TArray<FString>& Errors) const
{
	ResolveActionParamsTypes(Json, TEXT("Patterns"), TEXT("PatternAbilities"), Errors);
}

#if WITH_EDITOR
void UAgBossData::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// 패턴 전용 값의 구조체는 패턴 어빌리티가 정하므로, 어빌리티를 바꾸면 JSON을 다시 읽습니다.
	if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(UAgBossData, PatternAbilities))
	{
		UAgAssetManager::Get().LoadJsonValues(*this, ValuesFile);
	}
}
#endif
