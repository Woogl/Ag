// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "AgAttributeSet.generated.h"

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

	/** 이동 속도 (cm/s). The character movement's max walk speed follows it. */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FGameplayAttributeData MOV;
	ATTRIBUTE_ACCESSORS_BASIC(UAgAttributeSet, MOV)

protected:
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	/**
	 * 사망·그로기 판정(전투 시스템 '피격 처리 순서'): GE가 HP를 0 이하로 만들면 사망 이벤트를, PP를 잃을 수 있는 캐릭터의 PP를
	 * 0 이하로 만들면 그로기 이벤트를 보냅니다. 대미지 GE는 HP를 먼저 바꾸므로 사망이 먼저이고, 죽으면 그로기 어빌리티가 막힙니다.
	 */
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

private:
	/** Keeps every stat inside its range from 전투 시스템 '스탯' (resources between 0 and their max, the rest at least 0). */
	void ClampToRange(const FGameplayAttribute& Attribute, float& Value) const;
};
