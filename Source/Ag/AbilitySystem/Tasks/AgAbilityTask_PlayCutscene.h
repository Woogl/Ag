// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_PlayCutscene.generated.h"

class ALevelSequenceActor;
class ULevelSequence;

USTRUCT()
struct FAgPlayCutsceneParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<ULevelSequence> Sequence;

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

/**
 * 시퀀스는 게임 전체 시간 배율을 따르므로 매 프레임 주인의 시간 배율을 재생 속도로 둡니다. 시퀀스가 끝나거나
 * 멈추면 카메라와 시네마틱 모드는 시퀀스가 되돌립니다.
 */
UCLASS()
class UAgAbilityTask_PlayCutscene : public UAbilityTask
{
	GENERATED_BODY()

public:
	UAgAbilityTask_PlayCutscene();

	static UAgAbilityTask_PlayCutscene* PlayCutscene(UGameplayAbility* Owner, const FAgPlayCutsceneParams& Params);

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;

protected:
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	UFUNCTION()
	void HandleFinished();

	float GetOwnerTimeDilation() const;

	UPROPERTY()
	TObjectPtr<ULevelSequence> Cutscene;

	UPROPERTY()
	TObjectPtr<ALevelSequenceActor> SequenceActor;

	bool bTagAdded = false;
};
