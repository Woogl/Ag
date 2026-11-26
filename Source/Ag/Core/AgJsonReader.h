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
struct FAgSpawnActorDefinition;
struct FInstancedStruct;

/** 파싱한 JSON 파일 하나입니다. 에셋을 가리키는 값도 글자일 뿐이라, 들고 있어도 에셋을 읽지 않습니다. */
struct FAgJsonDocument
{
	TSharedPtr<FJsonObject> Root;

	/** null이 아닌 최상위 항목입니다. ID와 같이 FName으로 찾습니다. */
	TMap<FName, TSharedPtr<FJsonValue>> Entries;
};

/** 파싱한 파일을 확장자를 뺀 파일 이름으로 찾는 표입니다. */
using FAgJsonDocuments = TMap<FName, FAgJsonDocument>;

/** DesignData의 JSON을 리플렉션으로 속성 값에 읽고, 오류는 모두 알린 뒤 나머지를 계속 읽습니다. */
class FAgJsonReader
{
public:
	/**
	 * 읽으면서 만드는 객체는 Outer 아래에 둡니다. Documents는 파싱한 파일의 표로, 읽기 함수는 모두 이 표에서 읽고 정의 ID
	 * 검사와 생성 액터 정의 읽기도 이 표를 봅니다.
	 */
	FAgJsonReader(UObject& InOuter, FAgJsonDocuments& InDocuments);

	/** 파일을 파싱해 표에 넣습니다. */
	bool ParseFile(const FString& Path);

	/** 파싱해 둔 파일 전체를 그 속성에 읽습니다. */
	bool ReadFile(const FString& Path, const FProperty& Property, void* Out);

	/** 파싱해 둔 파일에서 그 ID의 항목만 맵 속성에 읽습니다. 파일에 없는 ID는 건너뜁니다. */
	bool ReadEntries(const FString& Path, TConstArrayView<FName> Ids, const FProperty& Property, void* Out);

	/** 파일의 최상위 키가 곧 Object의 필드인 파일을 읽습니다. */
	bool ReadFile(const FString& Path, UObject& Object);

	/** 파싱해 둔 파일의 null이 아닌 최상위 키입니다. */
	TArray<FName> GetEntryIds(const FString& Path) const;

	/** `SpawnActors.json`의 그 ID 항목을 클래스와, 그 액터 클래스가 정한 구조체의 속성값으로 읽습니다. */
	bool ReadSpawnActor(const FString& Path, FName Id, FAgSpawnActorDefinition& Out);

	/** 다시 읽은 데이터를 기존 객체에 복사할 때도 같은 속성만 옮겨야, 읽은 값이 버려지지 않습니다. */
	static bool IsField(const FProperty& Property);

private:
	const FAgJsonDocument* FindDocument(const FString& Path) const;

	/** 읽을 파일을 찾습니다. 폴더에 없는 파일이면 오류를 알립니다. */
	const FAgJsonDocument* FindFile(const FString& Path) const;

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

	FAgJsonDocuments& Documents;
};
