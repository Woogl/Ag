// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "AgEffect_InfinitePP.generated.h"

/**
 * PP 무한(플레이어 사양 '스탯'): 무한 지속으로 State.InfinitePP 태그를 줍니다. 캐릭터 데이터의 시작할 때 거는
 * GameplayEffect 목록(Effects)에 두면, 그 캐릭터는 PP를 잃지 않아 그로기에 빠지지 않습니다(AAgCharacterBase::CanLosePP).
 */
UCLASS()
class UAgEffect_InfinitePP : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UAgEffect_InfinitePP();
};
