// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "AgAssetManager.generated.h"

class UAgCameraData;
class UAgCombatRules;
class UAgMontageTimelineData;

/**
 * 프로젝트의 에셋 매니저(구현 계획서 'JSON과 DataAsset' 읽는 방법). DefaultEngine.ini의 AssetManagerClassName으로 지정합니다.
 * 프로젝트 Data 폴더의 JSON 데이터 파일로 데이터 객체의 Transient 속성을 채우고, 공통 데이터 객체를 들고 있습니다.
 * 파일 하나가 객체 하나를 채웁니다. 그 속성은 모두 파일에 있어야 하고, 다른 키는 없어야 합니다.
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

	/**
	 * FileName으로 Target의 Transient 속성을 채웁니다. 데이터 검증은 에디터·쿡에서만 수행합니다.
	 * 패키징된 게임은 검증한 JSON을 읽고 변환·로드합니다. 모든 빌드에서 로딩에 실패하면 기존 값을 유지하고
	 * 오류를 로그와 화면에 알립니다(구현 계획서 'JSON과 DataAsset' 검증 시점).
	 */
	bool LoadJsonValues(UObject& Target, const FString& FileName);

	/** CombatRules.json의 값 */
	const UAgCombatRules& GetCombatRules() const;

	/** Camera.json의 값 */
	const UAgCameraData& GetCameraData() const;

	/** MontageTimelines.json의 값(몽타주 시간표) */
	AG_API const UAgMontageTimelineData& GetMontageTimelines() const;

#if WITH_EDITOR
	/** 이 파일들로 채운 객체를 모두 다시 채웁니다. Data 폴더 감시가 부릅니다. */
	AG_API void ReloadJsonFiles(const TArray<FString>& FileNames);
	virtual void ModifyCook(TConstArrayView<const ITargetPlatform*> TargetPlatforms, TArray<FName>& PackagesToCook,
	    TArray<FName>& PackagesToNeverCook) override;
#endif

protected:
	virtual void StartInitialLoading() override;

private:
	/** 검증 중인 객체도 로딩 도중 GC가 따라갈 수 있게 보관합니다. 성공한 값만 대상에 복사합니다. */
	UPROPERTY()
	TArray<TObjectPtr<UObject>> JsonLoadTargets;

	UPROPERTY()
	TObjectPtr<UAgCombatRules> CombatRules;

	UPROPERTY()
	TObjectPtr<UAgCameraData> CameraData;

	UPROPERTY()
	TObjectPtr<UAgMontageTimelineData> MontageTimelines;

#if WITH_EDITOR
	/** 객체마다 채운 파일 (ReloadJsonFiles용) */
	TMap<TWeakObjectPtr<UObject>, FString> LoadedFiles;
#endif
};
