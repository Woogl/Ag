// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Tasks/TargetingFilterTask_BasicFilterTemplate.h"
#include "Tasks/TargetingSortTask_Base.h"
#include "Tasks/TargetingTask.h"
#include "AgTargeting.generated.h"

struct FAgAbilityTargeting;
struct FTargetingRequestHandle;

/**
 * 행동의 Targeting이 타겟팅 요청에 실어 보내는 조건(요청 핸들별 데이터). 프리셋 안의 행동 대상 필터(UAgTargetingFilterTask_ActionTarget)가
 * 읽습니다. 요청 핸들을 놓으면 함께 지워집니다.
 */
struct FAgTargetingConditions
{
	/** 캡슐 중심 사이 수평 거리(cm)의 상한. 음수면 제한 없음입니다. */
	float Range = -1.f;

	/** 대상이 가져야 할 태그 */
	FGameplayTagContainer RequiredTags;

	/** 대상이 가지면 안 되는 태그 */
	FGameplayTagContainer BlockedTags;

	static FAgTargetingConditions& FindOrAdd(FTargetingRequestHandle Handle);
	static FAgTargetingConditions* Find(FTargetingRequestHandle Handle);
};

namespace AgTargeting
{
	/**
	 * 행동의 Targeting을 Source 기준으로 평가해 첫 후보를 돌려줍니다(구현 계획서 '어빌리티와 AbilityTask'의 '대상'). 프리셋이 후보를
	 * 모으고 거르고 정렬합니다. 후보가 없으면 null입니다.
	 */
	AActor* FindTarget(const FAgAbilityTargeting& Targeting, AActor& Source);
}

/** 행동 대상 조건: 요청한 쪽과 적대 관계인 살아 있는 캐릭터만 남기고, 요청에 실은 범위와 태그 조건(FAgTargetingConditions)으로 거릅니다. */
UCLASS(meta = (DisplayName = "Ag Filter Action Target"))
class UAgTargetingFilterTask_ActionTarget : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

public:
	UAgTargetingFilterTask_ActionTarget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle,
	                                const FTargetingDefaultResultData& TargetData) const override;
};

/** 요청한 플레이어의 화면 안(뷰포트 안에 투영되는) 후보만 남깁니다. */
UCLASS(meta = (DisplayName = "Ag Filter On Screen"))
class UAgTargetingFilterTask_OnScreen : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

public:
	UAgTargetingFilterTask_OnScreen(const FObjectInitializer& ObjectInitializer);

protected:
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle,
	                                const FTargetingDefaultResultData& TargetData) const override;
};

/** 요청한 쪽과의 캡슐 중심 사이 수평 거리가 가까운 후보부터 둡니다. 거르는 Task 뒤에 둡니다(필터가 순서를 섞음). */
UCLASS(meta = (DisplayName = "Ag Sort Horizontal Distance"))
class UAgTargetingSortTask_HorizontalDistance : public UTargetingSortTask_Base
{
	GENERATED_BODY()

public:
	UAgTargetingSortTask_HorizontalDistance(const FObjectInitializer& ObjectInitializer);

protected:
	virtual float GetScoreForTarget(const FTargetingRequestHandle& TargetingHandle,
	                                const FTargetingDefaultResultData& TargetData) const override;
};

/** 요청한 보스의 AI 대상(보스 AI 컨트롤러가 정한 대상)을 후보로 더합니다. */
UCLASS(meta = (DisplayName = "Ag Select AI Target"))
class UAgTargetingSelectionTask_AITarget : public UTargetingTask
{
	GENERATED_BODY()

public:
	UAgTargetingSelectionTask_AITarget(const FObjectInitializer& ObjectInitializer);

	virtual void Execute(const FTargetingRequestHandle& TargetingHandle) const override;
};
