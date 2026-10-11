// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "AgAssetManager.generated.h"

class UAgCameraData;
class UAgCombatRules;
struct FPropertyChangedEvent;

/**
 * 프로젝트의 에셋 매니저(구현 계획서 '데이터' 읽는 방법). DefaultEngine.ini의 AssetManagerClassName으로 지정합니다.
 * 프로젝트 Data 폴더의 JSON 데이터 파일로 데이터 객체의 Transient 속성을 채우고, 공통 데이터 객체를 들고 있습니다.
 * 파일 하나가 객체 하나를 채웁니다. 그 속성은 모두 파일에 있어야 하고, 다른 키는 없어야 합니다.
 * 키는 속성 이름이고, 태그는 태그 이름으로, 열거형은 값 이름으로 적습니다.
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
	 * FileName으로 Target의 Transient 속성을 채웁니다. 오류(문법, 모르는 키나 빠진 키, 잘못된 형식, 없는 태그나 클래스, 정수가 아닌 값,
	 * 범위 밖의 값)가 하나라도 있으면 모든 값을 그대로 두고, 오류는 로그와 화면에 알립니다. 범위는 에디터에서만 검사합니다.
	 */
	bool LoadJsonValues(UObject& Target, const FString& FileName);

	/** CombatRules.json의 값 */
	const UAgCombatRules& GetCombatRules() const;

	/** Camera.json의 값 */
	const UAgCameraData& GetCameraData() const;

#if WITH_EDITOR
	/** 이 파일들로 채운 객체를 모두 다시 채우고, 그런 객체가 있었으면 OnJsonValuesReloaded를 한 번 알립니다. Data 폴더 감시가 부릅니다. */
	AG_API void ReloadJsonFiles(const TArray<FString>& FileNames);

	/** 데이터 파일을 다시 읽은 뒤. 스폰할 때 한 번 넣는 값을 다시 넣게 합니다. */
	FSimpleMulticastDelegate OnJsonValuesReloaded;
#endif

protected:
	virtual void StartInitialLoading() override;

private:
	/** 프로젝트 설정 'Ag'가 가리키는 공통 JSON 파일을 공통 데이터 객체에 읽습니다. */
	void LoadCommonData();

#if WITH_EDITOR
	/** 프로젝트 설정 'Ag'에서 공통 JSON 파일을 바꾸면 다시 읽습니다. */
	void HandleSettingsChanged(UObject* Settings, FPropertyChangedEvent& PropertyChangedEvent);
#endif

	UPROPERTY()
	TObjectPtr<UAgCombatRules> CombatRules;

	UPROPERTY()
	TObjectPtr<UAgCameraData> CameraData;

#if WITH_EDITOR
	/** 객체마다 채운 파일 (ReloadJsonFiles용) */
	TMap<TWeakObjectPtr<UObject>, FString> LoadedFiles;
#endif
};
