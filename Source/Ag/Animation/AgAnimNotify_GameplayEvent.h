// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "AgAnimNotify_GameplayEvent.generated.h"

/**
 * 모션의 한 순간을 주인에게 게임플레이 이벤트로 알립니다(예: 넉백으로 바닥에 쓰러진 순간 Event.DownGrounded).
 * 실행 중인 어빌리티가 이 이벤트를 기다립니다.
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
