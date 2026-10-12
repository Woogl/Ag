// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "AgEffect_PatternCooldown.generated.h"

class UAbilitySystemComponent;

/**
 * 보스 패턴의 쿨다운. 태그를 주지 않고, 걸 때 넣은 출처(그 패턴 스펙의 어빌리티 인스턴스)로 패턴을 구분합니다.
 * 패턴마다 태그를 두지 않으므로 패턴을 더해도 태그가 늘지 않습니다. 시간은 Apply가 스펙에 넣습니다.
 */
UCLASS()
class UAgEffect_PatternCooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UAgEffect_PatternCooldown();

	/** ASC의 주인에게 Source 패턴의 쿨다운을 Duration초(게임 시간) 동안 겁니다. 시간이 0 이하이면 걸지 않습니다. */
	static void Apply(UAbilitySystemComponent* ASC, const UObject* Source, float Duration);

	/** ASC의 주인에게 Source 패턴의 쿨다운이 걸려 있는지 */
	static bool IsActive(const UAbilitySystemComponent* ASC, const UObject* Source);
};
