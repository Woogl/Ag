// Copyright Woogle. All Rights Reserved.

#include "ActorFactories/ActorFactoryBlueprint.h"
#include "AgAnimationToolset.h"
#include "AgBlueprintToolset.h"
#include "AgEnemySightVisualizer.h"
#include "AgEnemySpawnerFactory.h"
#include "AgLevelToolset.h"
#include "AgPatrolVisualizer.h"
#include "AgStateTreeToolset.h"
#include "AI/AgPatrolPreviewComponent.h"
#include "Character/AgEnemySightPreviewComponent.h"
#include "Editor.h"
#include "Editor/UnrealEdEngine.h"
#include "Modules/ModuleManager.h"
#include "Subsystems/PlacementSubsystem.h"
#include "ToolsetRegistry/UToolsetRegistry.h"
#include "UnrealEdGlobals.h"

namespace AgEditorModulePrivate
{
	void RegisterEnemySpawnerFactory()
	{
		UAgEnemySpawnerFactory* Factory = NewObject<UAgEnemySpawnerFactory>(GetTransientPackage());
		// 엔진은 에셋을 받는 첫 팩토리를 씁니다. 엔진의 블루프린트 팩토리는 모든 액터 블루프린트를 받으므로 그 앞에 둡니다.
		GEditor->ActorFactories.Insert(Factory, 0);
		// 끌어다 놓기는 배치 서브시스템의 목록에서 고르는데, 이 목록은 등록한 순서뿐이라 블루프린트 팩토리를 뺐다가 뒤에 다시
		// 넣습니다.
		if (UPlacementSubsystem* Placement = GEditor->GetEditorSubsystem<UPlacementSubsystem>())
		{
			TArray<TScriptInterface<IAssetFactoryInterface>> BlueprintFactories;
			for (TScriptInterface<IAssetFactoryInterface> BlueprintFactory = Placement->GetAssetFactoryFromFactoryClass(UActorFactoryBlueprint::StaticClass()); BlueprintFactory;
				BlueprintFactory = Placement->GetAssetFactoryFromFactoryClass(UActorFactoryBlueprint::StaticClass()))
			{
				Placement->UnregisterAssetFactory(BlueprintFactory);
				BlueprintFactories.Add(BlueprintFactory);
			}
			Placement->RegisterAssetFactory(Factory);
			for (const TScriptInterface<IAssetFactoryInterface>& BlueprintFactory : BlueprintFactories)
			{
				Placement->RegisterAssetFactory(BlueprintFactory);
			}
		}
	}

	void UnregisterEnemySpawnerFactory()
	{
		GEditor->ActorFactories.RemoveAll([](const UActorFactory* Factory)
		{ return Factory && Factory->IsA<UAgEnemySpawnerFactory>(); });
		if (UPlacementSubsystem* Placement = GEditor->GetEditorSubsystem<UPlacementSubsystem>())
		{
			Placement->UnregisterAssetFactory(Placement->GetAssetFactoryFromFactoryClass(UAgEnemySpawnerFactory::StaticClass()));
		}
	}
} // namespace AgEditorModulePrivate

class FAgEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		UToolsetRegistry::RegisterToolsetClass(UAgAnimationToolset::StaticClass());
		UToolsetRegistry::RegisterToolsetClass(UAgBlueprintToolset::StaticClass());
		UToolsetRegistry::RegisterToolsetClass(UAgLevelToolset::StaticClass());
		UToolsetRegistry::RegisterToolsetClass(UAgStateTreeToolset::StaticClass());
		if (GUnrealEd)
		{
			const TSharedPtr<FAgEnemySightVisualizer> SightVisualizer = MakeShared<FAgEnemySightVisualizer>();
			GUnrealEd->RegisterComponentVisualizer(UAgEnemySightPreviewComponent::StaticClass()->GetFName(), SightVisualizer);
			SightVisualizer->OnRegister();
			const TSharedPtr<FAgPatrolVisualizer> PatrolVisualizer = MakeShared<FAgPatrolVisualizer>();
			GUnrealEd->RegisterComponentVisualizer(UAgPatrolPreviewComponent::StaticClass()->GetFName(), PatrolVisualizer);
			PatrolVisualizer->OnRegister();
			// 이 모듈은 엔진 초기화 뒤에 올라와, 에디터와 배치 서브시스템이 팩토리 목록을 이미 만든 뒤입니다.
			AgEditorModulePrivate::RegisterEnemySpawnerFactory();
		}
	}

	virtual void ShutdownModule() override
	{
		if (GUnrealEd)
		{
			GUnrealEd->UnregisterComponentVisualizer(UAgEnemySightPreviewComponent::StaticClass()->GetFName());
			GUnrealEd->UnregisterComponentVisualizer(UAgPatrolPreviewComponent::StaticClass()->GetFName());
			if (UObjectInitialized())
			{
				AgEditorModulePrivate::UnregisterEnemySpawnerFactory();
			}
		}
		if (UObjectInitialized())
		{
			UToolsetRegistry::UnregisterToolsetClass(UAgAnimationToolset::StaticClass());
			UToolsetRegistry::UnregisterToolsetClass(UAgBlueprintToolset::StaticClass());
			UToolsetRegistry::UnregisterToolsetClass(UAgLevelToolset::StaticClass());
			UToolsetRegistry::UnregisterToolsetClass(UAgStateTreeToolset::StaticClass());
		}
	}
};

IMPLEMENT_MODULE(FAgEditorModule, AgEditor);
