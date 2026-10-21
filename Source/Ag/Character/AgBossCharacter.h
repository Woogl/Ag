// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/AgCharacterBase.h"
#include "GameplayTagContainer.h"
#include "AgBossCharacter.generated.h"

class AAgPlayerCharacter;
class UStateTree;
class UWidgetComponent;
struct FAbilityEndedData;

UCLASS(Abstract)
class AAgBossCharacter : public AAgCharacterBase
{
	GENERATED_BODY()

public:
	AAgBossCharacter();

	int32 GetPhase() const
	{
		return Phase;
	}

	FVector GetLockOnLocation() const;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TObjectPtr<UStateTree> AIStateTree;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	/** 처형 시작으로 그로기가 끝나면 PP 회복은 처형 피격 종료까지 미룹니다. */
	void HandleGroggyChanged(const FGameplayTag Tag, int32 NewCount);

	/** 피격 반응 중에 PP가 0이 되었으면 그 반응이 끝날 때 그로기를 시작합니다. */
	void HandleHitReactionChanged(const FGameplayTag Tag, int32 NewCount);

	void HandleAbilityEnded(const FAbilityEndedData& Data);

	bool IsExecutionTargetOf(AAgPlayerCharacter& Player) const;

	void UpdateWorldWidgets();

	UPROPERTY(VisibleAnywhere, Category = "UI")
	TObjectPtr<UWidgetComponent> LockOnMarker;

	UPROPERTY(VisibleAnywhere, Category = "UI")
	TObjectPtr<UWidgetComponent> ExecutionPrompt;

	int32 Phase = 1;
};
