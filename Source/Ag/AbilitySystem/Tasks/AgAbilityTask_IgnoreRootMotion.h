// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Animation/AnimEnums.h"
#include "AgAbilityTask_IgnoreRootMotion.generated.h"

class UAnimInstance;

/**
 * 도는 동안 애님 인스턴스가 몽타주의 루트 모션을 무시합니다(구현 계획서 'DashThrust', '넉업').
 * 몽타주가 루트 모션을 주는 동안에는 캐릭터 무브먼트가 루트 모션 소스를 쓰지 않으므로, 루트 모션 소스로 옮기는 동안
 * 함께 돌립니다. 끝나면(어빌리티가 끝날 때 포함) 원래 루트 모션 모드로 되돌립니다.
 */
UCLASS()
class UAgAbilityTask_IgnoreRootMotion : public UAbilityTask
{
	GENERATED_BODY()

public:
	static UAgAbilityTask_IgnoreRootMotion* IgnoreRootMotion(UGameplayAbility* OwningAbility);

	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	TWeakObjectPtr<UAnimInstance> AnimInstance;
	TEnumAsByte<ERootMotionMode::Type> SavedRootMotionMode = ERootMotionMode::RootMotionFromMontagesOnly;
};
