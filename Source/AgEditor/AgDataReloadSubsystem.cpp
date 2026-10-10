// Copyright Woogle. All Rights Reserved.

#include "AgDataReloadSubsystem.h"

#include "CoreGlobals.h"
#include "Data/AgDataFile.h"
#include "DirectoryWatcherModule.h"
#include "IDirectoryWatcher.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"

bool UAgDataReloadSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// Commandlets (the cook) never change the files while running.
	return !IsRunningCommandlet();
}

void UAgDataReloadSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	WatchedFolder = FPaths::ConvertRelativePathToFull(AgDataFile::GetFolder());
	FDirectoryWatcherModule& DirectoryWatcher = FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));
	if (IDirectoryWatcher* Watcher = DirectoryWatcher.Get())
	{
		Watcher->RegisterDirectoryChangedCallback_Handle(WatchedFolder, IDirectoryWatcher::FDirectoryChanged::CreateUObject(this, &ThisClass::HandleFilesChanged),
			WatchHandle, IDirectoryWatcher::WatchOptions::IgnoreChangesInSubtree);
	}
}

void UAgDataReloadSubsystem::Deinitialize()
{
	if (FDirectoryWatcherModule* DirectoryWatcher = FModuleManager::GetModulePtr<FDirectoryWatcherModule>(TEXT("DirectoryWatcher")))
	{
		if (IDirectoryWatcher* Watcher = DirectoryWatcher->Get())
		{
			Watcher->UnregisterDirectoryChangedCallback_Handle(WatchedFolder, WatchHandle);
		}
	}

	Super::Deinitialize();
}

void UAgDataReloadSubsystem::HandleFilesChanged(const TArray<FFileChangeData>& Changes)
{
	TArray<FString> FileNames;
	for (const FFileChangeData& Change : Changes)
	{
		if (Change.Action != FFileChangeData::FCA_Removed && FPaths::GetExtension(Change.Filename).Equals(TEXT("json"), ESearchCase::IgnoreCase))
		{
			FileNames.AddUnique(FPaths::GetCleanFilename(Change.Filename));
		}
	}
	if (!FileNames.IsEmpty())
	{
		AgDataFile::ReloadFiles(FileNames);
	}
}
