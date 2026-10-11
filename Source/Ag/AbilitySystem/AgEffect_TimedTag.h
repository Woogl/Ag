// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "AgEffect_TimedTag.generated.h"

class UAbilitySystemComponent;

/**
 * 주인에게 정한 시간 동안 태그 하나를 줍니다(쿨다운, 리젠 대기, 패리 구간 등). 시간과 태그는 Apply가 스펙에 넣습니다.
 * 효과 타이머는 게임 시간으로 흘러 히트스톱 중에도 계속 셉니다(전투 시스템 '히트스톱 중의 시간').
 */
UCLASS()
class UAgEffect_TimedTag : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UAgEffect_TimedTag();

	/** ASC의 주인에게 Tag를 Duration초(게임 시간) 동안 줍니다. 시간이 0 이하이거나 태그가 없으면 걸지 않습니다. */
	static FActiveGameplayEffectHandle Apply(UAbilitySystemComponent* ASC, const FGameplayTag& Tag, float Duration);
};
