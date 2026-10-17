// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "AgEffect_GroggyImmune.generated.h"

/**
 * 그로기 면역(플레이어 사양 '스탯'): 무한 지속으로 State.GroggyImmune 태그를 줍니다. 캐릭터 데이터의 시작할 때 거는
 * GameplayEffect 목록(Effects)에 두면, 그 캐릭터는 PP 감소와 그로기 진입 대상에서 제외됩니다.
 */
UCLASS()
class UAgEffect_GroggyImmune : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UAgEffect_GroggyImmune();
};
