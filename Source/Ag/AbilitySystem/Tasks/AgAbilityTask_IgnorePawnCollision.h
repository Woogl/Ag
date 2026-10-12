// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Engine/EngineTypes.h"
#include "AgAbilityTask_IgnorePawnCollision.generated.h"

class AAgCharacterBase;

/**
 * 도는 동안 주인의 캡슐이 다른 캐릭터를 막지 않습니다(구현 계획서 '돌진 찌르기', '처형'). 끝나면(어빌리티가 끝날 때 포함)
 * 원래대로 되돌리고, 그사이 죽었으면 사망 처리가 정한 대로 둡니다.
 */
UCLASS()
class UAgAbilityTask_IgnorePawnCollision : public UAbilityTask
{
	GENERATED_BODY()

public:
	static UAgAbilityTask_IgnorePawnCollision* IgnorePawnCollision(UGameplayAbility* OwningAbility);

	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	TWeakObjectPtr<AAgCharacterBase> Character;
	TEnumAsByte<ECollisionResponse> SavedResponse = ECR_Block;
};
