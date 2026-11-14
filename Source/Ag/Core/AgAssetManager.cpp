// Copyright Woogle. All Rights Reserved.

#include "Core/AgAssetManager.h"

#include "Core/AgJsonReader.h"
#include "Data/AgAbilityData.h"
#include "Data/AgCameraData.h"
#include "Data/AgCharacterData.h"
#include "Data/AgCombatRules.h"
#include "Data/AgEffectData.h"
#include "Data/AgSpawnActorData.h"
#include "Data/AgTargetingPresetData.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Internationalization/StringTableRegistry.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "TargetingSystem/TargetingPreset.h"
#include "UObject/GCObjectScopeGuard.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"

namespace AgAssetManagerPrivate
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
		{TEXT("Camera.json"), TEXT("Camera"), nullptr},
		{TEXT("SpawnActors.json"), TEXT("SpawnActors"), TEXT("SpawnActors")},
		{TEXT("TargetingPresets.json"), TEXT("TargetingPresets"), TEXT("Definitions")},
		{TEXT("Abilities.json"), TEXT("Abilities"), TEXT("Abilities")},
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
				It->CopyCompleteValue_InContainer(&To, &From);
			}
		}
	}
} // namespace AgAssetManagerPrivate

UAgAssetManager::UAgAssetManager()
{
	for (const AgAssetManagerPrivate::FAgJsonFile& File : AgAssetManagerPrivate::JsonFiles)
	{
		const FObjectProperty& Member = AgAssetManagerPrivate::GetMember(File);
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
	return FPaths::Combine(FPaths::ProjectDir(), AgAssetManagerPrivate::JsonFolderName);
}

const UAgCombatRules& UAgAssetManager::GetCombatRules() const
{
	return *CombatRules;
}

const UAgCameraData& UAgAssetManager::GetCameraData() const
{
	return *Camera;
}

const UTargetingPreset* UAgAssetManager::FindTargetingPreset(FName Id) const
{
	return TargetingPresets->Presets.FindRef(Id);
}

const FAgAbilityData* UAgAssetManager::FindAbility(FName Id) const
{
	return Abilities->Abilities.Find(Id);
}

AActor* UAgAssetManager::FindSpawnActor(FName Id) const
{
	return SpawnActors->SpawnActors.FindRef(Id);
}

const UGameplayEffect* UAgAssetManager::FindEffect(FName Id) const
{
	return Effects->Effects.FindRef(Id);
}

const UGameplayEffect* UAgAssetManager::GetEffect(EAgEffect Effect) const
{
	return NativeEffects.FindChecked(Effect).Get();
}

UAgCharacterData* UAgAssetManager::FindCharacterData(FName Id) const
{
	return Characters->Characters.FindRef(Id);
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
	for (const AgAssetManagerPrivate::FAgJsonFile& File : AgAssetManagerPrivate::JsonFiles)
	{
		Pending.Add(AgAssetManagerPrivate::GetMember(File).GetObjectPropertyValue_InContainer(this));
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
			const UPackage* Package = Reference->GetOutermost();
			if (Reference->IsIn(this))
			{
				Pending.Add(Reference);
			}
			else if (!FPackageName::IsScriptPackage(Package->GetName()) && Package != GetTransientPackage())
			{
				PackagesToCook.AddUnique(Package->GetFName());
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
	// 현지화가 필요한 문구의 표입니다. JSON의 글자 값이 이 표를 가리키므로 먼저 등록합니다. 엔진의 번역 수집이 소스에서 이 매크로를
	// 찾아 CSV를 읽으므로, 경로를 글자 그대로 적은 매크로로 등록합니다. 에디터에서는 엔진이 이 CSV의 변경을 감시해 다시 읽습니다.
	LOCTABLE_FROMFILE_GAME("StringTable", "StringTable", "StringTables/StringTable.csv");
	LoadJsonData(true);
#if WITH_EDITOR
	// 에셋 쪽에서 고친 오류는 JSON이 바뀌지 않아 파일 감시가 다시 읽지 않으므로, 게임 월드를 시작할 때 다시 읽습니다.
	FWorldDelegates::OnPostWorldInitialization.AddWeakLambda(this, [this](UWorld* World, const UWorld::InitializationValues)
	{
		if (bJsonLoadFailed && World->IsGameWorld())
		{
			ReloadJsonData();
		}
	});
	// 엔진은 블루프린트를 다시 컴파일할 때 템플릿(아키타입)은 새 클래스로 바꾸지 않으므로, 옛 클래스로 남은 생성 액터
	// 템플릿은 다시 읽어 새로 만듭니다.
	FCoreUObjectDelegates::OnObjectsReinstanced.AddWeakLambda(this, [this](const FCoreUObjectDelegates::FReplacementObjectMap&)
	{
		for (const TPair<FName, TObjectPtr<AActor>>& Pair : SpawnActors->SpawnActors)
		{
			if (Pair.Value && Pair.Value->GetClass()->HasAnyClassFlags(CLASS_NewerVersionExists))
			{
				ReloadJsonData();
				return;
			}
		}
	});
#endif
}

bool UAgAssetManager::LoadJsonData(bool bInitialLoad)
{
	TArray<UObject*> Loaded;
	for (const AgAssetManagerPrivate::FAgJsonFile& File : AgAssetManagerPrivate::JsonFiles)
	{
		Loaded.Add(NewObject<UObject>(this, AgAssetManagerPrivate::GetMember(File).PropertyClass));
	}
	// 읽는 동안 클래스·에셋 로딩이 GC를 돌릴 수 있는데, 새 객체는 아직 아무도 가리키지 않습니다.
	const TGCObjectsScopeGuard<UObject> LoadedGuard(Loaded);

	FAgJsonReader Reader(*this);
	bool bRead = true;
	for (int32 Index = 0; Index < Loaded.Num(); ++Index)
	{
		const AgAssetManagerPrivate::FAgJsonFile& File = AgAssetManagerPrivate::JsonFiles[Index];
		const FString Path = FString(AgAssetManagerPrivate::JsonFolderName) / File.FileName;
		bool bFileRead = false;
		if (File.Field)
		{
			const FProperty* Field = FindFProperty<FProperty>(Loaded[Index]->GetClass(), File.Field);
			checkf(Field, TEXT("%s has no property %s for %s."), *Loaded[Index]->GetClass()->GetName(), File.Field, File.FileName);
			bFileRead = Reader.ReadFile(Path, *Field, Field->ContainerPtrToValuePtr<void>(Loaded[Index]));
		}
		else
		{
			bFileRead = Reader.ReadFile(Path, *Loaded[Index]);
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

	UAgEffectSet& NewEffects = AgAssetManagerPrivate::FindLoaded<UAgEffectSet>(Loaded);
	for (const TPair<FName, FAgEffectDefinition>& Pair : NewEffects.Definitions)
	{
		NewEffects.Effects.Add(Pair.Key, AgEffectData::MakeEffect(Pair.Value, *this));
	}
	UAgTargetingPresetSet& NewTargetingPresets = AgAssetManagerPrivate::FindLoaded<UAgTargetingPresetSet>(Loaded);
	for (const TPair<FName, FTargetingTaskSet>& Pair : NewTargetingPresets.Definitions)
	{
		UTargetingPreset* Preset = NewObject<UTargetingPreset>(this);
		Preset->GetMutableTargetingTaskSet() = Pair.Value;
		NewTargetingPresets.Presets.Add(Pair.Key, Preset);
	}

	// 캐릭터는 자기 데이터 객체를 들고 있으므로, ID가 그대로인 항목은 기존 객체에 값만 복사합니다.
	for (TPair<FName, TObjectPtr<UAgCharacterData>>& Pair : AgAssetManagerPrivate::FindLoaded<UAgCharacterSet>(Loaded).Characters)
	{
		const TObjectPtr<UAgCharacterData>* Existing = Characters->Characters.Find(Pair.Key);
		if (Existing)
		{
			AgAssetManagerPrivate::CopyJsonFields(*Pair.Value, **Existing);
			Pair.Value = *Existing;
		}
	}

	for (int32 Index = 0; Index < Loaded.Num(); ++Index)
	{
		AgAssetManagerPrivate::CopyJsonFields(*Loaded[Index], *AgAssetManagerPrivate::GetMember(AgAssetManagerPrivate::JsonFiles[Index]).GetObjectPropertyValue_InContainer(this));
	}
	return bRead;
}
