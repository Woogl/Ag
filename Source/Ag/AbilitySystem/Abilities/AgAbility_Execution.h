// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgAbility_Execution.generated.h"

class AAgCharacterBase;
class AAgPlayerCharacter;

/**
 * 처형 (플레이어 사양 '처형'): 처형 타겟팅 범위 안에서 그로기 중인 보스에게 그로기마다 한 번 쓰는 마무리 공격입니다.
 * 보스는 처형 몽타주와 같은 프레임에 플레이어 쪽으로 돌아 처형 피격 몽타주를 시작하고, 그 몽타주는 두 모션의 시간
 * 차이만큼 첫 프레임에 머물러 두 모션이 프레임 단위로 맞물립니다. 플레이어는 찌르기까지 Motion Warping으로 보스 정면에
 * 붙고, 처형이 끝날 때까지 무적입니다. 타격(찌르기와 마지막 일격, 각각 Execution의 타 하나)마다 히트스톱 없이 대미지를
 * 적용합니다. 후딜이 없어 모션이 끝나기 전에는 어떤 행동도 처형을 끊지 못합니다.
 */
UCLASS()
class UAgAbility_Execution : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgAbility_Execution();

	/** 전용 값은 공격 데이터 하나입니다. TargetingRange가 처형 타겟팅 범위이고, 타 하나가 타격 하나입니다. */
	virtual const UScriptStruct* GetActionParamsType() const override { return FAgPlayerAttack::StaticStruct(); }

	/** 지금 처형할 수 있는 대상: 발동 조건 1, 2를 만족하는 가장 가까운 적(구현 계획서 '대상 찾기'). 없으면 null입니다. */
	static AAgCharacterBase* FindExecutionTarget(const AAgPlayerCharacter* Player);

protected:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	UFUNCTION()
	void HandleStrike(FGameplayEventData Payload);

	UFUNCTION()
	void HandleMontageFinished();

	UFUNCTION()
	void HandleMontageCancelled();

	TWeakObjectPtr<AAgCharacterBase> Target;

	/** 이번 처형에서 지난 타격 수. Execution의 마지막 타가 마지막 일격입니다. */
	int32 StrikeIndex = 0;
};
