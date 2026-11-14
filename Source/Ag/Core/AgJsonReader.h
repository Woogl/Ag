// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FArrayProperty;
class FJsonObject;
class FJsonValue;
class FMapProperty;
class FObjectProperty;
class FStructProperty;
class UStruct;
struct FInstancedStruct;

/** DesignData의 JSON을 리플렉션으로 속성 값에 읽고, 오류는 모두 알린 뒤 나머지를 계속 읽습니다. */
class FAgJsonReader
{
public:
	/** 읽으면서 만드는 객체는 Outer 아래에 둡니다. */
	explicit FAgJsonReader(UObject& InOuter);

	/** 정의 ID는 이미 읽은 파일의 최상위 키와 대조하므로, ID로 가리켜지는 파일을 먼저 읽습니다. */
	bool ReadFile(const FString& Path, const FProperty& Property, void* Out);

	/** 파일의 최상위 키가 곧 Object의 필드인 파일을 읽습니다. */
	bool ReadFile(const FString& Path, UObject& Object);

	/** 다시 읽은 데이터를 기존 객체에 복사할 때도 같은 속성만 옮겨야, 읽은 값이 버려지지 않습니다. */
	static bool IsField(const FProperty& Property);

private:
	bool ParseFile(const FString& Path, TSharedPtr<FJsonValue>& OutValue);

	/** 속성의 종류를 가려 아래 함수 하나로 넘깁니다. */
	bool Read(const FString& Path, const TSharedPtr<FJsonValue>& Value, const FProperty& Property, void* Out);

	bool ReadArray(const FString& Path, const TSharedPtr<FJsonValue>& Value, const FArrayProperty& Property, void* Out);
	bool ReadMap(const FString& Path, const TSharedPtr<FJsonValue>& Value, const FMapProperty& Property, void* Out);

	/** `{"Task": 이름, 필드}`를 그 Task의 Params 구조체로 읽습니다. */
	bool ReadTask(const FString& Path, const TSharedPtr<FJsonValue>& Value, FInstancedStruct& Out);

	/** JSON 객체를 UObject로 읽습니다. 비어 있는 속성은 `Class` 키의 클래스(없으면 속성의 클래스)로 객체를 만듭니다. */
	bool ReadObject(const FString& Path, const TSharedPtr<FJsonValue>& Value, const FObjectProperty& Property, void* Out);
	bool ReadStruct(const FString& Path, const TSharedPtr<FJsonValue>& Value, const FStructProperty& Property, void* Out);

	/** JSON 객체의 키마다 Type의 속성을 찾아 읽습니다. TypeKey는 종류를 고르는 키(`Task`, `Class`)라 건너뜁니다. */
	bool ReadFields(const FString& Path, const FJsonObject& Object, const UStruct& Type, void* Container, const TCHAR* TypeKey = nullptr);

	/** 나머지 값은 엔진 변환으로 읽고, 엔진이 놓치는 것(없는 에셋, 리다이렉터, 없는 정의 ID)을 확인합니다. */
	bool ReadValue(const FString& Path, const TSharedPtr<FJsonValue>& Value, const FProperty& Property, void* Out);

	UObject& Outer;

	TMap<FName, TSet<FName>> Ids;
};
