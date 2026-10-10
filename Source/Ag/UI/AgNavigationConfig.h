// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Application/NavigationConfig.h"

/** Slate navigation with W·S added to the arrow keys (게임 플로우 'UI 조작'). */
class FAgNavigationConfig : public FNavigationConfig
{
public:
	FAgNavigationConfig()
	{
		KeyEventRules.Emplace(EKeys::W, EUINavigation::Up);
		KeyEventRules.Emplace(EKeys::S, EUINavigation::Down);
	}
};
