// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "AgDataReloadSubsystem.generated.h"

struct FFileChangeData;

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
