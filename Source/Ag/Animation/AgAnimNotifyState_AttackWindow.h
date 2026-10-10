// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AgAnimNotifyState_AttackWindow.generated.h"

/**
 * 판정 구간: while active, the owner's weapon edge sweeps for hits.
 * One window is one hit (1타); HitIndex picks the hit's attack data.
 * The end of an action's last window is the start of its recovery (후딜).
 */
UCLASS(meta = (DisplayName = "Attack Window"))
class UAgAnimNotifyState_AttackWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	/** Which hit of the action this window is, from 0. */
	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = 0))
	int32 HitIndex = 0;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
