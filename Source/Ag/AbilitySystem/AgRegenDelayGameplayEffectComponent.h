// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/AgEffectData.h"
#include "GameplayEffectComponent.h"
#include "AgRegenDelayGameplayEffectComponent.generated.h"

class UAbilitySystemComponent;
struct FGameplayEffectRemovalInfo;
struct FOnAttributeChangeData;

UCLASS()
class UAgRegenDelayGameplayEffectComponent : public UGameplayEffectComponent
{
	GENERATED_BODY()

public:
	virtual bool OnActiveGameplayEffectAdded(FActiveGameplayEffectsContainer& GEContainer, FActiveGameplayEffect& ActiveGE) const override;

	UPROPERTY()
	FAgRegenDelay Delay;

private:
	void HandleAttributeChanged(const FOnAttributeChangeData& Data, UAbilitySystemComponent* ASC) const;

	void HandleEffectRemoved(const FGameplayEffectRemovalInfo& RemovalInfo, UAbilitySystemComponent* ASC, TArray<TTuple<FGameplayAttribute, FDelegateHandle>> Bindings) const;
};
