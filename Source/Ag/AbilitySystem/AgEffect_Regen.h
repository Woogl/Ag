// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "AgEffect_Regen.generated.h"

class AAgCharacterBase;

/**
 * 리젠(플레이어 사양 '자원 회복', 전투 시스템 'PP 리젠'): 끝나지 않는 주기 효과로, 주기마다 초당 회복량 × 주기만큼
 * 더합니다(UAgRegenCalculation). 초당 회복량은 효과마다 주인의 캐릭터 데이터에서 읽고, 음수면 소모합니다(달리기의 SP 소모).
 * 태그 조건이 맞지 않는 동안(예: 리젠 대기 태그가 있는 동안) 멈추고, 다시 맞으면 주기를 처음부터 셉니다.
 * 캐릭터마다 거는 리젠은 캐릭터 데이터의 시작할 때 거는 GameplayEffect 목록(Effects)에 있습니다.
 */
UCLASS(Abstract)
class UAgEffect_Regen : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UAgEffect_Regen();

	/** 주인의 초당 회복량(음수면 소모)을 캐릭터 데이터에서 읽습니다. */
	virtual float GetRatePerSecond(const AAgCharacterBase& Owner) const PURE_VIRTUAL(UAgEffect_Regen::GetRatePerSecond, return 0.f;);

protected:
	/** 회복하는 어트리뷰트와, 리젠이 도는 동안 주인에게 있어야 하는 태그와 없어야 하는 태그를 정합니다. */
	void SetupRegen(const FGameplayAttribute& Attribute, const FGameplayTagContainer& RequiredTags, const FGameplayTagContainer& IgnoredTags);
};

/** 가드 중이 아닐 때의 SP 회복 */
UCLASS()
class UAgEffect_SPRegen : public UAgEffect_Regen
{
	GENERATED_BODY()

public:
	UAgEffect_SPRegen();

	virtual float GetRatePerSecond(const AAgCharacterBase& Owner) const override;
};

/** 가드 중의 SP 회복 */
UCLASS()
class UAgEffect_SPRegenGuarding : public UAgEffect_Regen
{
	GENERATED_BODY()

public:
	UAgEffect_SPRegenGuarding();

	virtual float GetRatePerSecond(const AAgCharacterBase& Owner) const override;
};

/** PP 리젠 */
UCLASS()
class UAgEffect_PPRegen : public UAgEffect_Regen
{
	GENERATED_BODY()

public:
	UAgEffect_PPRegen();

	virtual float GetRatePerSecond(const AAgCharacterBase& Owner) const override;
};

/** 달리기의 SP 소모(초당). 달리기 태그가 있는 동안만 돕니다. */
UCLASS()
class UAgEffect_SprintSPCost : public UAgEffect_Regen
{
	GENERATED_BODY()

public:
	UAgEffect_SprintSPCost();

	virtual float GetRatePerSecond(const AAgCharacterBase& Owner) const override;
};
