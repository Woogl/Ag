// Copyright Woogle. All Rights Reserved.

#include "Animation/AgAnimNotifyState_AbilityTask.h"

UAgAnimNotifyState_AbilityTask::UAgAnimNotifyState_AbilityTask()
{
	NotifyStateBehaviorFlags = static_cast<uint8>(EAnimNotifyStateBehaviorFlags::NoMergeOnConcurrentPlay);
}
