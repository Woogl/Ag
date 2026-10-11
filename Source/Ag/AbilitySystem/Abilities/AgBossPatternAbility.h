// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AgBossPatternAbility.generated.h"

class AAgBossCharacter;
class AAgCharacterBase;
class UAnimMontage;
struct FAgBossPattern;

/**
 * 보스 공용 패턴(A1, A2, A5, A7): 패턴 몽타주를 패턴 재생 속도로 공격 데이터와 함께 재생하고, 공격 중 이동 태스크로 정지
 * 간격(보스 사양 '이동')을 지키며 몽타주의 워핑 구간(A1·A5 1타, A7) 동안 다가갑니다. 쿨다운이 있는 패턴은 시작할 때
 * 쿨다운을 겁니다. 공격 예고 섬광은 몽타주의 GameplayCue 노티파이가 켭니다.
 * 부여된 스펙마다 패턴 ID를 동적 태그로 갖습니다. 특수 패턴(A3, A4, A6, B1)은 이 클래스를 상속합니다.
 */
UCLASS()
class UAgBossPatternAbility : public UAgGameplayAbility
{
	GENERATED_BODY()

public:
	UAgBossPatternAbility();

	/** The pattern ID a granted spec runs. */
	static FGameplayTag GetPatternTag(const FGameplayAbilitySpec& Spec);

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** 쿨다운 per pattern: the pattern's cooldown tag and time from Boss.json, counted from the pattern start. */
	virtual bool CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

	/** Pattern motion play rate: 모션 재생 속도 times the phase 2 패턴 진행 속도. */
	virtual float GetPatternPlayRate() const;

	const FAgBossPattern* GetPattern() const;

	AAgBossCharacter* GetBoss() const;

	/** 보스 AI의 현재 대상(구현 계획서 'AI 대상'). 패턴은 이 대상을 향해 움직이고 겨눕니다. */
	AAgCharacterBase* GetTarget() const;

	UAnimMontage* GetMontage() const { return Montage; }

	/** 공용 이동 규칙(정지 간격, 워핑 구간의 접근)을 씁니다. A3, A4, B1은 각자의 이동 규칙을 따릅니다. */
	virtual bool UsesSharedMovement() const { return true; }

	/** 패턴 몽타주가 재생되기 시작하면 한 번 불립니다. 특수 패턴은 여기서 자기 이동을 시작합니다. 재생에 실패하면 불리지 않습니다. */
	virtual void OnPatternStarted() {}

private:
	/** How far the montage's own root motion moves forward before the first attack window. */
	static float GetForwardTravelBeforeFirstWindow(const UAnimMontage* Montage);

	UFUNCTION()
	void HandleMontageFinished();

	UFUNCTION()
	void HandleMontageCancelled();

	UPROPERTY()
	TObjectPtr<UAnimMontage> Montage;
};
