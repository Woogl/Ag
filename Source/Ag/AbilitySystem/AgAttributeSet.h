// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "AgAttributeSet.generated.h"

UCLASS()
class UAgAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FGameplayAttributeData HP;
	ATTRIBUTE_ACCESSORS_BASIC(UAgAttributeSet, HP)

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FGameplayAttributeData MaxHP;
	ATTRIBUTE_ACCESSORS_BASIC(UAgAttributeSet, MaxHP)

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FGameplayAttributeData SP;
	ATTRIBUTE_ACCESSORS_BASIC(UAgAttributeSet, SP)

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FGameplayAttributeData MaxSP;
	ATTRIBUTE_ACCESSORS_BASIC(UAgAttributeSet, MaxSP)

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FGameplayAttributeData MP;
	ATTRIBUTE_ACCESSORS_BASIC(UAgAttributeSet, MP)

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FGameplayAttributeData MaxMP;
	ATTRIBUTE_ACCESSORS_BASIC(UAgAttributeSet, MaxMP)

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FGameplayAttributeData UP;
	ATTRIBUTE_ACCESSORS_BASIC(UAgAttributeSet, UP)

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FGameplayAttributeData MaxUP;
	ATTRIBUTE_ACCESSORS_BASIC(UAgAttributeSet, MaxUP)

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FGameplayAttributeData PP;
	ATTRIBUTE_ACCESSORS_BASIC(UAgAttributeSet, PP)

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FGameplayAttributeData MaxPP;
	ATTRIBUTE_ACCESSORS_BASIC(UAgAttributeSet, MaxPP)

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FGameplayAttributeData ATK;
	ATTRIBUTE_ACCESSORS_BASIC(UAgAttributeSet, ATK)

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FGameplayAttributeData DEF;
	ATTRIBUTE_ACCESSORS_BASIC(UAgAttributeSet, DEF)

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FGameplayAttributeData MOV;
	ATTRIBUTE_ACCESSORS_BASIC(UAgAttributeSet, MOV)

protected:
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	/** 대미지 GE는 HP를 먼저 바꾸므로 사망 판정이 먼저이고, 죽으면 그로기 어빌리티가 막힙니다. */
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

private:
	void ClampToRange(const FGameplayAttribute& Attribute, float& Value) const;
};
