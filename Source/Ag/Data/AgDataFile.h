// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * The JSON data files in the project's Data folder (구현 계획서 '데이터').
 * A file fills the Transient properties of its data object: every one of them must be in the file, and nothing else.
 * Keys are the property names; tags are written as tag names, classes as class paths and enums as value names.
 */
namespace AgDataFile
{
	/** The Data folder. */
	AG_API FString GetFolder();

	/** Full path of a file in the Data folder. */
	FString GetPath(const FString& FileName);

	/**
	 * Fills the Transient properties of Target from FileName. Any error (syntax, unknown or missing key, wrong type, unknown
	 * tag or class, out of range) leaves every value as it was; the errors go to the log and the screen.
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
