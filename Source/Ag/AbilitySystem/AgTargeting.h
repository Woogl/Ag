// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "Tasks/TargetingFilterTask_BasicFilterTemplate.h"
#include "Tasks/TargetingSortTask_Base.h"
#include "Tasks/TargetingTask.h"
#include "AgTargeting.generated.h"

struct FAgTargetingRequest;

namespace AgTargeting
{
	TArray<AActor*> FindTargets(const FAgTargetingRequest& Request, AActor& Source);
	AActor* FindTarget(const FAgTargetingRequest& Request, AActor& Source);
	bool IsInRange(const AActor& Source, const AActor& Target, float Range);

	/** 출처가 AI면 지각의 시야가 대상을 지금 감지하고, Socket이 있으면 출처 메시의 그 소켓에서 대상 캡슐 중심까지 WorldStatic이 가리지 않습니다. */
	bool IsVisible(const AActor& Source, const AActor& Target, FName Socket);

	/** 조종하는 폰의 카메라 컴포넌트 시야로 투영합니다. 처형 카메라나 컷신이 화면을 바꿔도 게임 카메라 기준입니다. */
	bool ProjectToGameScreen(const APawn& Pawn, const FVector& WorldLocation, FVector2D& OutScreenPosition);

	/** 후보 가운데 게임 카메라 화면에서 Current보다 Direction 쪽(양수 = 오른쪽)에 있고 Current와 가로로 가장 가까운 대상입니다. */
	AActor* FindSideTarget(const FAgTargetingRequest& Request, APawn& Source, const AActor& Current, float Direction);
}

UCLASS()
class UAgTargetingSelectionTask_Sphere : public UTargetingTask
{
	GENERATED_BODY()

public:
	virtual void Execute(const FTargetingRequestHandle& TargetingHandle) const override;
};

UCLASS()
class UAgTargetingFilterTask_Hostile : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

protected:
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;
};

UCLASS()
class UAgTargetingFilterTask_Hittable : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

protected:
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;
};

UCLASS()
class UAgTargetingFilterTask_TagRequirements : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

protected:
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;

	UPROPERTY(EditAnywhere, Category = "Target Filter")
	FGameplayTagRequirements Requirements;
};

UCLASS()
class UAgTargetingFilterTask_OnScreen : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

protected:
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;
};

UCLASS()
class UAgTargetingFilterTask_Visible : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

protected:
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;

	UPROPERTY(EditAnywhere, Category = "Target Filter")
	FName Socket;
};

UCLASS()
class UAgTargetingSortTask_ScreenCenter : public UTargetingSortTask_Base
{
	GENERATED_BODY()

protected:
	virtual float GetScoreForTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;
};
