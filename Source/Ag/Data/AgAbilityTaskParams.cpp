// Copyright Woogle. All Rights Reserved.

#include "Data/AgAbilityTaskParams.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgJsonRead.h"
#include "GameplayEffect.h"

namespace
{
	/** 자원 효과의 SetByCaller 셋을 모두 넣습니다. 넣지 않은 값은 엔진이 오류로 남기므로, 바꾸지 않는 자원도 0을 넣습니다. */
	void SetResourceAmounts(FGameplayEffectSpec& Spec, float SP, float MP, float UP)
	{
		Spec.SetSetByCallerMagnitude(AgGameplayTags::SetByCaller_SP, SP);
		Spec.SetSetByCallerMagnitude(AgGameplayTags::SetByCaller_MP, MP);
		Spec.SetSetByCallerMagnitude(AgGameplayTags::SetByCaller_UP, UP);
	}

	void SetResourceAmount(FGameplayEffectSpec& Spec, EAgResource Resource, float Amount)
	{
		SetResourceAmounts(Spec, Resource == EAgResource::SP ? Amount : 0.f, Resource == EAgResource::MP ? Amount : 0.f,
		    Resource == EAgResource::UP ? Amount : 0.f);
	}
}

void FAgResourceCostParams::AddToSpec(FGameplayEffectSpec& Spec) const
{
	SetResourceAmount(Spec, Resource, -Amount);
}

void FAgDrainResourceParams::AddToSpec(FGameplayEffectSpec& Spec) const
{
	SetResourceAmount(Spec, Resource, -AmountPerSecond * Spec.GetPeriod());
}

void FAgDrainResourceParams::Validate(const FJsonObject& Json, const FString& Path, TArray<FString>& Errors)
{
	if (AmountPerSecond <= 0.f)
	{
		Errors.Add(Path + TEXT(".AmountPerSecond: must be positive"));
	}
}

void FAgTimedTagParams::Validate(const FJsonObject& Json, const FString& Path, TArray<FString>& Errors)
{
	if (Duration <= 0.f)
	{
		Errors.Add(Path + TEXT(".Duration: must be positive"));
	}
}

void FAgGrantResourcesParams::AddToSpec(FGameplayEffectSpec& Spec) const
{
	SetResourceAmounts(Spec, SP, MP, UP);
}

void FAgGrantResourcesParams::Validate(const FJsonObject& Json, const FString& Path, TArray<FString>& Errors)
{
	if (SP + MP + UP <= 0.f)
	{
		Errors.Add(Path + TEXT(": reward must be positive"));
	}
}

void FAgSlowMotionParams::Validate(const FJsonObject& Json, const FString& Path, TArray<FString>& Errors)
{
	if (Speed <= 0.f || Speed > 1.f || RealDuration <= 0.f)
	{
		Errors.Add(Path + TEXT(": invalid speed or duration"));
	}
}

void FAgLaunchVerticalParams::GetFieldRules(FAgJsonFieldRules& Rules) const
{
	Rules.Signed.Add(GET_MEMBER_NAME_CHECKED(FAgLaunchVerticalParams, VelocityZ));
}

void FAgLaunchToTargetParams::Validate(const FJsonObject& Json, const FString& Path, TArray<FString>& Errors)
{
	if (AirTime <= 0.f)
	{
		Errors.Add(Path + TEXT(".AirTime: must be positive"));
	}
}

void FAgGameplayEventParams::Validate(const FJsonObject& Json, const FString& Path, TArray<FString>& Errors)
{
	if (!Event.ToString().StartsWith(TEXT("Event.")))
	{
		Errors.Add(Path + TEXT(".Event: expected a tag under Event"));
	}
}
