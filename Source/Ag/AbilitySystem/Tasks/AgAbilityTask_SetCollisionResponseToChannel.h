// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Data/AgTaskParams.h"
#include "Engine/EngineTypes.h"
#include "AgAbilityTask_SetCollisionResponseToChannel.generated.h"

class UPrimitiveComponent;

/** 주인 캡슐의 채널 응답(SetCollisionResponseToChannel Task의 Params). 발동 Task 배열의 항목은 행동 처리가 바꿀 때를 정합니다. */
USTRUCT()
struct FAgSetCollisionResponseToChannelParams : public FAgTaskParams
{
	GENERATED_BODY()

	UPROPERTY()
	TEnumAsByte<ECollisionChannel> Channel = ECC_MAX;

	UPROPERTY()
	TEnumAsByte<ECollisionResponse> Response = ECR_MAX;

	virtual EAgTaskPlacement GetPlacements() const override
	{
		return EAgTaskPlacement::Activation | EAgTaskPlacement::Window;
	}

	virtual UAbilityTask* MakeTask(const FAgTaskContext& Context) const override;
};

/** 지정 채널 응답을 바꾸고 종료 시 복원합니다. 같은 대상·채널은 이전 Task가 종료된 뒤 변경합니다. */
UCLASS()
class UAgAbilityTask_SetCollisionResponseToChannel : public UAbilityTask
{
	GENERATED_BODY()

public:
	static UAgAbilityTask_SetCollisionResponseToChannel* SetCollisionResponseToChannel(
	    UGameplayAbility* OwningAbility, UPrimitiveComponent* InComponent, ECollisionChannel InChannel, ECollisionResponse InResponse);

	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	TWeakObjectPtr<UPrimitiveComponent> Component;
	bool bResponseApplied = false;
	TEnumAsByte<ECollisionChannel> Channel = ECC_Pawn;
	TEnumAsByte<ECollisionResponse> Response = ECR_Ignore;
	TEnumAsByte<ECollisionResponse> SavedResponse = ECR_Block;
};
