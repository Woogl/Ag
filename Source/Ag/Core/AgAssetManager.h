// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "AgAssetManager.generated.h"

class UAgAbilitySet;
class UAgCameraData;
class UAgCharacterData;
class UAgCombatRules;
class UAgEffectSet;
class UAgMontageTimelineData;
class UAgProjectileSet;
class UGameplayEffect;
enum class EAgEffect : uint8;
struct FAgAbilityData;
struct FAgProjectileDefinition;

/**
 * 프로젝트의 에셋 매니저(구현 계획서 'JSON 데이터'). DefaultEngine.ini의 AssetManagerClassName으로 지정합니다.
 * 프로젝트 Data 폴더의 JSON 데이터 파일을 모두 한 묶음으로 읽어, 데이터 객체의 Transient 속성을 채우고 들고 있습니다.
 * 리플렉션으로 읽는 객체는 그 속성이 모두 파일에 있어야 하고, 다른 키는 없어야 합니다.
 * 키는 속성 이름이고, 태그·열거형은 이름으로, 에셋·클래스는 전체 객체 경로로 적습니다.
 */
UCLASS()
class UAgAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	UAgAssetManager();

	AG_API static UAgAssetManager& Get();

	/** JSON 데이터 파일이 있는 프로젝트의 Data 폴더 */
	AG_API static FString GetJsonFolder();

	/** CombatRules.json의 값 */
	const UAgCombatRules& GetCombatRules() const;

	/** Camera.json의 값 */
	const UAgCameraData& GetCameraData() const;

	/** MontageTimelines.json의 값(몽타주 시간표) */
	AG_API const UAgMontageTimelineData& GetMontageTimelines() const;

	/** Abilities.json의 어빌리티 정의. 없으면 null입니다. 데이터를 다시 읽으면 바뀌므로 포인터는 그 함수 안에서만 씁니다. */
	const FAgAbilityData* FindAbility(FName Id) const;

	/** Projectiles.json의 투사체 정의 */
	const TMap<FName, FAgProjectileDefinition>& GetProjectiles() const;

	/** Effects.json의 효과. 없으면 null입니다. */
	const UGameplayEffect* FindEffect(FName Id) const;

	/** 코드가 거는 효과 */
	const UGameplayEffect* GetEffect(EAgEffect Effect) const;

	/** Characters.json의 캐릭터 데이터. 없으면 null입니다. */
	UAgCharacterData* FindCharacterData(FName Id) const;

	/** 이 데이터 클래스의 캐릭터 ID. 캐릭터 블루프린트의 선택 목록입니다. */
	TArray<FName> GetCharacterDataIds(const UClass* DataClass) const;

#if WITH_EDITOR
	/** Data 폴더의 JSON을 모두 다시 읽습니다. Data 폴더 감시가 부릅니다. */
	AG_API void ReloadJsonData();
	virtual void ModifyCook(TConstArrayView<const ITargetPlatform*> TargetPlatforms, TArray<FName>& PackagesToCook, TArray<FName>& PackagesToNeverCook) override;
#endif

protected:
	virtual void StartInitialLoading() override;

private:
	/**
	 * 모든 JSON 파일을 새 객체에 읽고 파일 사이의 참조까지 검사한 뒤, 모두 통과하면 지금 객체에 한꺼번에 복사합니다.
	 * 데이터 검증은 에디터·쿡에서만 합니다. 실패하면 기존 값을 유지하고 오류를 로그와 화면에 알립니다(구현 계획서 'JSON 데이터' 검증 시점).
	 */
	bool LoadJsonData();

	/** 검증 중인 객체도 로딩 도중 GC가 따라갈 수 있게 보관합니다. */
	UPROPERTY()
	TArray<TObjectPtr<UObject>> JsonLoadTargets;

	UPROPERTY()
	TObjectPtr<UAgCombatRules> CombatRules;

	UPROPERTY()
	TObjectPtr<UAgCameraData> CameraData;

	UPROPERTY()
	TObjectPtr<UAgEffectSet> Effects;

	UPROPERTY()
	TObjectPtr<UAgProjectileSet> Projectiles;

	UPROPERTY()
	TObjectPtr<UAgMontageTimelineData> MontageTimelines;

	UPROPERTY()
	TObjectPtr<UAgAbilitySet> Abilities;

	/** Characters.json의 캐릭터 ID마다 그 데이터. 항목의 형식을 데이터 클래스가 정하므로 ID와 클래스의 짝은 생성자에 적습니다. */
	UPROPERTY()
	TMap<FName, TObjectPtr<UAgCharacterData>> Characters;

	/** 읽기를 시도할 때마다 늘립니다. 효과 GE 객체의 이름에 붙여 읽을 때마다 새 객체를 만듭니다. */
	int32 DataVersion = 0;
};
