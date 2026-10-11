// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "GameplayTagContainer.h"
#include "AgAbilityTask_HoldTag.generated.h"

/**
 * 도는 동안 주인에게 상태 태그 하나를 붙여 둡니다(예: 회피의 무적 구간, 가드 중). 어빌리티가 EndTask로 끝내거나 어빌리티가
 * 끝나면 태그를 뗍니다. 어빌리티가 도는 내내 붙는 태그는 이 태스크 대신 어빌리티의 ActivationOwnedTags에 둡니다.
 */
UCLASS()
class UAgAbilityTask_HoldTag : public UAbilityTask
{
	GENERATED_BODY()

public:
	static UAgAbilityTask_HoldTag* HoldTag(UGameplayAbility* OwningAbility, const FGameplayTag& Tag);

	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	FGameplayTag Tag;
	bool bAdded = false;
};
