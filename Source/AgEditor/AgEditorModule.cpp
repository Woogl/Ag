// Copyright Woogle. All Rights Reserved.

#include "AgAnimationToolset.h"
#include "AgBlueprintToolset.h"
#include "AgEnemySightVisualizer.h"
#include "AgStateTreeToolset.h"
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
		UToolsetRegistry::RegisterToolsetClass(UAgAnimationToolset::StaticClass());
		UToolsetRegistry::RegisterToolsetClass(UAgBlueprintToolset::StaticClass());
		UToolsetRegistry::RegisterToolsetClass(UAgStateTreeToolset::StaticClass());
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
			UToolsetRegistry::UnregisterToolsetClass(UAgAnimationToolset::StaticClass());
			UToolsetRegistry::UnregisterToolsetClass(UAgBlueprintToolset::StaticClass());
			UToolsetRegistry::UnregisterToolsetClass(UAgStateTreeToolset::StaticClass());
		}
	}
};

IMPLEMENT_MODULE(FAgEditorModule, AgEditor);
