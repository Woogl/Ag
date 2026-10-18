// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_Face.generated.h"

/** Face가 몸을 돌리는 쪽 */
UENUM()
enum class EAgFaceTo : uint8
{
	/** 행동 대상 쪽 */
	Target,

	/** 이동 입력 방향. 입력이 없으면 돌지 않습니다. */
	MoveInput
};

/** 몸을 바로 돌립니다(Face Task의 Params). 재생 전 몸 돌리기(피격 반응, 패리, 처형 피격, 회피)에 씁니다. */
USTRUCT()
struct FAgFaceParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	EAgFaceTo To = EAgFaceTo::Target;

	virtual EAgTaskPlacement GetPlacements() const override
	{
		return EAgTaskPlacement::Activation | EAgTaskPlacement::Point;
	}

	virtual bool NeedsTarget() const override
	{
		return To == EAgFaceTo::Target;
	}

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

/** 주인의 Yaw를 정한 쪽으로 한 번에 돌리고 끝납니다. */
UCLASS()
class UAgAbilityTask_Face : public UAbilityTask
{
	GENERATED_BODY()

public:
	static UAgAbilityTask_Face* Face(UGameplayAbility* Owner, EAgFaceTo To);

	virtual void Activate() override;

private:
	EAgFaceTo To = EAgFaceTo::Target;
};
