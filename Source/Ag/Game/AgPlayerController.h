// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AgPlayerController.generated.h"

class UAgInventoryComponent;
class UCommonActivatableWidget;
class UInputMappingContext;
class UAgUILayout;
enum class EAgUILayer : uint8;

UCLASS()
class AAgPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AAgPlayerController();

	virtual void ReceivedPlayer() override;

	UCommonActivatableWidget* PushScreen(EAgUILayer Layer, TSubclassOf<UCommonActivatableWidget> ScreenClass);

	UAgInventoryComponent* GetInventory() const
	{
		return Inventory;
	}

protected:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UAgUILayout> UILayoutClass;

	/** CommonUI는 Enhanced Input UI 액션(뒤로 등)의 키를 로컬 플레이어에 걸린 매핑에서 찾으므로, 화면과 관계없이 늘 걸어 둡니다. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TObjectPtr<UInputMappingContext> UIInputMapping;

private:
	UPROPERTY(VisibleAnywhere, Category = "Items")
	TObjectPtr<UAgInventoryComponent> Inventory;

	UPROPERTY()
	TObjectPtr<UAgUILayout> UILayout;
};
