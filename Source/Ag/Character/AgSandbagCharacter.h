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

protected:
	virtual void BeginPlay() override;

	virtual void HandleDeathEvent(const FGameplayEventData* Payload) override;

private:
	void HandlePatternActivated(UGameplayAbility* Ability);
	void HandlePatternEnded(const FAbilityEndedData& Data);
};
