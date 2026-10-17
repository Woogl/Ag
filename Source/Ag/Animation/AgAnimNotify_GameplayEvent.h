// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "AgAnimNotify_GameplayEvent.generated.h"

/** 모션의 한 순간을 엔진 몽타주 알림으로 방송합니다. 재생 소유자가 EventTag를 처리합니다. */
UCLASS(meta = (DisplayName = "Gameplay Event"))
class UAgAnimNotify_GameplayEvent : public UAnimNotify
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Event", meta = (Categories = "Event"))
	FGameplayTag EventTag;

	virtual void Notify(
	    USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
