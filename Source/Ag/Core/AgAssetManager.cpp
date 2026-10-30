// Copyright Woogle. All Rights Reserved.

#include "Core/AgAssetManager.h"

#include "Core/AgJsonReader.h"
#include "Data/AgAbilityData.h"
#include "Data/AgCharacterData.h"
#include "Data/AgCombatRules.h"
#include "Data/AgEffectData.h"
#include "Data/AgMontageTimeline.h"
#include "Data/AgProjectileData.h"
#include "Data/AgTargetingPresetData.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/Paths.h"
#include "TargetingSystem/TargetingPreset.h"
#include "UObject/GCObjectScopeGuard.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"

namespace
{
	/** JSON 파일은 Content 밖에 둡니다. Content에 두면 에디터가 에셋으로 가져오려 합니다. */
	const TCHAR* const JsonFolderName = TEXT("DesignData");

	struct FAgJsonFile
	{
		const TCHAR* FileName;
		const TCHAR* Member;
		/** 없으면 Member 객체 자체가 파일 전체를 받습니다. */
		const TCHAR* Field;
	};

	// 생성, 읽기, 다시 읽기, 쿡이 모두 이 목록을 돌므로 파일은 여기에만 등록합니다. 다른 파일이 ID로 가리키는 정의 파일을
	// 먼저 읽습니다.
	const FAgJsonFile JsonFiles[] = {
		{TEXT("Effects.json"), TEXT("Effects"), TEXT("Definitions")},
		{TEXT("CombatRules.json"), TEXT("CombatRules"), nullptr},
		{TEXT("Projectiles.json"), TEXT("Projectiles"), TEXT("Projectiles")},
		{TEXT("TargetingPresets.json"), TEXT("TargetingPresets"), TEXT("Definitions")},
		{TEXT("Abilities.json"), TEXT("Abilities"), TEXT("Abilities")},
		{TEXT("MontageTimelines.json"), TEXT("MontageTimelines"), TEXT("Timelines")},
		{TEXT("Characters.json"), TEXT("Characters"), TEXT("Characters")},
	};

	const FObjectProperty& GetMember(const FAgJsonFile& File)
	{
		const FObjectProperty* Member = FindFProperty<FObjectProperty>(UAgAssetManager::StaticClass(), File.Member);
		checkf(Member, TEXT("UAgAssetManager has no object property %s for %s."), File.Member, File.FileName);
		return *Member;
	}

	template <typename T>
	T& FindLoaded(const TArray<UObject*>& Loaded)
	{
		return *CastChecked<T>(*Loaded.FindByPredicate([](const UObject* Object)
		{ return Object->IsA<T>(); }));
	}

	/** 지금 객체를 가리키는 쪽이 그대로 새 값을 보도록 객체를 바꾸지 않고 값만 복사합니다. */
	void CopyJsonFields(const UObject& From, UObject& To)
	{
		for (TFieldIterator<FProperty> It(To.GetClass()); It; ++It)
		{
			if (FAgJsonReader::IsField(**It))
			{
				It->CopyCompleteValue(It->ContainerPtrToValuePtr<void>(&To), It->ContainerPtrToValuePtr<void>(&From));
			}
		}
	}
} // namespace

UAgAssetManager::UAgAssetManager()
{
	for (const FAgJsonFile& File : JsonFiles)
	{
		const FObjectProperty& Member = GetMember(File);
		UObject* Object = CreateDefaultSubobject(File.Member, Member.PropertyClass, Member.PropertyClass, true, false);
		Member.SetObjectPropertyValue_InContainer(this, Object);
	}
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
	const TObjectPtr<UAgCharacterData>* Found = Characters->Characters.Find(Id);
	return Found ? Found->Get() : nullptr;
}

TArray<FName> UAgAssetManager::GetCharacterDataIds() const
{
	TArray<FName> Ids;
	Characters->Characters.GetKeys(Ids);
	return Ids;
}

#if WITH_EDITOR
void UAgAssetManager::ReloadJsonData()
{
	if (!LoadJsonData(false))
	{
		return;
	}
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Green,
			TEXT("Data reloaded. Spawn settings apply on the next play start."));
	}
	OnJsonDataReloaded.Broadcast();
}

void UAgAssetManager::ModifyCook(TConstArrayView<const ITargetPlatform*> TargetPlatforms, TArray<FName>& PackagesToCook, TArray<FName>& PackagesToNeverCook)
{
	Super::ModifyCook(TargetPlatforms, PackagesToCook, PackagesToNeverCook);
	// 초기 로딩 이후 파일이 바뀌었더라도 쿡은 최신 JSON 전체를 검사합니다. 읽기 오류의 Error 로그가 쿡을 실패시킵니다.
	if (!LoadJsonData(false))
	{
		return;
	}

	// JSON에서 읽은 참조는 저장된 에셋 의존성이 아니므로 쿡에 직접 전달합니다. 읽으면서 만든 객체(캐릭터 데이터, GE, 대상
	// 프리셋과 그 Task)는 저장되지 않으므로, 그 객체의 클래스와 그 객체가 가리키는 것까지 따라갑니다.
	TArray<UObject*> Pending;
	for (const FAgJsonFile& File : JsonFiles)
	{
		Pending.Add(GetMember(File).GetObjectPropertyValue_InContainer(this));
	}
	TSet<UObject*> Visited;
	while (!Pending.IsEmpty())
	{
		UObject* Object = Pending.Pop();
		bool bVisited = false;
		Visited.Add(Object, &bVisited);
		if (bVisited)
		{
			continue;
		}
		TArray<UObject*> References = {Object->GetClass()};
		FReferenceFinder Finder(References, nullptr, false, true, false, false);
		Finder.AddPropertyReferencesWithStructARO(Object->GetClass(), Object);
		for (UObject* Reference : References)
		{
			const FString PackageName = Reference->GetOutermost()->GetName();
			if (Reference->IsIn(this))
			{
				Pending.Add(Reference);
			}
			else if (!PackageName.StartsWith(TEXT("/Script/")) && PackageName != TEXT("/Engine/Transient"))
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
	LoadJsonData(true);
#if WITH_EDITOR
	// 에셋 쪽에서 고친 오류는 JSON이 바뀌지 않아 파일 감시가 다시 읽지 않으므로, 게임 월드를 시작할 때 다시 읽습니다.
	FWorldDelegates::OnPostWorldInitialization.AddWeakLambda(this, [this](UWorld* World, const UWorld::InitializationValues)
	{
		if (bJsonLoadFailed && World->IsGameWorld())
		{
			LoadJsonData(false);
		}
	});
#endif
}

bool UAgAssetManager::LoadJsonData(bool bInitialLoad)
{
	TArray<UObject*> Loaded;
	for (const FAgJsonFile& File : JsonFiles)
	{
		Loaded.Add(NewObject<UObject>(this, GetMember(File).PropertyClass));
	}
	// 읽는 동안 클래스·에셋 로딩이 GC를 돌릴 수 있는데, 새 객체는 아직 아무도 가리키지 않습니다.
	const TGCObjectsScopeGuard<UObject> LoadedGuard(Loaded);

	FAgJsonReader Reader(*this);
	bool bRead = true;
	for (int32 Index = 0; Index < Loaded.Num(); ++Index)
	{
		const FAgJsonFile& File = JsonFiles[Index];
		const FString Path = FString(JsonFolderName) / File.FileName;
		bool bFileRead = false;
		if (File.Field)
		{
			const FProperty* Field = FindFProperty<FProperty>(Loaded[Index]->GetClass(), File.Field);
			checkf(Field, TEXT("%s has no property %s for %s."), *Loaded[Index]->GetClass()->GetName(), File.Field, File.FileName);
			bFileRead = Reader.ReadFile(Path, *Field, Field->ContainerPtrToValuePtr<void>(Loaded[Index]));
		}
		else
		{
			TObjectPtr<UObject> Object = Loaded[Index];
			bFileRead = Reader.ReadFile(Path, GetMember(File), &Object);
		}
		// 오류를 모두 알리도록 실패해도 다음 파일을 읽습니다.
		bRead = bFileRead && bRead;
	}
	bJsonLoadFailed = !bRead;
	// 처음에는 이전 데이터가 없으므로 오류가 있어도 읽은 값으로 시작합니다. 다시 읽기는 진행 중인 플레이의 데이터를 빼지 않게
	// 이전 데이터를 유지합니다.
	if (!bRead && !bInitialLoad)
	{
		return false;
	}

	UAgEffectSet& NewEffects = FindLoaded<UAgEffectSet>(Loaded);
	for (const TPair<FName, FAgEffectDefinition>& Pair : NewEffects.Definitions)
	{
		NewEffects.Effects.Add(Pair.Key, AgEffectData::MakeEffect(Pair.Value, *this));
	}
	UAgTargetingPresetSet& NewTargetingPresets = FindLoaded<UAgTargetingPresetSet>(Loaded);
	for (const TPair<FName, FTargetingTaskSet>& Pair : NewTargetingPresets.Definitions)
	{
		UTargetingPreset* Preset = NewObject<UTargetingPreset>(this);
		Preset->GetMutableTargetingTaskSet() = Pair.Value;
		NewTargetingPresets.Presets.Add(Pair.Key, Preset);
	}

	// 캐릭터는 자기 데이터 객체를 들고 있으므로, ID가 그대로인 항목은 기존 객체에 값만 복사합니다.
	for (TPair<FName, TObjectPtr<UAgCharacterData>>& Pair : FindLoaded<UAgCharacterSet>(Loaded).Characters)
	{
		const TObjectPtr<UAgCharacterData>* Existing = Characters->Characters.Find(Pair.Key);
		if (Existing)
		{
			CopyJsonFields(*Pair.Value, **Existing);
			Pair.Value = *Existing;
		}
	}

	for (int32 Index = 0; Index < Loaded.Num(); ++Index)
	{
		CopyJsonFields(*Loaded[Index], *GetMember(JsonFiles[Index]).GetObjectPropertyValue_InContainer(this));
	}
	return bRead;
}
