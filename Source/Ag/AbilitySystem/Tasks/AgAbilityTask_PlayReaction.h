// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AgAbilityTask_PlayReaction.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAgAbilityTask_HoldTag;
class UAnimMontage;

/**
 * 피격 반응 모션(전투 시스템 '피격 모션', '가드 반응'): 반응 몽타주를 재생하고, 그 블렌드 아웃이 시작되면 이어지는 몽타주(넉업의
 * 기상)를 한 번 잇습니다. 반응은 블렌드 아웃이 시작될 때가 아니라 마지막 모션이 끝날 때 끝나며(OnCompleted), 모션이 끊기면
 * OnInterrupted를 보냅니다. 바닥에 쓰러진 순간을 표시한 모션(Event.DownGrounded: 넉백, 넉업)은 그때부터 태스크가 끝날 때까지 무적입니다.
 */
UCLASS()
class UAgAbilityTask_PlayReaction : public UAbilityTask
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FGenericGameplayTaskDelegate OnCompleted;

	UPROPERTY(BlueprintAssignable)
	FGenericGameplayTaskDelegate OnInterrupted;

	static UAgAbilityTask_PlayReaction* PlayReaction(UGameplayAbility* OwningAbility, UAnimMontage* InMontage, UAnimMontage* InFollowUpMontage = nullptr);

	/** 반응 도중 모션을 바꿉니다(넉업의 낙하, 착지). 이어지는 몽타주를 아직 재생하지 않았으면 이 모션 뒤에 잇습니다. */
	void PlayMontage(UAnimMontage* InMontage, FName StartSection = NAME_None);

	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	void HandleGrounded(const FGameplayEventData* Payload);

	/** 블렌드 아웃이 시작되면 이어지는 몽타주를 한 번 재생해 섞습니다. 반응은 마지막 모션이 끝날 때 끝납니다. */
	UFUNCTION()
	void HandleMontageBlendingOut();

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageCancelled();

	UPROPERTY()
	TObjectPtr<UAnimMontage> Montage;

	UPROPERTY()
	TObjectPtr<UAnimMontage> FollowUpMontage;

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	/** 쓰러진 뒤의 무적(State.Invincible) */
	UPROPERTY()
	TObjectPtr<UAgAbilityTask_HoldTag> InvincibleTask;

	FDelegateHandle GroundedHandle;
	bool bPlayingFollowUp = false;
};
