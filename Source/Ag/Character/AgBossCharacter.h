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

	AAgCharacterBase* GetTarget() const;

	FVector GetLockOnLocation() const;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TObjectPtr<UStateTree> AIStateTree;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	virtual AActor* GetFacingTarget() const override
	{
		return GetTarget();
	}

private:
	/**
	 * 처형 피격이 State.Groggy를 이어받으므로 PP는 처형 피격이 끝날 때 한 번 채웁니다. 남은 그로기 시간 효과는 다음
	 * 그로기와 섞이지 않게 지웁니다.
	 */
	void HandleGroggyChanged(const FGameplayTag Tag, int32 NewCount);

	void HandleAbilityEnded(const FAbilityEndedData& Data);

	bool IsExecutionTargetOf(AAgPlayerCharacter& Player) const;

	void AttachWorldWidgets();

	void UpdateWorldWidgets();

	UPROPERTY(VisibleAnywhere, Category = "UI")
	TObjectPtr<UWidgetComponent> LockOnMarker;

	UPROPERTY(VisibleAnywhere, Category = "UI")
	TObjectPtr<UWidgetComponent> ExecutionPrompt;

	int32 Phase = 1;
};
