// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/AgInteractable.h"
#include "AgCheckpoint.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/**
 * 상호작용하면 휴식하는 체크포인트입니다. 휴식하면 플레이어가 회복하고 레벨의 적이 처음 상태로 돌아가며, 이 던전을 다시 열 때
 * 시작하는 자리가 됩니다. 범위의 충돌을 끄지 않으므로 언제든 몇 번이든 쓸 수 있습니다. 범위는 구의 반지름이, 안내 지점은
 * PromptPoint의 위치가, 시작하는 자리는 StartPoint의 위치와 방향이 정합니다.
 */
UCLASS(Abstract)
class AAgCheckpoint : public AActor, public IAgInteractable
{
	GENERATED_BODY()

public:
	AAgCheckpoint();

	virtual FText GetInteractionText() const override;
	virtual FVector GetInteractionLocation() const override;
	virtual void Interact(AAgPlayerCharacter& Player) override;

	/** 이 체크포인트에서 시작할 때 플레이어의 발밑 자리와 방향입니다. */
	FTransform GetStartTransform() const;

private:
	UPROPERTY(VisibleAnywhere, Category = "Checkpoint")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** 기본은 정면 1.5m에서 체크포인트를 바라보는 자리입니다. 바닥이 있고 비어 있는 자리에 둡니다. */
	UPROPERTY(VisibleAnywhere, Category = "Checkpoint")
	TObjectPtr<USceneComponent> StartPoint;

	/** 이 컴포넌트의 위치가 안내 지점이자 범위의 중심입니다. */
	UPROPERTY(VisibleAnywhere, Category = "Checkpoint")
	TObjectPtr<USceneComponent> PromptPoint;

	UPROPERTY(VisibleAnywhere, Category = "Checkpoint")
	TObjectPtr<USphereComponent> InteractionRange;

	/** 안내에 적는 행동 이름입니다. 문구 표(Content/StringTables/StringTable.csv)의 항목을 고릅니다. */
	UPROPERTY(EditAnywhere, Category = "Checkpoint")
	FText InteractionText;
};
