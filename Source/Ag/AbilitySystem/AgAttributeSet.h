// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "AgAttributeSet.generated.h"

#define AG_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * The stats of 전투 시스템 '스탯', named as in the spec.
 * A resource a character doesn't use (the boss's SP, MP, UP) stays at 0.
 */
UCLASS()
class UAgAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FGameplayAttributeData HP;
	AG_ATTRIBUTE_ACCESSORS(UAgAttributeSet, HP)

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FGameplayAttributeData MaxHP;
	AG_ATTRIBUTE_ACCESSORS(UAgAttributeSet, MaxHP)

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FGameplayAttributeData SP;
	AG_ATTRIBUTE_ACCESSORS(UAgAttributeSet, SP)

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FGameplayAttributeData MaxSP;
	AG_ATTRIBUTE_ACCESSORS(UAgAttributeSet, MaxSP)

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FGameplayAttributeData MP;
	AG_ATTRIBUTE_ACCESSORS(UAgAttributeSet, MP)

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FGameplayAttributeData MaxMP;
	AG_ATTRIBUTE_ACCESSORS(UAgAttributeSet, MaxMP)

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FGameplayAttributeData UP;
	AG_ATTRIBUTE_ACCESSORS(UAgAttributeSet, UP)

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FGameplayAttributeData MaxUP;
	AG_ATTRIBUTE_ACCESSORS(UAgAttributeSet, MaxUP)

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FGameplayAttributeData PP;
	AG_ATTRIBUTE_ACCESSORS(UAgAttributeSet, PP)

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FGameplayAttributeData MaxPP;
	AG_ATTRIBUTE_ACCESSORS(UAgAttributeSet, MaxPP)

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FGameplayAttributeData ATK;
	AG_ATTRIBUTE_ACCESSORS(UAgAttributeSet, ATK)

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FGameplayAttributeData DEF;
	AG_ATTRIBUTE_ACCESSORS(UAgAttributeSet, DEF)

	/** 이동 속도 (cm/s). The character movement's max walk speed follows it. */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FGameplayAttributeData MOV;
	AG_ATTRIBUTE_ACCESSORS(UAgAttributeSet, MOV)

protected:
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

private:
	/** Keeps every stat inside its range from 전투 시스템 '스탯' (resources between 0 and their max, the rest at least 0). */
	void ClampToRange(const FGameplayAttribute& Attribute, float& Value) const;
};
