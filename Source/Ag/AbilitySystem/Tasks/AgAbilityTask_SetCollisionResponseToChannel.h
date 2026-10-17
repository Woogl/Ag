// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Engine/EngineTypes.h"
#include "AgAbilityTask_SetCollisionResponseToChannel.generated.h"

class UPrimitiveComponent;

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
