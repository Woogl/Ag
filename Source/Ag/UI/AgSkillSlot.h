// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GameplayTagContainer.h"
#include "AgSkillSlot.generated.h"

class AAgPlayerCharacter;
class UAgGameplayAbility;
class UImage;
class UTextBlock;

/** 전투 HUD의 스킬·궁극기·회복병 칸. 입력 태그의 어빌리티로 아이콘, MP 코스트, 흐림, 쿨다운, 남은 충전 수를 그립니다. */
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgSkillSlot : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	void SetInput(AAgPlayerCharacter* InPlayer, const FGameplayTag& InInputTag);

	/** 아이콘, MP 코스트, 흐림을 다시 그립니다. */
	void UpdateAbility();

	/** 쿨다운 표시와 남은 충전 수를 다시 그리고, 충전이 최대보다 적으면 다음 틱에 다시 부릅니다. */
	void UpdateCooldown();

protected:
	virtual void NativeDestruct() override;

	/** 자원이 부족하면 흐리게 하는 부분입니다. 쿨다운 표시는 흐릴 때도 그대로 보이도록 이 밖에 둡니다. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> Body;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> IconImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CostText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CooldownSweep;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CooldownText;

	/** 최대 충전이 2회 이상인 어빌리티만 보입니다. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ChargeText;

	UPROPERTY(EditAnywhere, Category = "HUD", meta = (ClampMin = 0, ClampMax = 1))
	float DisabledOpacity = 1.f;

private:
	/** 이 칸의 입력 태그로 발동하는 어빌리티입니다. */
	const UAgGameplayAbility* FindAbility() const;

	TWeakObjectPtr<AAgPlayerCharacter> Player;

	FGameplayTag InputTag;

	int32 ShownCost = INDEX_NONE;
	FIntPoint ShownCooldown = FIntPoint(INDEX_NONE, INDEX_NONE);
	int32 ShownCharges = INDEX_NONE;

	FTimerHandle CooldownTimer;
};
