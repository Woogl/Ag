// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/AgInteractable.h"
#include "AgCheckpoint.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/**
 * 상호작용하면 휴식하는 체크포인트입니다. 휴식하면 플레이어가 회복하고 레벨의 적이 처음 상태로 돌아갑니다. 범위의 충돌을 끄지
 * 않으므로 언제든 몇 번이든 쓸 수 있습니다. 범위는 구의 반지름이, 안내 지점은 PromptPoint의 위치가 정합니다.
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

private:
	UPROPERTY(VisibleAnywhere, Category = "Checkpoint")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** 이 컴포넌트의 위치가 안내 지점이자 범위의 중심입니다. */
	UPROPERTY(VisibleAnywhere, Category = "Checkpoint")
	TObjectPtr<USceneComponent> PromptPoint;

	UPROPERTY(VisibleAnywhere, Category = "Checkpoint")
	TObjectPtr<USphereComponent> InteractionRange;

	/** 안내에 적는 행동 이름입니다. 문구 표(Content/StringTables/StringTable.csv)의 항목을 고릅니다. */
	UPROPERTY(EditAnywhere, Category = "Checkpoint")
	FText InteractionText;
};
