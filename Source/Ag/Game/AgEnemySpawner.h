// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AgEnemySpawner.generated.h"

class AAgEnemyCharacter;
class AAgPatrolRoute;
class FTokenizedMessage;
class UAgAbilitySet;
class UAgEnemySightPreviewComponent;
class UAgPatrolPreviewComponent;
class UMeshComponent;
struct FStreamableHandle;

/**
 * 레벨에 놓아 적 하나를 생성하는 자리입니다. 이 액터의 위치가 적의 발밑이고, 방향이 적의 처음 방향입니다. 적 클래스와 그 종류의
 * 어빌리티 에셋을 비동기로 읽은 뒤 생성하고, 읽은 것을 붙잡고 있어 다시 생성할 때는 읽지 않습니다.
 */
UCLASS()
class AAgEnemySpawner : public AActor
{
	GENERATED_BODY()

public:
	AAgEnemySpawner();

	/** 에셋을 붙잡고 있으면 그 자리에서, 아니면 비동기로 읽은 뒤 적을 생성합니다. 적이 있거나 읽는 중이면 아무것도 하지 않습니다. */
	void Spawn();

	/** 살아 있든 죽었든 자기 적을 지웁니다. */
	void DestroyEnemy();

	bool IsSpawnPending() const
	{
		return bSpawnPending;
	}

	AAgPatrolRoute* GetPatrolRoute() const
	{
		return PatrolRoute;
	}

	/** 읽기를 기다린 생성이 끝나면 부릅니다. 클래스를 읽지 못해 생성하지 못했을 때도 부릅니다. */
	FSimpleMulticastDelegate OnSpawnFinished;

#if WITH_EDITOR
	/** 편집 중에 적 블루프린트의 기본값(캐릭터 데이터 ID, 메시)을 봅니다. 클래스는 그 자리에서 읽습니다. */
	AG_API const AAgEnemyCharacter* GetEnemyDefaults() const;

	/** 에디터가 적 블루프린트를 끌어다 놓아 스포너를 만들 때 그 블루프린트를 정합니다. */
	AG_API void SetEnemyClass(TSubclassOf<AAgEnemyCharacter> InEnemyClass);

	virtual void PostRegisterAllComponents() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;
	virtual void CheckForErrors() override;
#endif

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleClassLoaded();
	void SpawnEnemy();

#if WITH_EDITOR
	/** 편집 화면에 적 외형을 보여 주는 미리 보기 컴포넌트를 지금의 적 블루프린트로 다시 만듭니다. */
	void UpdatePreview();

	void AddPreviewComponent(UMeshComponent& Component, USceneComponent& Parent, FName SocketName);

	/** 정찰 경로를 쓸 수 없는 MOV와, 이 자리에서 가장 가까운 정찰 지점까지 걸어서 갈 수 없는 배치를 모읍니다. */
	void CollectPatrolProblems(TArray<TSharedRef<FTokenizedMessage>>& OutMessages) const;

	/** 그룹 이름을 잘못 적으면 그룹이 갈리므로, 같은 그룹의 다른 스포너가 열린 레벨에 없으면 알립니다. */
	void CollectGroupProblems(TArray<TSharedRef<FTokenizedMessage>>& OutMessages) const;
#endif

	UPROPERTY(EditInstanceOnly, Category = "Enemy")
	TSoftClassPtr<AAgEnemyCharacter> EnemyClass;

	UPROPERTY(EditInstanceOnly, Category = "Enemy")
	TObjectPtr<AAgPatrolRoute> PatrolRoute;

	/** 같은 이름을 가진 적끼리 플레이어 인식을 나눕니다. 비워 두면 그룹이 없습니다. */
	UPROPERTY(EditInstanceOnly, Category = "Enemy")
	FName GroupId;

	UPROPERTY(VisibleInstanceOnly, Category = "Enemy")
	TObjectPtr<AAgEnemyCharacter> Enemy;

	// 적이 모두 죽어도 클래스와 어빌리티 에셋이 내려가지 않게 붙잡습니다.
	UPROPERTY()
	TSubclassOf<AAgEnemyCharacter> LoadedClass;

	UPROPERTY()
	TObjectPtr<UAgAbilitySet> AbilitySet;

	TSharedPtr<FStreamableHandle> LoadHandle;

	bool bSpawnPending = false;

#if WITH_EDITORONLY_DATA
	/** 적의 몸과, 블루프린트에서 몸에 붙인 메시(무기)의 미리 보기입니다. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMeshComponent>> PreviewComponents;

	UPROPERTY()
	TObjectPtr<UAgEnemySightPreviewComponent> SightPreview;

	UPROPERTY()
	TObjectPtr<UAgPatrolPreviewComponent> PatrolPreview;
#endif
};
