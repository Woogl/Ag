// Copyright Woogle. All Rights Reserved.

#include "AgAssetToolset.h"
#include "Modules/ModuleManager.h"
#include "ToolsetRegistry/UToolsetRegistry.h"

/** Editor-only module: adds the project's tools to the editor's MCP tools. */
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
