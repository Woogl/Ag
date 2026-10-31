// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "AgAssetManager.generated.h"

class UAgAbilitySet;
class UAgCharacterData;
class UAgCharacterSet;
class UAgCombatRules;
class UAgEffectSet;
class UAgMontageTimelineData;
class UAgProjectileSet;
class UAgTargetingPresetSet;
class UGameplayEffect;
class UTargetingPreset;
enum class EAgEffect : uint8;
struct FAgAbilityData;
struct FAgProjectileDefinition;

UCLASS()
class UAgAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	UAgAssetManager();

	AG_API static UAgAssetManager& Get();

	AG_API static FString GetJsonFolder();

	const UAgCombatRules& GetCombatRules() const;

	const UAgMontageTimelineData& GetMontageTimelines() const;

	const UTargetingPreset* FindTargetingPreset(FName Id) const;

	/** 데이터를 다시 읽으면 바뀌므로 포인터는 그 함수 안에서만 씁니다. */
	const FAgAbilityData* FindAbility(FName Id) const;

	const TMap<FName, FAgProjectileDefinition>& GetProjectiles() const;

	const UGameplayEffect* FindEffect(FName Id) const;

	const UGameplayEffect* GetEffect(EAgEffect Effect) const;

	UAgCharacterData* FindCharacterData(FName Id) const;

	TArray<FName> GetCharacterDataIds() const;

#if WITH_EDITOR
	AG_API void ReloadJsonData();
	FSimpleMulticastDelegate OnJsonDataReloaded;
	virtual void ModifyCook(TConstArrayView<const ITargetPlatform*> TargetPlatforms, TArray<FName>& PackagesToCook, TArray<FName>& PackagesToNeverCook) override;
#endif

protected:
	virtual void StartInitialLoading() override;

private:
	bool LoadJsonData(bool bInitialLoad);

	bool bJsonLoadFailed = false;

	// JSON 파일을 담는 속성은 .cpp의 JsonFiles 목록이 이름으로 찾아 만들고 채웁니다.
	UPROPERTY()
	TObjectPtr<UAgCombatRules> CombatRules;

	UPROPERTY()
	TObjectPtr<UAgEffectSet> Effects;

	UPROPERTY(Transient)
	TMap<EAgEffect, TObjectPtr<UGameplayEffect>> NativeEffects;

	UPROPERTY()
	TObjectPtr<UAgProjectileSet> Projectiles;

	UPROPERTY()
	TObjectPtr<UAgMontageTimelineData> MontageTimelines;

	UPROPERTY()
	TObjectPtr<UAgTargetingPresetSet> TargetingPresets;

	UPROPERTY()
	TObjectPtr<UAgAbilitySet> Abilities;

	UPROPERTY()
	TObjectPtr<UAgCharacterSet> Characters;
};
