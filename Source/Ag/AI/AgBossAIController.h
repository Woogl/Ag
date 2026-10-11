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

/**
 * 보스 AI (보스 사양 'AI 행동 규칙', '전투 시작과 행동 전환').
 * 흐름은 보스 데이터의 StateTree(ST_Boss)에 상태와 전환으로 있고, 이 컨트롤러는 빙의할 때 그것을 실행합니다.
 * 플레이어가 평타의 각 타를 시작하면 StateTree 이벤트로 넘깁니다(B1 백스텝).
 */
UCLASS()
class AAgBossAIController : public AAIController
{
	GENERATED_BODY()

public:
	AAgBossAIController();

	AAgBossCharacter* GetBoss() const { return Boss.Get(); }
	const UAgBossData* GetBossData() const;

	/** 플레이어 캐릭터. 아직 스폰되지 않았으면 null입니다. */
	AAgCharacterBase* GetPlayer() const;

	/** 넉백이나 그로기(처형 피격 포함) 중인지. 그동안 AI는 피격 반응 상태에서 기다립니다. */
	bool IsBossReacting() const;

	/** 페이즈 전환은 보스전에서 한 번만 시작합니다. */
	bool HasStartedPhaseTransition() const { return bPhaseTransitionStarted; }
	void MarkPhaseTransitionStarted() { bPhaseTransitionStarted = true; }

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	/** 플레이어가 스폰된 뒤 그 평타 시작 이벤트를 받습니다. */
	void BindPlayerEvents();

	void HandlePlayerBasicAttack(const FGameplayEventData* Payload);

	UPROPERTY(VisibleAnywhere, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeComponent;

	TWeakObjectPtr<AAgBossCharacter> Boss;
	TWeakObjectPtr<UAbilitySystemComponent> PlayerASC;
	FDelegateHandle BasicAttackHandle;
	bool bPhaseTransitionStarted = false;
};
