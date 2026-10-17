// Copyright Woogle. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
class UAnimMontage;
class UAgCharacterData;
class AAgCharacterBase;
struct FAgAbilityData;
#if WITH_EDITOR
namespace AgMontageTaskValidation
{
	AG_API bool CanValidate(const UObject& Asset);
	AG_API void ValidateAsset(const UObject& Asset, TArray<FString>& Errors);
	void ValidateMontage(const UAnimMontage& Montage, const FAgAbilityData& Ability, TArray<FString>& Errors);
	void ValidateCharacter(const UAgCharacterData& Data, const AAgCharacterBase* Avatar, TArray<FString>& Errors);
}

#endif
