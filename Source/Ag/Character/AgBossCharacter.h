// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/AgCharacterBase.h"
#include "GameplayTagContainer.h"
#include "AgBossCharacter.generated.h"

struct FGameplayAbilitySpec;

/** The boss 커스터 (보스 사양). Turns toward the player on its own and runs the patterns its AI picks. */
UCLASS(Abstract)
class AAgBossCharacter : public AAgCharacterBase
{
	GENERATED_BODY()

public:
	AAgBossCharacter();

	/** The granted spec of a pattern, or null. */
	FGameplayAbilitySpec* FindPatternSpec(const FGameplayTag& Pattern) const;

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void GrantAbilities() override;

private:
	/** 회전: turns toward the player at the rotation speed, except during the rotation stop windows. */
	void UpdateRotation(float DeltaSeconds);

	/** True from the rotation stop lead before an attack window of a playing montage until the window ends. */
	bool IsInRotationStopWindow() const;
};
