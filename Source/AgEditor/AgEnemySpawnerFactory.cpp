// Copyright Woogle. All Rights Reserved.

#include "AgEnemySpawnerFactory.h"

#include "AssetRegistry/AssetData.h"
#include "Character/AgEnemyCharacter.h"
#include "Engine/Blueprint.h"
#include "Game/AgEnemySpawner.h"
#include "Misc/PackageName.h"

UAgEnemySpawnerFactory::UAgEnemySpawnerFactory()
{
	DisplayName = FText::FromString(TEXT("Enemy Spawner"));
	NewActorClass = AAgEnemySpawner::StaticClass();
}

bool UAgEnemySpawnerFactory::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
	// 불러오지 않은 블루프린트도 에셋 레지스트리의 태그로 부모를 봅니다. 적 블루프린트가 아니면 받지 않아 엔진의 블루프린트
	// 팩토리로 넘어갑니다.
	if (!AssetData.IsValid() || !AssetData.IsInstanceOf(UBlueprint::StaticClass()))
	{
		return false;
	}
	const FString NativeParentPath = AssetData.GetTagValueRef<FString>(FBlueprintTags::NativeParentClassPath);
	const UClass* NativeParent = FindObject<UClass>(nullptr, *FPackageName::ExportTextPathToObjectPath(NativeParentPath));
	return NativeParent && NativeParent->IsChildOf(AAgEnemyCharacter::StaticClass());
}

void UAgEnemySpawnerFactory::PostSpawnActor(UObject* Asset, AActor* NewActor)
{
	Super::PostSpawnActor(Asset, NewActor);

	// 스포너의 위치는 적의 발밑이라, 엔진이 준 놓을 면 위의 점을 그대로 씁니다.
	const UBlueprint* Blueprint = Cast<UBlueprint>(Asset);
	if (Blueprint && Blueprint->GeneratedClass && Blueprint->GeneratedClass->IsChildOf(AAgEnemyCharacter::StaticClass()))
	{
		CastChecked<AAgEnemySpawner>(NewActor)->SetEnemyClass(Blueprint->GeneratedClass.Get());
	}
}
