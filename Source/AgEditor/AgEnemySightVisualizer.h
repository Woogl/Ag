// Copyright Woogle. All Rights Reserved.

#pragma once

#include "ComponentVisualizer.h"

class FAgEnemySightVisualizer : public FComponentVisualizer
{
public:
	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
};
