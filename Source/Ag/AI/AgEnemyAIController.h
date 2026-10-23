// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "AgEnemyAIController.generated.h"

class AAgCharacterBase;
class AAgEnemyCharacter;
class UAbilitySystemComponent;
class UAgEnemyData;
class UAISenseConfig_Damage;
class UAISenseConfig_Sight;
class UStateTreeAIComponent;
struct FGameplayEventData;

UCLASS()
class AAgEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AAgEnemyAIController();

	AAgEnemyCharacter* GetEnemy() const;
	const UAgEnemyData* GetEnemyData() const;

	AAgCharacterBase* GetTarget() const;

	bool IsEnemyReacting() const;

	/** 적대 상대가 시야에 보이거나, 마지막으로 보이거나 맞은 뒤 놓친 시간이 전투 규칙의 TargetLostTime보다 짧습니다. */
	bool IsTargetNoticed() const;

	int32 GetPhaseTransitionTarget() const
	{
		return PhaseTransitionTarget;
	}
	void MarkPhaseTransitionStarted(int32 TargetPhase)
	{
		PhaseTransitionTarget = TargetPhase;
	}

	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void ConfigureSight(const UAgEnemyData& Data);

	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	void BindTargetEvents();

	void HandleTargetBasicAttack(const FGameplayEventData* Payload);

	UPROPERTY(VisibleAnywhere, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeComponent;

	UPROPERTY(VisibleAnywhere, Category = "AI")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(VisibleAnywhere, Category = "AI")
	TObjectPtr<UAISenseConfig_Damage> DamageConfig;

	bool bTargetInSight = false;
	double LastNoticedTime = -1.;

	TWeakObjectPtr<UAbilitySystemComponent> TargetASC;
	FDelegateHandle BasicAttackHandle;
	/** 마지막으로 시작한 페이즈 전환이 향하는 페이즈입니다. 전환이 취소되어도 같은 페이즈로는 다시 전환하지 않습니다. */
	int32 PhaseTransitionTarget = 1;
};
