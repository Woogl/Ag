// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AgCharacterBase.generated.h"

class UAgCharacterData;
class AAgCharacterBase;

DECLARE_MULTICAST_DELEGATE_OneParam(FAgCharacterDiedSignature, AAgCharacterBase* /*DeadCharacter*/);

/**
 * Base for the player and the boss.
 * Owns the character data asset and the death handling from 전투 시스템 '사망 처리'.
 */
UCLASS(Abstract)
class AAgCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	AAgCharacterBase();

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
	virtual void BeginPlay() override;

	/** 레벨의 Kill Z 아래로 떨어진 캐릭터는 사망 처리합니다. (전투 시스템 '사망') */
	virtual void FellOutOfWorld(const UDamageType& DamageType) override;

	/** DA_Player or DA_Boss. */
	UPROPERTY(EditDefaultsOnly, Category = "Data")
	TObjectPtr<UAgCharacterData> CharacterData;

private:
	bool bDead = false;
};
