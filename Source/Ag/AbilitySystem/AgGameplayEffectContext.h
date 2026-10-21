// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/AgHitReward.h"
#include "GameplayEffectTypes.h"
#include "AgGameplayEffectContext.generated.h"

struct FAgHitRewardState
{
	FAgHitReward Amounts;
	bool bGranted = false;

	static TSharedPtr<FAgHitRewardState> Make(const TOptional<FAgHitReward>& Reward)
	{
		return Reward ? MakeShared<FAgHitRewardState>(FAgHitRewardState{*Reward}) : TSharedPtr<FAgHitRewardState>();
	}
};

USTRUCT()
struct FAgGameplayEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()

	virtual UScriptStruct* GetScriptStruct() const override;
	virtual FAgGameplayEffectContext* Duplicate() const override;

	static TSharedPtr<FAgHitRewardState> GetHitReward(const FGameplayEffectContextHandle& Context);

	// 이전 공격의 지연 적중과 대상별 컨텍스트 복제도 원래 충전 단위의 지급 여부를 공유합니다.
	TSharedPtr<FAgHitRewardState> HitReward;
};

template <>
struct TStructOpsTypeTraits<FAgGameplayEffectContext> : public TStructOpsTypeTraitsBase2<FAgGameplayEffectContext>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};
