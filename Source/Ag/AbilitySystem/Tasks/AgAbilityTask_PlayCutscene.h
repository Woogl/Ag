// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_PlayCutscene.generated.h"

class ALevelSequenceActor;
class ULevelSequence;

/** 재생할 컷신(PlayCutscene Task의 Params). 행동 처리가 재생할 때를 정합니다. */
USTRUCT()
struct FAgPlayCutsceneParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<ULevelSequence> Sequence;

	virtual EAgTaskPlacement GetPlacements() const override
	{
		return EAgTaskPlacement::Activation;
	}
};

/**
 * 컷신(레벨 시퀀스) 하나를 Origin의 위치·방향을 기준으로 재생합니다(시퀀스의 Transform Origin). 재생하는 동안 플레이어 컨트롤러는
 * 시네마틱 모드(시야·이동 입력 무시, HUD 숨김)이고, 시간 정지 중이면 시퀀스를 정상 속도로 재생합니다. 끝나면 OnFinished를 알리고,
 * 그 전에 어빌리티가 끝나면 시퀀스를 멈춥니다. 시퀀스가 끝나거나 멈추면 카메라와 시네마틱 모드는 시퀀스가 되돌립니다.
 */
UCLASS()
class UAgAbilityTask_PlayCutscene : public UAbilityTask
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FGenericGameplayTaskDelegate OnFinished;

	static UAgAbilityTask_PlayCutscene* PlayCutscene(UGameplayAbility* OwningAbility, ULevelSequence* Cutscene, AActor* Origin);

	virtual void Activate() override;

protected:
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	UFUNCTION()
	void HandleFinished();

	UPROPERTY()
	TObjectPtr<ULevelSequence> Cutscene;

	UPROPERTY()
	TObjectPtr<AActor> Origin;

	UPROPERTY()
	TObjectPtr<ALevelSequenceActor> SequenceActor;
};
