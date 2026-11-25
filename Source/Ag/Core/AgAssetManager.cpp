// Copyright Woogle. All Rights Reserved.

#include "Core/AgAssetManager.h"

#include "Core/AgJsonData.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Internationalization/StringTableRegistry.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/GCObjectScopeGuard.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UObjectHash.h"
#include "UObject/UnrealType.h"

namespace AgAssetManagerPrivate
{
	/** JSON 파일은 Content 밖에 둡니다. Content에 두면 에디터가 에셋으로 가져오려 합니다. */
	const TCHAR* const JsonFolderName = TEXT("DesignData");

	/** JSON으로 채우는 데이터 클래스를 모두 찾습니다. 클래스를 만들면 따로 등록하지 않아도 읽기, 다시 읽기, 쿡에 들어갑니다. */
	TArray<UClass*> GetJsonDataClasses()
	{
		TArray<UClass*> Classes;
		GetDerivedClasses(UAgJsonData::StaticClass(), Classes);
		Classes.RemoveAll([](const UClass* Class)
		{ return Class->HasAnyClassFlags(CLASS_Abstract | CLASS_NewerVersionExists); });
		// 오류가 늘 같은 순서로 나오게 합니다.
		Classes.Sort([](const UClass& A, const UClass& B)
		{ return A.GetName() < B.GetName(); });
		return Classes;
	}
} // namespace AgAssetManagerPrivate

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

FString UAgAssetManager::GetJsonPath(const TCHAR* FileName)
{
	return FString(AgAssetManagerPrivate::JsonFolderName) / FileName;
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

	// JSON에서 읽은 참조는 저장된 에셋 의존성이 아니므로 쿡에 직접 전달합니다. 쓰는 쪽이 붙잡는 데이터는 지금 들고 있지 않으므로
	// 여기서 모두 읽어 함께 따라갑니다.
	TArray<UObject*> Pending;
	for (UAgJsonData* Data : PersistentData)
	{
		Pending.Add(Data);
	}
	for (UClass* Class : AgAssetManagerPrivate::GetJsonDataClasses())
	{
		if (!Class->GetDefaultObject<UAgJsonData>()->IsAlwaysLoaded())
		{
			Pending.Add(NewObject<UAgJsonData>(this, Class));
		}
	}
	const TGCObjectsScopeGuard<UObject> PendingGuard(Pending);
	FAgJsonReader Reader(*this, JsonDocuments);
	for (UObject* Object : Pending)
	{
		UAgJsonData* Data = CastChecked<UAgJsonData>(Object);
		if (!Data->IsAlwaysLoaded())
		{
			Data->ReadJson(Reader, NAME_None);
		}
	}

	// 읽으면서 만든 객체(캐릭터 데이터, GE, 대상 프리셋과 그 Task)는 저장되지 않으므로, 그 객체의 클래스와 그 객체가 가리키는
	// 것까지 따라갑니다.
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
		for (TPropertyValueIterator<FSoftObjectProperty> It(Object->GetClass(), Object); It; ++It)
		{
			const FSoftObjectPtr& Soft = *static_cast<const FSoftObjectPtr*>(It.Value());
			if (!Soft.IsNull())
			{
				PackagesToCook.AddUnique(Soft.ToSoftObjectPath().GetLongPackageFName());
			}
		}
	}
}
#endif

void UAgAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();

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
#endif
}

UAgJsonData* UAgAssetManager::FindData(const UClass& Class) const
{
	const TObjectPtr<UAgJsonData>* Data = PersistentData.FindByPredicate([&Class](const UAgJsonData* Candidate)
	{ return Candidate->GetClass() == &Class; });
	return Data ? Data->Get() : nullptr;
}

UAgJsonData* UAgAssetManager::LoadData(const UClass& Class, FName Key)
{
	const TPair<const UClass*, FName> CacheKey(&Class, Key);
	if (UAgJsonData* Cached = LoadedData.FindRef(CacheKey).Get())
	{
		return Cached;
	}
	UAgJsonData* Loaded = NewObject<UAgJsonData>(this, &Class);
	// 읽는 동안 에셋 로딩이 GC를 돌릴 수 있는데, 새 객체는 아직 아무도 가리키지 않습니다.
	const TGCObjectScopeGuard<UAgJsonData> LoadedGuard(Loaded);
	FAgJsonReader Reader(*this, JsonDocuments);
	Loaded->ReadJson(Reader, Key);
	LoadedData.Add(CacheKey, Loaded);
	return Loaded;
}

bool UAgAssetManager::LoadJsonData(bool bInitialLoad)
{
	// 모든 파일을 먼저 파싱해 두므로 정의 ID 검사는 읽는 순서와 관계없습니다. 오류를 모두 알리도록 실패해도 다음으로 갑니다.
	FAgJsonDocuments NewDocuments;
	FAgJsonReader Reader(*this, NewDocuments);
	TArray<FString> FileNames;
	IFileManager::Get().FindFiles(FileNames, *FPaths::Combine(GetJsonFolder(), TEXT("*.json")), true, false);
	FileNames.Sort();
	bool bRead = true;
	for (const FString& FileName : FileNames)
	{
		bRead = Reader.ParseFile(GetJsonPath(*FileName)) && bRead;
	}

	// 새 객체에 먼저 읽어, 오류가 있으면 지금 데이터를 건드리지 않고 버릴 수 있게 합니다. 쓰는 쪽이 붙잡는 데이터도 오류를 지금
	// 알리도록 한 번 모두 읽어 봅니다. 그때 읽힌 에셋은 붙잡는 것이 없어 다음 가비지 컬렉션 때 내려갑니다.
	TArray<UAgJsonData*> Loaded;
	for (UClass* Class : AgAssetManagerPrivate::GetJsonDataClasses())
	{
		Loaded.Add(NewObject<UAgJsonData>(this, Class));
	}
	// 읽는 동안 클래스·에셋 로딩이 GC를 돌릴 수 있는데, 새 객체는 아직 아무도 가리키지 않습니다.
	const TGCObjectsScopeGuard<UAgJsonData> LoadedGuard(Loaded);
	for (UAgJsonData* Data : Loaded)
	{
		bRead = Data->ReadJson(Reader, NAME_None) && bRead;
	}
	bJsonLoadFailed = !bRead;
	// 처음에는 이전 데이터가 없으므로 오류가 있어도 읽은 값으로 시작합니다. 다시 읽기는 진행 중인 플레이의 데이터를 빼지 않게
	// 이전 데이터를 유지합니다.
	if (!bRead && !bInitialLoad)
	{
		return false;
	}

	JsonDocuments = MoveTemp(NewDocuments);
	for (UAgJsonData* Data : Loaded)
	{
		if (!Data->IsAlwaysLoaded())
		{
			continue;
		}
		UAgJsonData* Persistent = FindData(*Data->GetClass());
		if (!Persistent)
		{
			// 에디터 파이썬이 이름으로 찾을 수 있게 클래스 이름으로 둡니다.
			Persistent = NewObject<UAgJsonData>(this, Data->GetClass(), Data->GetClass()->GetFName());
			PersistentData.Add(Persistent);
		}
		Persistent->CopyJson(*Data);
	}

	// 쓰는 쪽이 붙잡고 있는 데이터는 그 객체 그대로 새 값을 받습니다. 늘 들고 있는 데이터를 위에서 먼저 바꿨으므로, 그것을 보고
	// 읽을 항목을 정하는 데이터도 새 값을 봅니다.
	FAgJsonReader LoadedReader(*this, JsonDocuments);
	for (TMap<TPair<const UClass*, FName>, TWeakObjectPtr<UAgJsonData>>::TIterator It = LoadedData.CreateIterator(); It; ++It)
	{
		UAgJsonData* Data = It.Value().Get();
		if (!Data)
		{
			It.RemoveCurrent();
			continue;
		}
		UAgJsonData* Fresh = NewObject<UAgJsonData>(this, Data->GetClass());
		const TGCObjectScopeGuard<UAgJsonData> FreshGuard(Fresh);
		// 읽을 항목이 없어진 데이터는 옛 값을 그대로 둡니다.
		if (Fresh->ReadJson(LoadedReader, It.Key().Value))
		{
			Data->CopyJson(*Fresh);
		}
	}
	return bRead;
}
