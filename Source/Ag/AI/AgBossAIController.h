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

	int32 GetPhaseTransitionTarget() const
	{
		return PhaseTransitionTarget;
	}
	void MarkPhaseTransitionStarted(int32 TargetPhase)
	{
		PhaseTransitionTarget = TargetPhase;
	}

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void UpdateTarget();

	void BindTargetEvents();

	void HandleTargetBasicAttack(const FGameplayEventData* Payload);

	UPROPERTY(VisibleAnywhere, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeComponent;

	TWeakObjectPtr<UAbilitySystemComponent> TargetASC;
	FDelegateHandle BasicAttackHandle;
	/** 마지막으로 시작한 페이즈 전환이 향하는 페이즈입니다. 전환이 취소되어도 같은 페이즈로는 다시 전환하지 않습니다. */
	int32 PhaseTransitionTarget = 1;
};
