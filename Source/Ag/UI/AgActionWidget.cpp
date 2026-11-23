// Copyright Woogle. All Rights Reserved.

#include "UI/AgActionWidget.h"

#include "CommonInputBaseTypes.h"
#include "CommonInputSubsystem.h"
#include "CommonUITypes.h"
#include "Styling/StyleDefaults.h"

FSlateBrush UAgActionWidget::GetIcon() const
{
	const UCommonInputSubsystem* InputSubsystem = GetInputSubsystem();
	const UInputAction* Action = GetEnhancedInputAction();
	if (IsDesignTime() || !InputSubsystem || !Action || !CommonUI::IsEnhancedInputSupportEnabled())
	{
		return Super::GetIcon();
	}

	TArray<FKey> MappedKeys;
	CommonUI::GetEnhancedInputActionKeys(InputSubsystem->GetLocalPlayer(), Action, MappedKeys);

	// 키 묶음은 키 수가 같아야 맞으므로, 같은 키가 두 번 매핑되어도 한 번만 넘깁니다.
	const ECommonInputType InputType = InputSubsystem->GetCurrentInputType();
	TArray<FKey> Keys;
	for (const FKey& Key : MappedKeys)
	{
		if (CommonUI::IsKeyValidForInputType(Key, InputType))
		{
			Keys.AddUnique(Key);
		}
	}

	FSlateBrush Brush;
	if (UCommonInputPlatformSettings::Get()->TryGetInputBrush(Brush, Keys, InputType, InputSubsystem->GetCurrentGamepadName()))
	{
		return Brush;
	}
	return *FStyleDefaults::GetNoBrush();
}
