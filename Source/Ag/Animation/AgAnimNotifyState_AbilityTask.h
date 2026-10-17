// Copyright Woogle. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "AnimNotifies/AnimNotify_PlayMontageNotify.h"
#include "AgAnimNotifyState_AbilityTask.generated.h"

/** 기능 참조와 구간만 전달합니다. 공유 Notify 객체에는 실행 상태를 보관하지 않습니다. */
UCLASS(HideCategories = (Notify), meta = (DisplayName = "Ag Ability Task"))
class UAgAnimNotifyState_AbilityTask : public UAnimNotify_PlayMontageNotifyWindow
{
	GENERATED_BODY()

public:
	UAgAnimNotifyState_AbilityTask();

	UPROPERTY(EditAnywhere, Category = "Task")
	FName Task;

	UPROPERTY(EditAnywhere, Category = "Task")
	FName ParamsId;
	virtual FString GetNotifyName_Implementation() const override
	{
		return ParamsId.IsNone() ? Task.ToString() : Task.ToString() + TEXT("/") + ParamsId.ToString();
	}
};
