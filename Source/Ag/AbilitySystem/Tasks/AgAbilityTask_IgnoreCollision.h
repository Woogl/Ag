// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Engine/EngineTypes.h"
#include "AgAbilityTask_IgnoreCollision.generated.h"

class UPrimitiveComponent;

/**
 * 도는 동안 주인의 루트 충돌 컴포넌트(캐릭터는 캡슐)가 정한 충돌 채널을 무시합니다(예: 돌진 찌르기의 돌진과 처형 피격은 Pawn
 * 채널을 무시해 플레이어를 막지 않음). 끝나면(어빌리티가 끝날 때 포함) 원래 응답으로 되돌리고, 그사이 죽었으면 사망 처리가
 * 정한 대로 둡니다.
 */
UCLASS()
class UAgAbilityTask_IgnoreCollision : public UAbilityTask
{
	GENERATED_BODY()

public:
	static UAgAbilityTask_IgnoreCollision* IgnoreCollision(UGameplayAbility* OwningAbility, ECollisionChannel InChannel);

	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	TWeakObjectPtr<UPrimitiveComponent> Component;
	TEnumAsByte<ECollisionChannel> Channel = ECC_Pawn;
	TEnumAsByte<ECollisionResponse> SavedResponse = ECR_Block;
};
