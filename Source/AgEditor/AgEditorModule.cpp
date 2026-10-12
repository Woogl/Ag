// Copyright Woogle. All Rights Reserved.

#include "AgAssetToolset.h"
#include "Modules/ModuleManager.h"
#include "ToolsetRegistry/UToolsetRegistry.h"

/** 에디터 전용 모듈. 프로젝트 MCP 도구(UAgAssetToolset)를 에디터 MCP 도구에 등록합니다. */
class FAgEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		UToolsetRegistry::RegisterToolsetClass(UAgAssetToolset::StaticClass());
	}

	virtual void ShutdownModule() override
	{
		if (UObjectInitialized())
		{
			UToolsetRegistry::UnregisterToolsetClass(UAgAssetToolset::StaticClass());
		}
	}
};

IMPLEMENT_MODULE(FAgEditorModule, AgEditor);
