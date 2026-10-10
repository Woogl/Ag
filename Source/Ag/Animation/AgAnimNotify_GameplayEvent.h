// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "AgAnimNotify_GameplayEvent.generated.h"

/**
 * Marks a moment in a motion by sending a gameplay event to the owner (for example, 다운 lying down: Event.DownGrounded).
 * The running ability waits for the event.
 */
UCLASS(meta = (DisplayName = "Gameplay Event"))
class UAgAnimNotify_GameplayEvent : public UAnimNotify
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Event", meta = (Categories = "Event"))
	FGameplayTag EventTag;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
