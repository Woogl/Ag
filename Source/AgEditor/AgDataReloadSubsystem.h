// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "AgDataReloadSubsystem.generated.h"

struct FFileChangeData;

/**
 * Data 폴더의 JSON이 저장되면 에셋 매니저가 그 파일을 다시 읽게 합니다. 컴파일 없이 PIE 중에 수치를 바꿔 볼 수
 * 있습니다(구현 계획서 '다시 읽기').
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
