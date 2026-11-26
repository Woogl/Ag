// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/AgAssetManager.h"
#include "UObject/Object.h"
#include "AgJsonData.generated.h"

class FAgJsonReader;

/**
 * DesignData의 JSON 파일 하나로 값을 채우는 데이터 객체의 공통 부모입니다. 에셋 매니저는 이 클래스만 알고, 하위 클래스를 찾아
 * 읽고 다시 읽고 쿡에 전달합니다. 파일을 더하려면 하위 클래스를 만듭니다.
 */
UCLASS(Abstract)
class UAgJsonData : public UObject
{
	GENERATED_BODY()

public:
	/** 값을 받는 파일의 이름입니다(예: `Effects.json`). */
	virtual const TCHAR* GetFileName() const PURE_VIRTUAL(UAgJsonData::GetFileName, return TEXT(""););

	/**
	 * 에셋 매니저가 게임 내내 들고 있는 데이터인지 돌려줍니다. 기본은 false로, 쓰는 쪽이 UAgAssetManager::LoadData로 읽어
	 * 붙잡는 동안만 메모리에 있습니다. 에셋을 가리키지 않고 어디서나 조회하는 표만 true로 둡니다. true인 데이터가 가리키는
	 * 에셋은 내려가지 않습니다.
	 */
	virtual bool IsAlwaysLoaded() const
	{
		return false;
	}

	/**
	 * 파싱해 둔 파일에서 값을 읽습니다. 기본은 파일의 최상위 키를 이 객체의 필드로 읽습니다. Key는 LoadData에 준 값으로, 파일의
	 * 일부만 읽는 데이터가 읽을 항목을 정합니다. 검사와 쿡은 None을 주어 전체를 읽습니다.
	 */
	virtual bool ReadJson(FAgJsonReader& Reader, FName Key);

	/** 새로 읽은 객체의 값을 받습니다. 이 객체를 가리키는 쪽이 그대로 새 값을 보도록 객체를 바꾸지 않고 값만 복사합니다. */
	virtual void CopyJson(const UAgJsonData& Loaded);

protected:
	/** JSON으로 채우는 필드만 복사합니다. */
	static void CopyJsonFields(const UObject& From, UObject& To);

	/** 리더에 넘기는 파일 경로이자 오류에 적는 위치입니다. */
	FString GetJsonPath() const;

	/** 파일 전체를 이 객체의 그 속성 하나에 읽습니다. ID마다 항목을 두는 파일에 씁니다. */
	bool ReadJsonInto(FAgJsonReader& Reader, FName PropertyName);

	/** 이름은 GET_MEMBER_NAME_CHECKED로 넘겨 컴파일할 때 확인합니다. UPROPERTY가 아닌 멤버면 여기서 이름을 알리며 멈춥니다. */
	const FProperty& GetJsonProperty(FName PropertyName) const;
};
