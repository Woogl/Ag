// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AgPlayerController.generated.h"

class UCommonActivatableWidget;
class UAgUILayout;
enum class EAgUILayer : uint8;

UCLASS()
class AAgPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AAgPlayerController();

	virtual void ReceivedPlayer() override;

	void PushScreen(EAgUILayer Layer, TSubclassOf<UCommonActivatableWidget> ScreenClass);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgUILayout> UILayoutClass;

private:
	UPROPERTY()
	TObjectPtr<UAgUILayout> UILayout;
};
