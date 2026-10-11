// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "AgEffect_Kill.generated.h"

/** HP를 0으로 만듭니다(Kill Z 낙하, 테스트용 콘솔 명령). 사망 판정은 UAgAttributeSet이 합니다. */
UCLASS()
class UAgEffect_Kill : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UAgEffect_Kill();
};
