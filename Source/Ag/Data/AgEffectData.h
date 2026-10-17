// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AgEffectData.generated.h"

class FJsonObject;
class UGameplayEffect;

/**
 * 코드가 거는 효과. 이름이 Effects.json의 효과 ID이고, 로더가 코드가 넘기는 값을 받을 수 있는 모양인지 검사합니다
 * (구현 계획서 'JSON 데이터').
 */
UENUM()
enum class EAgEffect : uint8
{
	Damage,
	Resource,
	ResourceDrain,
	Cooldown,
	TimedTag,
	Hitstop,
	SPRegenDelay,
	SPRegenDelayEmpty,
	PPRegenDelay,
	Kill,
	PPRefill,
	ParryPPLoss,
};

/** Effects.json의 값: 효과 ID마다 읽을 때 만든 GameplayEffect */
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
	/** 코드가 거는 효과의 ID */
	FName Id(EAgEffect Effect);

	/**
	 * 효과 정의를 읽어 Outer 아래에 GE 객체를 만듭니다. 객체 이름에 Version을 붙여 읽을 때마다 새 객체를 만듭니다. 같은 이름으로
	 * 다시 만들면 엔진이 기존 객체를 그 자리에서 부수고 다시 만들어, 걸려 있는 효과가 망가지기 때문입니다. 실패하면 Out을 바꾸지 않습니다.
	 */
	bool Parse(
	    const FJsonObject& Json, UObject& Outer, int32 Version, TMap<FName, TObjectPtr<UGameplayEffect>>& Out, TArray<FString>& Errors);
}
