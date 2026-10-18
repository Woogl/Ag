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

/**
 * 보스 AI (보스 사양 'AI 행동 규칙', '전투 시작과 행동 전환').
 * 흐름은 보스 데이터의 StateTree(ST_Boss)에 상태와 전환으로 있고, 이 컨트롤러는 빙의할 때 그것을 실행합니다.
 * 대상이 이 보스를 노려 평타의 한 타나 점프 공격을 시작하면 StateTree 이벤트로 넘깁니다(Backstep). 페이즈 전환이 끝나면 PP를
 * 채우고 2페이즈로 넘어갑니다.
 */
UCLASS()
class AAgBossAIController : public AAIController
{
	GENERATED_BODY()

public:
	AAgBossAIController();

	AAgBossCharacter* GetBoss() const;
	const UAgBossData* GetBossData() const;

	/**
	 * 현재 대상(구현 계획서 'AI 대상'). AI 태스크와 조건, 보스 회전, 패턴이 이 대상을 씁니다. 대상은 여기 한 곳에서 정하며,
	 * 지금은 플레이어입니다. 아직 스폰되지 않았으면 null입니다.
	 */
	AAgCharacterBase* GetTarget() const;

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
	/** 대상이 스폰된 뒤 그 평타·점프 공격 시작 이벤트를 받습니다. */
	void BindTargetEvents();

	/** 대상이 평타의 한 타나 점프 공격을 시작함: 그 공격의 대상이 이 보스일 때만 StateTree 이벤트로 넘깁니다. */
	void HandleTargetBasicAttack(const FGameplayEventData* Payload);

	/** 페이즈 전환이 정상 종료로 끝나면(취소가 아니면) PP를 MaxPP로 채우고 2페이즈로 넘어갑니다. */
	void HandleBossAbilityEnded(const FAbilityEndedData& Data);

	UPROPERTY(VisibleAnywhere, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeComponent;

	TWeakObjectPtr<UAbilitySystemComponent> TargetASC;
	FDelegateHandle BasicAttackHandle;
	TWeakObjectPtr<UAbilitySystemComponent> BossASC;
	FDelegateHandle AbilityEndedHandle;
	bool bPhaseTransitionStarted = false;
};
