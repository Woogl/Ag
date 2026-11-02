// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/AgEnemyCharacter.h"
#include "AgSandbagCharacter.generated.h"

class UGameplayAbility;
struct FAbilityEndedData;

UCLASS(Abstract)
class AAgSandbagCharacter : public AAgEnemyCharacter
{
	GENERATED_BODY()

public:
	AAgSandbagCharacter();

	/** 공격 샌드백도 StateTree의 전투 상태에 들어가지만, 네임플레이트는 락온이나 최근 피해로만 표시합니다. */
	virtual bool ShowsNameplateInCombat() const override
	{
		return false;
	}

	virtual bool ShowsDamageNumbers() const override
	{
		return true;
	}

protected:
	virtual void BeginPlay() override;

	virtual void HandleDeathEvent(const FGameplayEventData* Payload) override;

private:
	void HandlePatternActivated(UGameplayAbility* Ability);
	void HandlePatternEnded(const FAbilityEndedData& Data);
};
