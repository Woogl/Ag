// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "AgGameplayEffectContext.generated.h"

/** 피해 Task 하나(한 타)가 만든 피해가 모두 함께 가리킵니다. 생성 액터는 그 액터가 낸 피해 전체가 한 상태를 가리킵니다. */
struct FAgHitState
{
	bool bLanded = false;
};

USTRUCT()
struct FAgGameplayEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()

	virtual UScriptStruct* GetScriptStruct() const override;
	virtual FAgGameplayEffectContext* Duplicate() const override;

	static TSharedPtr<FAgHitState> GetHitState(const FGameplayEffectContextHandle& Context);

	// 이전 공격의 지연 적중과 대상별 컨텍스트 복제도 같은 타의 첫 적중 여부를 공유합니다.
	TSharedPtr<FAgHitState> HitState;
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
