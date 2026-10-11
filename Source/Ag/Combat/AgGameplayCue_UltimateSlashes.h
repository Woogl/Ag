// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "AgGameplayCue_UltimateSlashes.generated.h"

class UStaticMeshComponent;

/**
 * 궁극기의 베기 선(플레이어 사양 '베기 선'). 큐가 켜진 동안 실행(Execute)될 때마다 넘어온 위치에 베기 선을 하나 새기고,
 * 큐가 꺼지면 모두 함께 지웁니다. 위치(Location)와 베기 선의 방향(Normal)은 실행하는 쪽이 정하고, 모양은 큐 블루프린트의
 * 견본 메시 컴포넌트(SlashTemplate)가 정합니다.
 */
UCLASS(Abstract)
class AAgGameplayCue_UltimateSlashes : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	AAgGameplayCue_UltimateSlashes();

	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

protected:
	/** 베기 선 하나의 모양(메시, 머티리얼, 크기, 색). 긴 쪽이 Y축입니다. 견본 자체는 보이지 않습니다. */
	UPROPERTY(VisibleAnywhere, Category = "Slash")
	TObjectPtr<UStaticMeshComponent> SlashTemplate;

private:
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Slashes;
};
