// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FJsonValue;

/** DesignData의 JSON을 리플렉션으로 속성 값에 읽고, 오류는 모두 알린 뒤 나머지를 계속 읽습니다. */
class FAgJsonReader
{
public:
	/** `Class` 키로 만드는 객체는 Outer 아래에 둡니다. */
	explicit FAgJsonReader(UObject& InOuter);

	/** 정의 ID는 이미 읽은 파일의 최상위 키와 대조하므로, ID로 가리켜지는 파일을 먼저 읽습니다. */
	bool ReadFile(const FString& Path, const FProperty& Property, void* Out);

	/** 다시 읽은 데이터를 기존 객체에 복사할 때도 같은 속성만 옮겨야, 읽은 값이 버려지지 않습니다. */
	static bool IsField(const FProperty& Property);

private:
	bool Read(const FString& Path, const TSharedPtr<FJsonValue>& Value, const FProperty& Property, void* Out);

	UObject& Outer;

	TMap<FName, TSet<FName>> Ids;
};
