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

	/** 피격 반응 중에 PP가 0이 되었으면 그 반응이 끝날 때 그로기를 시작합니다. */
	void HandleHitReactionChanged(const FGameplayTag Tag, int32 NewCount);

	void HandleAbilityEnded(const FAbilityEndedData& Data);

	UPROPERTY(EditInstanceOnly, Category = "AI")
	TArray<TObjectPtr<ATargetPoint>> PatrolPoints;

	FTransform HomeTransform;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UAgEnemySightPreviewComponent> SightPreview;
#endif

	int32 Phase = 1;

	int32 DeathMotionInstanceId = INDEX_NONE;
};
