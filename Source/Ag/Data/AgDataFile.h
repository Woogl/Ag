// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * 프로젝트 Data 폴더의 JSON 데이터 파일(구현 계획서 '데이터').
 * 파일 하나가 데이터 객체의 Transient 속성을 채웁니다. 그 속성은 모두 파일에 있어야 하고, 다른 키는 없어야 합니다.
 * 키는 속성 이름이고, 태그는 태그 이름으로, 열거형은 값 이름으로 적습니다.
 */
namespace AgDataFile
{
	/** The Data folder. */
	AG_API FString GetFolder();

	/** Full path of a file in the Data folder. */
	FString GetPath(const FString& FileName);

	/**
	 * FileName으로 Target의 Transient 속성을 채웁니다. 오류(문법, 모르는 키나 빠진 키, 잘못된 형식, 없는 태그나 클래스, 정수가 아닌 값,
	 * 범위 밖의 값)가 하나라도 있으면 모든 값을 그대로 두고, 오류는 로그와 화면에 알립니다. 범위는 에디터에서만 검사합니다.
	 */
	bool Load(UObject& Target, const FString& FileName);

#if WITH_EDITOR
	/**
	 * Fills every object loaded from these files again, then broadcasts OnReloaded once if any object uses them.
	 * The data reload subsystem calls this when files in the Data folder are saved.
	 */
	AG_API void ReloadFiles(const TArray<FString>& FileNames);

	/** Broadcast after data files were read again, so values applied once at spawn are applied again. */
	FSimpleMulticastDelegate& OnReloaded();
#endif
}
