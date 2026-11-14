// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AgInteractable.generated.h"

class AAgPlayerCharacter;

UINTERFACE(meta = (CannotImplementInterfaceInBlueprint))
class UAgInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 상호작용 키로 쓸 수 있는 장치(문, 아이템, 레버)가 구현합니다. 상호작용 범위는 장치가 가진 AgInteractionRange 프로파일의
 * 도형이고, 쓸 수 없는 동안에는 장치가 그 도형의 충돌을 끕니다.
 */
class IAgInteractable
{
	GENERATED_BODY()

public:
	/** 지금 상호작용하면 할 행동의 이름입니다. 장치의 상태에 따라 달라질 수 있습니다. */
	virtual FText GetInteractionText() const = 0;

	/** 안내를 띄울 지점입니다. */
	virtual FVector GetInteractionLocation() const = 0;

	virtual void Interact(AAgPlayerCharacter& Player) = 0;
};
