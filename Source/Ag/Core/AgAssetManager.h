// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "AgAssetManager.generated.h"

class FJsonValue;
class UAgAbilitySet;
class UAgCharacterData;
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

	AG_API const UAgMontageTimelineData& GetMontageTimelines() const;

	const UTargetingPreset* FindTargetingPreset(FName Id) const;

	/** 데이터를 다시 읽으면 바뀌므로 포인터는 그 함수 안에서만 씁니다. */
	const FAgAbilityData* FindAbility(FName Id) const;

	const TMap<FName, FAgProjectileDefinition>& GetProjectiles() const;

	const UGameplayEffect* FindEffect(FName Id) const;

	const UGameplayEffect* GetEffect(EAgEffect Effect) const;

	UAgCharacterData* FindCharacterData(FName Id) const;

	TArray<FName> GetCharacterDataIds(const UClass* DataClass) const;

#if WITH_EDITOR
	AG_API void ReloadJsonData();
	virtual void ModifyCook(TConstArrayView<const ITargetPlatform*> TargetPlatforms, TArray<FName>& PackagesToCook, TArray<FName>& PackagesToNeverCook) override;
#endif

protected:
	virtual void StartInitialLoading() override;

private:
	bool LoadJsonData();

	bool ReadJson(const FString& Path, TSharedPtr<FJsonValue> Value, const FProperty& Property, void* Out);

	TMap<FName, TSet<FName>> JsonIds;

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

	/** 항목의 형식을 데이터 클래스가 정하므로 ID와 클래스의 짝은 생성자에 적습니다. */
	UPROPERTY()
	TMap<FName, TObjectPtr<UAgCharacterData>> Characters;
};
