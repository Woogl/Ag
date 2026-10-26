// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/AgCharacterBase.h"
#include "GameplayTagContainer.h"
#include "AgEnemyCharacter.generated.h"

class ATargetPoint;
class UAnimMontage;
class UAgEnemySightPreviewComponent;
class UStateTree;
struct FAbilityEndedData;

UCLASS(Abstract)
class AAgEnemyCharacter : public AAgCharacterBase
{
	GENERATED_BODY()

public:
	AAgEnemyCharacter();

	virtual bool CanLosePP() const override;

	int32 GetPhase() const
	{
		return Phase;
	}

	/** 현재 HP로 들어선 마지막 페이즈입니다. 페이즈 전환이 끝나야 현재 페이즈가 됩니다. */
	int32 GetReachedPhase() const;

	/** 들어선 페이즈로의 전환을 아직 시작하지 않았습니다. 시작한 전환이 취소되어도 같은 페이즈로는 다시 전환하지 않습니다. */
	bool IsPhaseTransitionPending() const;

	/** 페이즈 전환 어빌리티를 발동하고, 시작했으면 들어선 페이즈로의 전환을 시작한 것으로 기록합니다. */
	bool StartPhaseTransition();

	bool IsPhaseTransitionActive() const;

	bool CanActivatePattern(FName Id) const;

	virtual bool CanReturnToNonCombat() const
	{
		return true;
	}

	const TArray<TObjectPtr<ATargetPoint>>& GetPatrolPoints() const
	{
		return PatrolPoints;
	}

	const FTransform& GetHomeTransform() const
	{
		return HomeTransform;
	}

	virtual void PreInitializeComponents() override;

	virtual void GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const override;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TObjectPtr<UStateTree> AIStateTree;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void HandleDeathEvent(const FGameplayEventData* Payload) override;

private:
	void DieWithDeathMotion(UAnimMontage& DeathMontage, float StartPosition);
	void HandleDeathMotionBlendingOut(UAnimMontage* Montage, bool bInterrupted);
	/** 처형 시작으로 그로기가 끝나면 PP 회복은 처형 피격 종료까지 미룹니다. */
	void HandleGroggyChanged(const FGameplayTag Tag, int32 NewCount);

	void HandleAbilityEnded(const FAbilityEndedData& Data);

	UPROPERTY(EditInstanceOnly, Category = "AI")
	TArray<TObjectPtr<ATargetPoint>> PatrolPoints;

	FTransform HomeTransform;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UAgEnemySightPreviewComponent> SightPreview;
#endif

	int32 Phase = 1;
	int32 StartedTransitionPhase = 1;

	int32 DeathMotionInstanceId = INDEX_NONE;
};
