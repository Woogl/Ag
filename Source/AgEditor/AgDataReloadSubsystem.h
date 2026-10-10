// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "AgDataReloadSubsystem.generated.h"

struct FFileChangeData;

/**
 * Reads a data file again when it is saved in the project's Data folder, so changed values show without compiling or
 * restarting PIE (구현 계획서 '데이터' 다시 읽기).
 */
UCLASS()
class UAgDataReloadSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	void HandleFilesChanged(const TArray<FFileChangeData>& Changes);

	FString WatchedFolder;
	FDelegateHandle WatchHandle;
};
