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

	/** 페이즈: 1, or 2 after the phase transition ends. */
	int32 GetPhase() const { return Phase; }

	/** 페이즈 전환이 끝남: the phase 2 changes apply from now on. */
	void EnterPhase2();

	/** 2페이즈 변화 '패턴 진행 속도': 1 in phase 1. Scales the pattern motions and the movement inside patterns. */
	float GetPatternSpeed() const;

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void GrantAbilities() override;
	virtual float GetMoveSpeedMultiplier() const override;

private:
	/** 회전: turns toward the player at the rotation speed, except during the rotation stop windows. */
	void UpdateRotation(float DeltaSeconds);

	/** True from the rotation stop lead before an attack window of a playing montage until the window ends. */
	bool IsInRotationStopWindow() const;

	int32 Phase = 1;
};
