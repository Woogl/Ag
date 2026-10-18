// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_PlayCutscene.generated.h"

class ALevelSequenceActor;
class ULevelSequence;

/** 컷신(PlayCutscene Task의 Params). 구간이 시작되면 Sequence를 재생하고, 끝나면 Next로 전환합니다. */
USTRUCT()
struct FAgPlayCutsceneParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<ULevelSequence> Sequence;

	UPROPERTY()
	FAgTransition Next;

	virtual EAgTaskPlacement GetPlacements() const override
	{
		return EAgTaskPlacement::Window;
	}

	virtual void GetFieldRules(FAgJsonFieldRules& Rules) const override;

	virtual const FAgTransition* GetTransition() const override
	{
		return &Next;
	}

	virtual bool WaitsForTransition() const override
	{
		return true;
	}

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

/**
 * 컷신(레벨 시퀀스) 하나를 주인의 위치·방향을 기준으로 재생합니다(시퀀스의 Transform Origin). 재생하는 동안 플레이어 컨트롤러는
 * 시네마틱 모드(시야·이동 입력 무시, HUD 숨김)입니다. 시퀀스는 게임 전체 시간 배율을 따르므로, 매 프레임 주인의 시간 배율을 재생
 * 속도로 두어 주인의 시간으로 재생합니다(시간 정지에서 빠진 주인은 정상 속도, 주인의 히트스톱에는 멈춤). 끝나면 Next로 전환하고,
 * 그 전에 구간이나 행동이 끝나면 시퀀스를 멈춥니다. 시퀀스가 끝나거나 멈추면 카메라와 시네마틱 모드는 시퀀스가 되돌립니다.
 */
UCLASS()
class UAgAbilityTask_PlayCutscene : public UAbilityTask
{
	GENERATED_BODY()

public:
	UAgAbilityTask_PlayCutscene();

	static UAgAbilityTask_PlayCutscene* PlayCutscene(UGameplayAbility* Owner, const FAgPlayCutsceneParams& Params, int32 MontageInstanceId);

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;

protected:
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	UFUNCTION()
	void HandleFinished();

	/** 주인의 시간 배율. 주인이 없으면 1입니다. */
	float GetOwnerTimeDilation() const;

	UPROPERTY()
	TObjectPtr<ULevelSequence> Cutscene;

	UPROPERTY()
	TObjectPtr<ALevelSequenceActor> SequenceActor;

	FAgTransition Next;
	int32 MontageInstanceId = INDEX_NONE;
};
