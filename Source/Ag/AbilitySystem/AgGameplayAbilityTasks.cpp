// Copyright Woogle. All Rights Reserved.
#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystem/Tasks/AgAbilityTask_Attack.h"
#include "AbilitySystem/Tasks/AgAbilityTask_Dash.h"
#include "AbilitySystem/Tasks/AgAbilityTask_HoldTag.h"
#include "AbilitySystem/Tasks/AgAbilityTask_SetCollisionResponseToChannel.h"
#include "AbilitySystem/Tasks/AgAbilityTask_TimeStop.h"
#include "AbilitySystem/Tasks/AgAbilityTask_SpawnProjectile.h"
#include "Animation/AgAnimNotify_SpawnProjectile.h"
#include "Animation/AgAnimNotify_GameplayEvent.h"
#include "Animation/AgAnimNotifyState_AbilityTask.h"
#include "Animation/AgRootMotionModifier_MotionWarping.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/AgCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/AgCharacterData.h"
#include "Ag.h"

bool UAgGameplayAbility::IsCurrentMontageNotify(const FBranchingPointNotifyPayload& Payload) const
{
	UAnimInstance* Anim = Motion.AnimInstance.Get();
	const AAgCharacterBase* Character = GetAgCharacter();
	return !bEnding && OwnsMontage(Payload.MontageInstanceID) && Anim && Character && Payload.SkelMeshComponent == Character->GetMesh() &&
	       Payload.SkelMeshComponent->GetAnimInstance() == Anim && Payload.SequenceAsset == Motion.Montage && Payload.NotifyEvent &&
	       Anim->GetMontageInstanceForID(Payload.MontageInstanceID);
}

void UAgGameplayAbility::HandleMontageNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& Payload)
{
	if (!IsCurrentMontageNotify(Payload))
	{
		return;
	}
	const FAnimNotifyEvent& Event = *Payload.NotifyEvent;
	if (const UAgAnimNotifyState_AbilityTask* Notify = Cast<UAgAnimNotifyState_AbilityTask>(Event.NotifyStateClass))
	{
		const int32 Index = Motion.Montage->Notifies.IndexOfByKey(Event);
		BeginNotifyTask(Payload.MontageInstanceID, Index, Notify->Task, Notify->ParamsId);
	}
	else if (const UAgAnimNotify_GameplayEvent* GameplayEvent = Cast<UAgAnimNotify_GameplayEvent>(Event.Notify))
	{
		HandleMontageEvent(Payload.MontageInstanceID, GameplayEvent->EventTag);
	}
	else if (const UAgAnimNotify_SpawnProjectile* Projectile = Cast<UAgAnimNotify_SpawnProjectile>(Event.Notify))
	{
		SpawnProjectile(Payload.MontageInstanceID, *Projectile);
	}
}

void UAgGameplayAbility::HandleMontageNotifyEnd(FName NotifyName, const FBranchingPointNotifyPayload& Payload)
{
	if (IsCurrentMontageNotify(Payload) && Cast<UAgAnimNotifyState_AbilityTask>(Payload.NotifyEvent->NotifyStateClass))
	{
		// 엔진의 End payload는 활성 구간의 복사본일 수 있으므로 배열 주소로 인덱스를 계산하지 않습니다.
		const int32 Index = Motion.Montage->Notifies.IndexOfByKey(*Payload.NotifyEvent);
		EndNotifyTask(Payload.MontageInstanceID, Index);
	}
}

void UAgGameplayAbility::BeginNotifyTask(int32 InstanceId, int32 NotifyIndex, FName Function, FName ParamsId)
{
	if (!OwnsMontage(InstanceId) || bEnding || NotifyIndex == INDEX_NONE)
	{
		return;
	}
	if (Windows.ContainsByPredicate(
	        [=](const FAgAbilityWindowExecution& E)
	        {
		        return E.MontageInstanceId == InstanceId && E.NotifyIndex == NotifyIndex;
	        }))
	{
		return;
	}
	FString Error;
	if (!ExecutionDefinition.ValidateTaskReference(Function, ParamsId, Error))
	{
		UE_LOG(LogAg, Error, TEXT("%s %s (play %d notify %d)"), *GetAgCharacter()->GetCharacterData()->JsonFileName, *Error, InstanceId,
		    NotifyIndex);
		return;
	}
	if (GetAbilityId() == TEXT("DashThrust") && bDashFinished &&
	    (Function == TEXT("Dash") || Function == TEXT("Attack") || Function == TEXT("SetCollisionResponseToChannel")))
	{
		return;
	}
	const FAgAbilityTasks& Tasks = ExecutionDefinition.Tasks;
	UAbilityTask* Task = nullptr;
	if (Function == TEXT("Attack"))
	{
		Task = UAgAbilityTask_Attack::Sweep(this, Tasks.Attack.FindChecked(ParamsId));
	}
	else if (Function == TEXT("HoldTag"))
	{
		Task = UAgAbilityTask_HoldTag::HoldTag(this, Tasks.HoldTag.FindChecked(ParamsId).Tag);
	}
	else if (Function == TEXT("SetCollisionResponseToChannel"))
	{
		const FAgCollisionResponseParams& Params = Tasks.SetCollisionResponseToChannel.FindChecked(ParamsId);
		Task = UAgAbilityTask_SetCollisionResponseToChannel::SetCollisionResponseToChannel(
		    this, GetAgCharacter()->GetCapsuleComponent(), Params.Channel, Params.Response);
	}
	else if (Function == TEXT("TimeStop"))
	{
		Task = UAgAbilityTask_TimeStop::TimeStop(this);
	}
	else if (Function == TEXT("Dash"))
	{
		AActor* Target = GetAgCharacter()->FindAttackTarget();
		if (!Target)
		{
			UE_LOG(LogAg, Error, TEXT("%s Abilities.%s.Tasks.Dash: missing target input (play %d notify %d)"),
			    *GetAgCharacter()->GetCharacterData()->JsonFileName, *GetAbilityId().ToString(), InstanceId, NotifyIndex);
			return;
		}
		DashTask =
		    UAgAbilityTask_Dash::Dash(this, Tasks.Dash, GetAgCharacter()->GetActorForwardVector(), Target->GetActorLocation(), Target);
		if (GetAbilityId() == TEXT("DashThrust"))
		{
			DashTask->OnFinished.AddDynamic(this, &ThisClass::HandleDashFinished);
		}
		Task = DashTask;
	}
	if (!Task)
	{
		return;
	}
	FAgAbilityWindowExecution& Execution = Windows.AddDefaulted_GetRef();
	Execution.Task = Task;
	Execution.Function = Function;
	Execution.MontageInstanceId = InstanceId;
	Execution.NotifyIndex = NotifyIndex;
	Task->ReadyForActivation();
}

void UAgGameplayAbility::EndNotifyTask(int32 InstanceId, int32 NotifyIndex)
{
	const int32 Index = Windows.IndexOfByPredicate(
	    [=](const FAgAbilityWindowExecution& E)
	    {
		    return E.MontageInstanceId == InstanceId && E.NotifyIndex == NotifyIndex;
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

void UAgGameplayAbility::SpawnProjectile(int32 InstanceId, const UAgAnimNotify_SpawnProjectile& Notify)
{
	if (!OwnsMontage(InstanceId) || bEnding)
	{
		return;
	}
	const FAgSpawnProjectileParams* Params = ExecutionDefinition.Tasks.SpawnProjectile.Find(Notify.ParamsId);
	const FAgAttackParams* Hit = Params ? ExecutionDefinition.Tasks.Attack.Find(Params->AttackParamsId) : nullptr;
	const FAgProjectileDefinition* Projectile = Params ? ExecutionProjectiles.Find(Params->ProjectileId) : nullptr;
	if (!Params || !Hit || !Projectile)
	{
		UE_LOG(LogAg, Error, TEXT("%s Abilities.%s.Tasks.SpawnProjectile.%s: unresolved references (play %d)"),
		    *GetAgCharacter()->GetCharacterData()->JsonFileName, *GetAbilityId().ToString(), *Notify.ParamsId.ToString(), InstanceId);
		return;
	}
	UAgAbilityTask_SpawnProjectile::SpawnProjectile(this, InstanceId, *Projectile, *Hit, Notify.Origin, Notify.SocketName)
	    ->ReadyForActivation();
}

bool UAgGameplayAbility::PrepareWarp(
    int32 InstanceId, float WindowStart, FName ParamsId, FAgMotionWarpingParams& Params, AActor*& Target, float& Gap, bool& bFacingOnly)
{
	if (!OwnsMontage(InstanceId) || bEnding)
	{
		return false;
	}
	const FAgMotionWarpingParams* Found = ExecutionDefinition.Tasks.MotionWarping.Find(ParamsId);
	if (!Found)
	{
		return false;
	}
	Params = *Found;
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
