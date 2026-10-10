// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "AgCharacterBase.generated.h"

class UAbilitySystemComponent;
class UAgAttributeSet;
class UAgCharacterData;
class AAgCharacterBase;
struct FOnAttributeChangeData;

DECLARE_MULTICAST_DELEGATE_OneParam(FAgCharacterDiedSignature, AAgCharacterBase* /*DeadCharacter*/);

/**
 * Base for the player and the boss.
 * Owns the ability system and stats, the weapon, the character data asset and the death handling from 전투 시스템 '사망 처리'.
 */
UCLASS(Abstract)
class AAgCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AAgCharacterBase();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UAgAttributeSet* GetAttributeSet() const { return AttributeSet; }

	UAgCharacterData* GetCharacterData() const { return CharacterData; }

	bool IsDead() const { return bDead; }

	/**
	 * 사망 처리: stops input and movement, turns off character-to-character collision,
	 * switches to ragdoll pushed along HitDirection, then broadcasts OnDied.
	 */
	virtual void Die(const FVector& HitDirection);

	/** Broadcast once when this character dies. */
	FAgCharacterDiedSignature OnDied;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;

	/** 레벨의 Kill Z 아래로 떨어진 캐릭터는 사망 처리합니다. (전투 시스템 '사망') */
	virtual void FellOutOfWorld(const UDamageType& DamageType) override;

	/** DA_Player or DA_Boss. */
	UPROPERTY(EditDefaultsOnly, Category = "Data")
	TObjectPtr<UAgCharacterData> CharacterData;

private:
	/** Sets the stats to the starting values in the character data. */
	void InitializeStats();

	/** Puts the character data's weapon mesh on the matching component and attaches it to the weapon socket. */
	void ApplyWeapon();

	void HandleMOVChanged(const FOnAttributeChangeData& Data);

	UPROPERTY(VisibleAnywhere, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UAgAttributeSet> AttributeSet;

	/** Holds the weapon when it is a static mesh (katana). */
	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	TObjectPtr<UStaticMeshComponent> StaticWeapon;

	/** Holds the weapon when it is a skeletal mesh (halberd). */
	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	TObjectPtr<USkeletalMeshComponent> SkeletalWeapon;

	bool bDead = false;
};
