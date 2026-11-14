// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "AgNameplateWidget.generated.h"

class UAbilitySystemComponent;
class UAgStatBar;
struct FOnAttributeChangeData;

/**
 * 적의 HP·PP 바입니다. 일반 적·샌드백 머리 위 네임플레이트와 HUD의 보스 상태가 함께 쓰며, 표시 조건(Update)은 머리 위
 * 네임플레이트만 씁니다. 크기와 간격, 머리와의 여백은 WBP 배치가 정합니다.
 */
UCLASS(Abstract, meta = (DisableNativeTick))
class UAgNameplateWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * 화면 공간 위젯 컴포넌트는 숨길 때 위젯을 화면에서 내려 NativeDestruct가 불리므로, 구독은 다음 호출까지 유지합니다.
	 * 숨긴 동안에도 최근 감소 시각과 바를 따라가야 합니다. 구독은 약한 참조라 위젯이 사라져도 남지 않습니다.
	 */
	void SetAbilitySystem(UAbilitySystemComponent* NewASC);

	bool Update(bool bAllowed, bool bRequested);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgStatBar> HPBar;

	/** 최대 PP가 0이면 접습니다. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> PPSection;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAgStatBar> PPBar;

	UPROPERTY(EditDefaultsOnly, Category = "Nameplate")
	float RecentDropShowTime = 0.f;

private:
	void Unbind();

	/** 처형 피격 중의 PP 바는 처형으로 전환될 때 멈춘 표시를 그대로 둡니다. */
	void ClearTrails();

	void HandleAttributeChanged(const FOnAttributeChangeData& Data);

	void UpdateHP();
	void UpdatePP();

	TWeakObjectPtr<UAbilitySystemComponent> ASC;

	double LastDropTime = -UE_DOUBLE_BIG_NUMBER;

	bool bWasAllowed = false;

	FTimerHandle GroggyTimer;
};
