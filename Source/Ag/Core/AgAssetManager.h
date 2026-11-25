// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/AgJsonReader.h"
#include "Engine/AssetManager.h"
#include "AgAssetManager.generated.h"

class UAgJsonData;

/**
 * DesignData의 JSON을 파싱해 두고, UAgJsonData의 하위 클래스를 찾아 읽고 다시 읽고 쿡에 전달합니다. 파일마다의 내용과 조회는
 * 그 데이터 클래스가 맡으므로, 여기서는 게임 데이터의 종류를 알지 못합니다.
 */
UCLASS()
class UAgAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	AG_API static UAgAssetManager& Get();

	AG_API static FString GetJsonFolder();

	/** 리더에 넘기는 파일 경로이자 오류에 적는 위치입니다. */
	static FString GetJsonPath(const TCHAR* FileName);

	/** 게임을 켤 때 읽어 늘 들고 있는 데이터입니다. */
	template <typename T>
	const T& GetData() const
	{
		return *CastChecked<T>(FindData(*T::StaticClass()));
	}

	/**
	 * 쓰는 쪽이 붙잡는 동안만 메모리에 두는 데이터(UAgJsonData::IsAlwaysLoaded가 false)를 읽습니다. 같은 Key로 읽은 객체가
	 * 살아 있으면 그 객체를 돌려줍니다. 받은 쪽이 UPROPERTY로 붙잡고 있는 동안 그 데이터가 가리키는 에셋이 메모리에 남고,
	 * 아무도 붙잡지 않으면 가비지 컬렉션이 객체와 에셋을 함께 내립니다.
	 */
	template <typename T>
	T* LoadData(FName Key = NAME_None)
	{
		return CastChecked<T>(LoadData(*T::StaticClass(), Key));
	}

#if WITH_EDITOR
	AG_API void ReloadJsonData();
	FSimpleMulticastDelegate OnJsonDataReloaded;
	virtual void ModifyCook(TConstArrayView<const ITargetPlatform*> TargetPlatforms, TArray<FName>& PackagesToCook, TArray<FName>& PackagesToNeverCook) override;
#endif

protected:
	virtual void StartInitialLoading() override;

private:
	UAgJsonData* FindData(const UClass& Class) const;

	UAgJsonData* LoadData(const UClass& Class, FName Key);

	bool LoadJsonData(bool bInitialLoad);

	bool bJsonLoadFailed = false;

	/** DesignData의 모든 파일을 파싱한 표입니다. 에셋을 가리키는 값도 글자라, 들고 있어도 에셋을 읽지 않습니다. */
	FAgJsonDocuments JsonDocuments;

	/** 게임을 켤 때 읽어 늘 들고 있는 데이터입니다. 클래스마다 하나입니다. */
	UPROPERTY()
	TArray<TObjectPtr<UAgJsonData>> PersistentData;

	/** 쓰는 쪽이 붙잡고 있는 데이터를 클래스와 Key로 찾습니다. 여기서 붙잡으면 가비지 컬렉션이 내리지 못하므로 약한 참조로 둡니다. */
	TMap<TPair<const UClass*, FName>, TWeakObjectPtr<UAgJsonData>> LoadedData;
};
