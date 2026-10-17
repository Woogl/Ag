// Copyright Woogle. All Rights Reserved.

#pragma once

#include "EditorValidatorBase.h"
#include "AgMontageTaskValidator.generated.h"

UCLASS()
class UAgMontageTaskValidator : public UEditorValidatorBase
{
	GENERATED_BODY()

protected:
	virtual bool CanValidateAsset_Implementation(
	    const FAssetData& AssetData, UObject* Asset, FDataValidationContext& Context) const override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(
	    const FAssetData& AssetData, UObject* Asset, FDataValidationContext& Context) override;
};
