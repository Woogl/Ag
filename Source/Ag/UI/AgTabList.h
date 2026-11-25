// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonTabListWidgetBase.h"
#include "AgTabList.generated.h"

class UAgTabButton;
class UCommonActionWidget;
class UCommonActivatableWidget;
class UHorizontalBox;

USTRUCT()
struct FAgTab
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Tab")
	FText Label;

	UPROPERTY(EditAnywhere, Category = "Tab")
	TSubclassOf<UCommonActivatableWidget> Content;
};

/**
 * 탭 바입니다. 탭마다 내용 화면을 한 번 만들어 연결된 전환기에 넣고, 탭 이름 버튼을 양 끝의 탭 바꾸기 키 아이콘 사이에
 * 늘어놓습니다. 화면에 붙을 때마다 기본 탭을 고르고, 탭을 고르면 엔진이 전환기의 화면을 바꿉니다.
 */
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgTabList : public UCommonTabListWidgetBase
{
	GENERATED_BODY()

public:
	UAgTabList(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void HandleTabCreation_Implementation(FName TabNameID, UCommonButtonBase* TabButton) override;

	UPROPERTY(EditAnywhere, Category = "Tab")
	TArray<FAgTab> Tabs;

	UPROPERTY(EditAnywhere, Category = "Tab")
	int32 DefaultTabIndex = 0;

	UPROPERTY(EditAnywhere, Category = "Tab")
	TSubclassOf<UAgTabButton> TabButtonClass;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonActionWidget> PreviousTabAction;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> TabButtonBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonActionWidget> NextTabAction;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UCommonActivatableWidget>> ContentWidgets;
};
