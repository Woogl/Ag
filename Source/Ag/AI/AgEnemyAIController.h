// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AgEnemyAIController.generated.h"

class AAgCharacterBase;
class AAgEnemyCharacter;
class UAISenseConfig_Damage;
class UAISenseConfig_Sight;
class UStateTreeAIComponent;
struct FAgEnemyAIData;
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

	/** 캐릭터 데이터의 범위에서 무작위로 정한 길이로 패턴 후딜레이를 겁니다. 걸려 있으면 새 길이로 처음부터 셉니다. */
	void StartPostPatternDelay();
	void ClearPostPatternDelay();
	bool IsInPostPatternDelay() const;

	virtual FGenericTeamId GetGenericTeamId() const override;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

private:
	void ConfigurePerception(const FAgEnemyAIData& AI);

	void HandleTargetBasicAttack(const FGameplayEventData* Payload);

	UPROPERTY(VisibleAnywhere, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeComponent;

	UPROPERTY(VisibleAnywhere, Category = "AI")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(VisibleAnywhere, Category = "AI")
	TObjectPtr<UAISenseConfig_Damage> DamageConfig;

	FDelegateHandle BasicAttackHandle;

	/** 패턴 후딜레이가 끝나는 월드 시각입니다. 움찔이나 상태 이동과 관계없이 흐르도록 StateTree 상태가 아니라 여기에 둡니다. */
	double PostPatternDelayEndTime = 0.0;
};
