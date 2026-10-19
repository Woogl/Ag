// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AgEffectData.generated.h"

class FJsonObject;
class UGameplayEffect;

UENUM()
enum class EAgEffect : uint8
{
	Damage,
	Resource,
	Cooldown,
	Hitstop,
	SPRegenDelay,
	SPRegenDelayEmpty,
	PPRegenDelay,
	Kill,
	PPRefill,
	ParryPPLoss,
	ParryReward,
	PerfectDodgeReward,
	DodgeCounterChance,
};

UCLASS()
class UAgEffectSet : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, VisibleAnywhere, Category = "Effects")
	TMap<FName, TObjectPtr<UGameplayEffect>> Effects;
};

namespace AgEffectData
{
	FName Id(EAgEffect Effect);

	/**
	 * 객체 이름에 Version을 붙여 읽을 때마다 새 객체를 만듭니다. 같은 이름으로 다시 만들면 엔진이 기존 객체를
	 * 그 자리에서 부수고 다시 만들어, 걸려 있는 효과가 망가지기 때문입니다.
	 */
	bool Parse(const FJsonObject& Json, UObject& Outer, int32 Version, TMap<FName, TObjectPtr<UGameplayEffect>>& Out, TArray<FString>& Errors);
}
