// Copyright Woogle. All Rights Reserved.

#include "AgMontageTaskValidator.h"
#include "Animation/AgMontageTaskValidation.h"

bool UAgMontageTaskValidator::CanValidateAsset_Implementation(
    const FAssetData& AssetData, UObject* Asset, FDataValidationContext& Context) const
{
	return Asset && AgMontageTaskValidation::CanValidate(*Asset);
}

EDataValidationResult UAgMontageTaskValidator::ValidateLoadedAsset_Implementation(
    const FAssetData& AssetData, UObject* Asset, FDataValidationContext& Context)
{
	TArray<FString> Errors;
	AgMontageTaskValidation::ValidateAsset(*Asset, Errors);
	for (const FString& Error : Errors)
	{
		AssetFails(Asset, FText::FromString(Error));
	}
	if (Errors.IsEmpty())
	{
		AssetPasses(Asset);
	}
	return GetValidationResult();
}
