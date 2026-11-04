// Copyright Woogle. All Rights Reserved.

#include "AgAnimationToolset.h"
#include "AgBlueprintToolset.h"
#include "AgEnemySightVisualizer.h"
#include "AgPatrolVisualizer.h"
#include "AgStateTreeToolset.h"
#include "AI/AgPatrolPreviewComponent.h"
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
			const TSharedPtr<FAgEnemySightVisualizer> SightVisualizer = MakeShared<FAgEnemySightVisualizer>();
			GUnrealEd->RegisterComponentVisualizer(UAgEnemySightPreviewComponent::StaticClass()->GetFName(), SightVisualizer);
			SightVisualizer->OnRegister();
			const TSharedPtr<FAgPatrolVisualizer> PatrolVisualizer = MakeShared<FAgPatrolVisualizer>();
			GUnrealEd->RegisterComponentVisualizer(UAgPatrolPreviewComponent::StaticClass()->GetFName(), PatrolVisualizer);
			PatrolVisualizer->OnRegister();
		}
	}

	virtual void ShutdownModule() override
	{
		if (GUnrealEd)
		{
			GUnrealEd->UnregisterComponentVisualizer(UAgEnemySightPreviewComponent::StaticClass()->GetFName());
			GUnrealEd->UnregisterComponentVisualizer(UAgPatrolPreviewComponent::StaticClass()->GetFName());
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
