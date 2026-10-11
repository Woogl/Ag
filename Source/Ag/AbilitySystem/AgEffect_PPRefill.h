// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "AgEffect_PPRefill.generated.h"

/** PP를 최대(MaxPP)로 채웁니다(그로기가 끝날 때, 페이즈 전환). */
UCLASS()
class UAgEffect_PPRefill : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UAgEffect_PPRefill();
};
