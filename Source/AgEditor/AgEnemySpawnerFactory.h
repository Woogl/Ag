// Copyright Woogle. All Rights Reserved.

#pragma once

#include "ActorFactories/ActorFactory.h"
#include "CoreMinimal.h"
#include "AgEnemySpawnerFactory.generated.h"

/**
 * 적 블루프린트를 레벨에 끌어다 놓으면 적 대신 그 적을 생성하는 스포너(AAgEnemySpawner)를 놓습니다. 엔진은 에셋을 받는 첫
 * 팩토리를 쓰므로, 모든 블루프린트를 받는 엔진 팩토리보다 앞에 등록합니다(AgEditorModule).
 */
UCLASS()
class UAgEnemySpawnerFactory : public UActorFactory
{
	GENERATED_BODY()

public:
	UAgEnemySpawnerFactory();

	virtual bool CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) override;

protected:
	virtual void PostSpawnActor(UObject* Asset, AActor* NewActor) override;
};
