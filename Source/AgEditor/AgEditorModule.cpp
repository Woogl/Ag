// Copyright Woogle. All Rights Reserved.

#include "AgAssetToolset.h"
#include "AgEnemySightVisualizer.h"
#include "Character/AgEnemySightPreviewComponent.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#include "Modules/ModuleManager.h"
#include "ToolsetRegistry/UToolsetRegistry.h"

class FAgEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		UToolsetRegistry::RegisterToolsetClass(UAgAssetToolset::StaticClass());
		if (GUnrealEd)
		{
			const TSharedPtr<FAgEnemySightVisualizer> Visualizer = MakeShared<FAgEnemySightVisualizer>();
			GUnrealEd->RegisterComponentVisualizer(UAgEnemySightPreviewComponent::StaticClass()->GetFName(), Visualizer);
			Visualizer->OnRegister();
		}
	}

	virtual void ShutdownModule() override
	{
		if (GUnrealEd)
		{
			GUnrealEd->UnregisterComponentVisualizer(UAgEnemySightPreviewComponent::StaticClass()->GetFName());
		}
		if (UObjectInitialized())
		{
			UToolsetRegistry::UnregisterToolsetClass(UAgAssetToolset::StaticClass());
		}
	}
};

IMPLEMENT_MODULE(FAgEditorModule, AgEditor);
