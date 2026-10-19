// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AgBossAIController.generated.h"

class AAgBossCharacter;
class AAgCharacterBase;
class UAbilitySystemComponent;
class UAgBossData;
class UStateTreeAIComponent;
struct FAbilityEndedData;
struct FGameplayEventData;

UCLASS()
class AAgBossAIController : public AAIController
{
	GENERATED_BODY()

public:
	AAgBossAIController();

	AAgBossCharacter* GetBoss() const;
	const UAgBossData* GetBossData() const;

	AAgCharacterBase* GetTarget() const;

	bool IsBossReacting() const;

	bool HasStartedPhaseTransition() const
	{
		return bPhaseTransitionStarted;
	}
	void MarkPhaseTransitionStarted()
	{
		bPhaseTransitionStarted = true;
	}

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void UpdateTarget();

	void BindTargetEvents();

	void HandleTargetBasicAttack(const FGameplayEventData* Payload);

	void HandleBossAbilityEnded(const FAbilityEndedData& Data);

	UPROPERTY(VisibleAnywhere, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeComponent;

	TWeakObjectPtr<AAgCharacterBase> Target;
	TWeakObjectPtr<UAbilitySystemComponent> TargetASC;
	FDelegateHandle BasicAttackHandle;
	TWeakObjectPtr<UAbilitySystemComponent> BossASC;
	FDelegateHandle AbilityEndedHandle;
	bool bPhaseTransitionStarted = false;
};
