// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgTaskParams.h"
#include "AgAbilityTask_Face.generated.h"

UENUM()
enum class EAgFaceTo : uint8
{
	Target,
	MoveInput
};

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
