// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "AgEffect_Cooldown.generated.h"

class UAbilitySystemComponent;

/**
 * 행동(보스는 패턴)의 쿨다운. 태그를 주지 않고, 걸 때 넣은 출처(그 행동 스펙의 어빌리티 인스턴스)로 행동을 구분합니다.
 * 행동마다 태그를 두지 않으므로 행동을 더해도 태그가 늘지 않습니다. 시간은 Apply가 스펙에 넣습니다.
 */
UCLASS()
class UAgEffect_Cooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UAgEffect_Cooldown();

	/** ASC의 주인에게 Source 행동의 쿨다운을 Duration초(게임 시간) 동안 겁니다. 시간이 0 이하이면 걸지 않습니다. */
	static void Apply(UAbilitySystemComponent* ASC, const UObject* Source, float Duration);

	/** ASC의 주인에게 Source 행동의 쿨다운이 걸려 있는지 */
	static bool IsActive(const UAbilitySystemComponent* ASC, const UObject* Source);

	/** Source 행동의 쿨다운이 남은 시간과 전체 시간. 걸려 있지 않으면 둘 다 0입니다. */
	static void GetTimeRemaining(const UAbilitySystemComponent* ASC, const UObject* Source, float& OutRemaining, float& OutDuration);
};
