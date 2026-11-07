// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GameplayTagContainer.h"
#include "AgSkillSlot.generated.h"

class UAgAbilitySystemComponent;
class UGameplayAbility;
class UImage;
class UTextBlock;
struct FGameplayAbilitySpecHandle;

/** 전투 HUD의 스킬·궁극기 칸. 입력 태그의 어빌리티로 아이콘, MP 코스트, 흐림, 쿨다운을 그립니다. */
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgSkillSlot : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	void SetInput(UAgAbilitySystemComponent* InASC, const FGameplayTag& InInputTag);

	/** 아이콘, MP 코스트, 흐림을 다시 그립니다. */
	void UpdateAbility();

	/** 쿨다운 표시를 다시 그리고, 쿨다운 중이면 다음 틱에 다시 부릅니다. */
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

	UPROPERTY(EditAnywhere, Category = "HUD", meta = (ClampMin = 0, ClampMax = 1))
	float DisabledOpacity = 1.f;

private:
	const UGameplayAbility* FindAbility(FGameplayAbilitySpecHandle& OutHandle) const;

	TWeakObjectPtr<UAgAbilitySystemComponent> ASC;

	FGameplayTag InputTag;

	int32 ShownCost = INDEX_NONE;
	FIntPoint ShownCooldown = FIntPoint(INDEX_NONE, INDEX_NONE);

	FTimerHandle CooldownTimer;
};
