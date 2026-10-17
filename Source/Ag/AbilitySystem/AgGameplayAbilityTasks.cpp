// Copyright Woogle. All Rights Reserved.
#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystem/Tasks/AgAbilityTask_Attack.h"
#include "AbilitySystem/Tasks/AgAbilityTask_Dash.h"
#include "AbilitySystem/Tasks/AgAbilityTask_HoldTag.h"
#include "AbilitySystem/Tasks/AgAbilityTask_SetCollisionResponseToChannel.h"
#include "AbilitySystem/Tasks/AgAbilityTask_SpawnProjectile.h"
#include "Animation/AgMontageTimelineComponent.h"
#include "Animation/AgRootMotionModifier_MotionWarping.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/AgCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/AgCharacterData.h"
#include "Data/AgMontageTimeline.h"
#include "Ag.h"

namespace
{
	/** 시간표 구간의 실행 키: 기능과 그 기능 배열의 번호 */
	int32 TimelineKey(const FAgTimelineEvent& Event)
	{
		return static_cast<int32>(Event.Feature) * 1000 + Event.Index;
	}
}

void UAgGameplayAbility::HandleTimelineEvent(const FAgTimelineEvent& Event)
{
	if (!OwnsMontage(Event.InstanceId) || bEnding || !Event.Timeline)
	{
		return;
	}
	const FAgMontageTimeline& Timeline = *Event.Timeline;
	const int32 Key = TimelineKey(Event);
	if (Event.Phase == EAgTimelinePhase::End)
	{
		EndWindowTask(Event.InstanceId, Key);
		return;
	}
	switch (Event.Feature)
	{
	case EAgTimelineFeature::Attack:
		StartWindowTask(Event.InstanceId, Key, TEXT("Attack"),
		    [&]() -> UAbilityTask*
		    {
			    return UAgAbilityTask_Attack::Sweep(this, Timeline.Attack[Event.Index]);
		    });
		break;
	case EAgTimelineFeature::HoldTag:
		StartWindowTask(Event.InstanceId, Key, TEXT("HoldTag"),
		    [&]() -> UAbilityTask*
		    {
			    return UAgAbilityTask_HoldTag::HoldTag(this, Timeline.HoldTag[Event.Index].Tag);
		    });
		break;
	case EAgTimelineFeature::SetCollisionResponseToChannel:
		StartWindowTask(Event.InstanceId, Key, TEXT("SetCollisionResponseToChannel"),
		    [&]() -> UAbilityTask*
		    {
			    const FAgTimelineCollisionResponse& Params = Timeline.SetCollisionResponseToChannel[Event.Index];
			    return UAgAbilityTask_SetCollisionResponseToChannel::SetCollisionResponseToChannel(
			        this, GetAgCharacter()->GetCapsuleComponent(), Params.Channel, Params.Response);
		    });
		break;
	case EAgTimelineFeature::Dash:
		StartWindowTask(Event.InstanceId, Key, TEXT("Dash"),
		    [&]() -> UAbilityTask*
		    {
			    return MakeDashTask(Timeline.Dash[Event.Index], Event.InstanceId, Key);
		    });
		break;
	case EAgTimelineFeature::GameplayEvent:
		HandleMontageEvent(Event.InstanceId, Timeline.GameplayEvent[Event.Index].Event);
		break;
	case EAgTimelineFeature::SpawnProjectile:
	{
		const FAgTimelineSpawnProjectile& Shot = Timeline.SpawnProjectile[Event.Index];
		LaunchProjectile(Event.InstanceId, Shot.ProjectileId, Shot.Attack, Shot.Origin, Shot.SocketName);
		break;
	}
	}
}

void UAgGameplayAbility::StartWindowTask(int32 InstanceId, int32 Key, FName Function, TFunctionRef<UAbilityTask*()> MakeTask)
{
	if (Windows.ContainsByPredicate(
	        [=](const FAgAbilityWindowExecution& E)
	        {
		        return E.MontageInstanceId == InstanceId && E.TimelineKey == Key;
	        }))
	{
		return;
	}
	if (GetAbilityId() == TEXT("DashThrust") && bDashFinished &&
	    (Function == TEXT("Dash") || Function == TEXT("Attack") || Function == TEXT("SetCollisionResponseToChannel")))
	{
		return;
	}
	UAbilityTask* Task = MakeTask();
	if (!Task)
	{
		return;
	}
	FAgAbilityWindowExecution& Execution = Windows.AddDefaulted_GetRef();
	Execution.Task = Task;
	Execution.Function = Function;
	Execution.MontageInstanceId = InstanceId;
	Execution.TimelineKey = Key;
	Task->ReadyForActivation();
}

UAbilityTask* UAgGameplayAbility::MakeDashTask(const FAgDashParams& Params, int32 InstanceId, int32 Key)
{
	AActor* Target = GetAgCharacter()->FindAttackTarget();
	if (!Target)
	{
		UE_LOG(LogAg, Error, TEXT("%s Abilities.%s Dash: missing target input (play %d window %d)"),
		    *GetAgCharacter()->GetCharacterData()->GetName(), *GetAbilityId().ToString(), InstanceId, Key);
		return nullptr;
	}
	DashTask = UAgAbilityTask_Dash::Dash(this, Params, GetAgCharacter()->GetActorForwardVector(), Target->GetActorLocation(), Target);
	if (GetAbilityId() == TEXT("DashThrust"))
	{
		DashTask->OnFinished.AddDynamic(this, &ThisClass::HandleDashFinished);
	}
	return DashTask;
}

void UAgGameplayAbility::EndWindowTask(int32 InstanceId, int32 Key)
{
	const int32 Index = Windows.IndexOfByPredicate(
	    [=](const FAgAbilityWindowExecution& E)
	    {
		    return E.MontageInstanceId == InstanceId && E.TimelineKey == Key;
	    });
	if (Index == INDEX_NONE)
	{
		return;
	}
	UAbilityTask* Task = Windows[Index].Task;
	Windows.RemoveAt(Index);
	if (Task && !Task->IsFinished())
	{
		Task->EndTask();
	}
}

void UAgGameplayAbility::HandleDashFinished()
{
	if (!IsActive() || bEnding)
	{
		return;
	}
	bDashFinished = true;
	DashTask = nullptr;
	// 기획의 돌진 종료 사건만 자기 행동의 남은 판정·이동·충돌 구간을 닫습니다.
	TArray<TObjectPtr<UAbilityTask>> Ending;
	for (FAgAbilityWindowExecution& Window : Windows)
	{
		if (Window.Function == TEXT("Dash") || Window.Function == TEXT("Attack") ||
		    Window.Function == TEXT("SetCollisionResponseToChannel"))
		{
			Ending.Add(Window.Task);
			Window.Task = nullptr;
		}
	}
	for (UAbilityTask* Task : Ending)
	{
		if (Task && !Task->IsFinished())
		{
			Task->EndTask();
		}
	}
	if (bHoldingDash && Motion.Montage)
	{
		bHoldingDash = false;
		GetActorInfo().GetAnimInstance()->Montage_Resume(Motion.Montage);
	}
}

void UAgGameplayAbility::LaunchProjectile(
    int32 InstanceId, FName ProjectileId, const FAgAttackParams& Attack, EAgProjectileOrigin Origin, FName SocketName)
{
	const FAgProjectileDefinition* Projectile = ExecutionProjectiles.Find(ProjectileId);
	if (!Projectile)
	{
		UE_LOG(LogAg, Error, TEXT("%s Abilities.%s: projectile %s missing in Projectiles (play %d)"),
		    *GetAgCharacter()->GetCharacterData()->GetName(), *GetAbilityId().ToString(), *ProjectileId.ToString(), InstanceId);
		return;
	}
	UAgAbilityTask_SpawnProjectile::SpawnProjectile(this, InstanceId, *Projectile, Attack, Origin, SocketName)->ReadyForActivation();
}

bool UAgGameplayAbility::PrepareWarpTarget(
    int32 InstanceId, float WindowStart, const FAgMotionWarpingParams& Params, AActor*& Target, float& Gap, bool& bFacingOnly)
{
	if (!OwnsMontage(InstanceId) || bEnding)
	{
		return false;
	}
	bFacingOnly = false;
	AAgCharacterBase* Character = GetAgCharacter();
	if (GetAbilityId() == TEXT("Execution"))
	{
		Target = ExecutionTarget.Get();
		Gap = WarpStopDistance;
		return true;
	}
	if (Motion.TargetPhaseStart != WindowStart)
	{
		Motion.TargetPhaseStart = WindowStart;
		Motion.Target = Character->FindAttackTarget();
		UpdateMotionConstraints();
	}
	Target = Motion.Target.Get();
	Gap = WarpStopDistance;
	if (!Target || (Params.bHasTargetingRange && Character->GetHorizontalDistanceTo(Target) > Params.TargetingRange))
	{
		Target = Character->GetFallbackFacingTarget();
		bFacingOnly = true;
	}
	return true;
}

void UAgGameplayAbility::RegisterWarp(UAgRootMotionModifier_MotionWarping* Modifier)
{
	Warps.AddUnique(Modifier);
}
