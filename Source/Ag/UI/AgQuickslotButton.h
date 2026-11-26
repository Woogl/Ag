// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "GameplayTagContainer.h"
#include "AgQuickslotButton.generated.h"

class AAgPlayerCharacter;
class UCommonLazyImage;
class UTextBlock;

/** 전투 HUD의 퀵슬롯 칸. 입력 태그의 어빌리티가 쓰는 아이템으로 아이콘, '보유 수/최대 보유 수', 흐림을 그립니다. */
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgQuickslotButton : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	void SetInput(AAgPlayerCharacter* InPlayer, const FGameplayTag& InInputTag);

	/** 아이콘, 보유 수, 흐림을 다시 그립니다. */
	void UpdateItem();

protected:
	/** 아이템이 없으면 흐리게 하는 부분입니다. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> Body;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonLazyImage> IconImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CountText;

	UPROPERTY(EditAnywhere, Category = "HUD", meta = (ClampMin = 0, ClampMax = 1))
	float DisabledOpacity = 1.f;

private:
	TWeakObjectPtr<AAgPlayerCharacter> Player;

	FGameplayTag InputTag;
};
