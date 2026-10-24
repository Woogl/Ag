// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AgEnemySightPreviewComponent.generated.h"

UCLASS(MinimalAPI)
class UAgEnemySightPreviewComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAgEnemySightPreviewComponent();

#if WITH_EDITOR
	AG_API bool GetSightPreview(FVector& OutOrigin, FQuat& OutRotation, float& OutDistance, float& OutAngle) const;
#endif
};
