// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/AgInteractable.h"
#include "AgGate.generated.h"

class UBoxComponent;
class UInterpToMovementComponent;
class UStaticMeshComponent;

/**
 * 상호작용하면 위로 올라가 열리는 성문입니다. 한 번 열리면 닫히지 않습니다. 열리는 시간과 올라가는 높이는 BP의 이동
 * 컴포넌트(Duration, ControlPoints)가, 막는 범위와 상호작용 범위는 상자 크기가 정합니다.
 */
UCLASS(Abstract)
class AAgGate : public AActor, public IAgInteractable
{
	GENERATED_BODY()

public:
	AAgGate();

	virtual FText GetInteractionText() const override;
	virtual FVector GetInteractionLocation() const override;
	virtual void Interact(AAgPlayerCharacter& Player) override;

	virtual void PreInitializeComponents() override;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleOpened(const FHitResult& ImpactResult, float Time);

	/** 보이는 문입니다. 올라가는 도중에 아래로 지나가지 못하도록 충돌은 Blocker가 맡습니다. */
	UPROPERTY(VisibleAnywhere, Category = "Gate")
	TObjectPtr<UStaticMeshComponent> Door;

	/** 문이 다 올라갈 때까지 이동, 시선, 탄환을 막습니다. */
	UPROPERTY(VisibleAnywhere, Category = "Gate")
	TObjectPtr<UBoxComponent> Blocker;

	UPROPERTY(VisibleAnywhere, Category = "Gate")
	TObjectPtr<UBoxComponent> InteractionRange;

	/** 이 컴포넌트의 위치가 안내 지점입니다. */
	UPROPERTY(VisibleAnywhere, Category = "Gate")
	TObjectPtr<USceneComponent> PromptPoint;

	UPROPERTY(VisibleAnywhere, Category = "Gate")
	TObjectPtr<UInterpToMovementComponent> OpenMovement;

	/** 안내에 적는 행동 이름입니다. 문구 표(Content/StringTables/StringTable.csv)의 항목을 고릅니다. */
	UPROPERTY(EditAnywhere, Category = "Gate")
	FText InteractionText;
};
