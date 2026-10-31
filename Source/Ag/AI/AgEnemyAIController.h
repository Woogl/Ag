// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AgEnemyAIController.generated.h"

class AAgCharacterBase;
class AAgEnemyCharacter;
class UAgCharacterData;
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

	AAgCharacterBase* GetTarget() const;

	bool IsEnemyReacting() const;
	bool IsEnemyFlinching() const;

	bool IsTargetNoticed() const;

	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

private:
	void ConfigurePerception(const UAgCharacterData& Data);

	void HandleTargetBasicAttack(const FGameplayEventData* Payload);

	UPROPERTY(VisibleAnywhere, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeComponent;

	UPROPERTY(VisibleAnywhere, Category = "AI")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(VisibleAnywhere, Category = "AI")
	TObjectPtr<UAISenseConfig_Damage> DamageConfig;

	FDelegateHandle BasicAttackHandle;
};
