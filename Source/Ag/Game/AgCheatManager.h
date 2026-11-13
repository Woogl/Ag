// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameFramework/CheatManager.h"
#include "AgCheatManager.generated.h"

class AAgPlayerCharacter;
class UAbilitySystemComponent;

enum class EAgCheatSwitch : uint8
{
	Invincible,
	NoCooldown,
	InfiniteMP,
	InfiniteUP,
};

UCLASS()
class UAgCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	UFUNCTION(Exec)
	void AgKillPlayer();

	UFUNCTION(Exec)
	void AgKillBoss();

	UFUNCTION(Exec)
	void AgKillEnemies();

	UFUNCTION(Exec)
	void AgGod(const FString& OnOff);

	UFUNCTION(Exec)
	void AgInvincible(const FString& OnOff);

	UFUNCTION(Exec)
	void AgSetHP(float Percent);

	UFUNCTION(Exec)
	void AgSetMP(float Percent);

	UFUNCTION(Exec)
	void AgSetUP(float Percent);

	UFUNCTION(Exec)
	void AgInfiniteMP(const FString& OnOff);

	UFUNCTION(Exec)
	void AgInfiniteUP(const FString& OnOff);

	UFUNCTION(Exec)
	void AgNoCooldown(const FString& OnOff);

	UFUNCTION(Exec)
	void AgTargetGroggy();

	UFUNCTION(Exec)
	void AgTargetPattern(FName Id);

	UFUNCTION(Exec)
	void AgTargetHP(float Percent);

	UFUNCTION(Exec)
	void AgStopAI();

	UFUNCTION(Exec)
	void AgStartAI();

	UFUNCTION(Exec)
	void AgIgnoreDeviceInput(const FString& OnOff);

	UFUNCTION(Exec)
	void AgPressKey(FName KeyName);

	UFUNCTION(Exec)
	void AgReleaseKey(FName KeyName);

	UFUNCTION(Exec)
	void AgMoveMouse(float X, float Y);

private:
	void RunSwitch(const TCHAR* Command, const FString& OnOff, EAgCheatSwitch Switch);

	bool IsSwitchOn(const UAbilitySystemComponent& ASC, EAgCheatSwitch Switch) const;

	void SetSwitch(AAgPlayerCharacter& Player, EAgCheatSwitch Switch, bool bOn);

	TMap<EAgCheatSwitch, FActiveGameplayEffectHandle> SwitchEffects;
};
