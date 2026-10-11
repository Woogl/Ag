// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgBossPattern_SwordWave.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/Tasks/AgAbilityTask_AttackMovement.h"
#include "AbilitySystem/Tasks/AgAbilityTask_HoldTag.h"
#include "AbilitySystem/Tasks/AgAbilityTask_WaitMontagePosition.h"
#include "Ag.h"
#include "Animation/AgAnimNotify_GameplayEvent.h"
#include "Animation/AgAnimNotifyState_AttackWindow.h"
#include "Animation/AnimMontage.h"
#include "Character/AgBossCharacter.h"
#include "Combat/AgSwordWave.h"
#include "Components/CapsuleComponent.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgAttackData.h"
#include "Data/AgCharacterData.h"
#include "Engine/World.h"

UAgBossPattern_SwordWave::UAgBossPattern_SwordWave()
{
	SetupBossPattern();
}

void UAgBossPattern_SwordWave::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AAgBossCharacter* Boss = Cast<AAgBossCharacter>(GetAgCharacter());
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	const FAgBossPattern* Pattern = Boss ? Boss->FindPattern(Handle) : nullptr;
	Montage = (Data && Pattern) ? Data->FindPatternMontage(Pattern->Pattern) : nullptr;
	if (!Montage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogAg, Verbose, TEXT("Boss pattern %s starts (play rate %.2f)"), *Pattern->Pattern.ToString(), Boss->GetPatternPlayRate());
	if (AAgCharacterBase* Target = Boss->GetTarget())
	{
		// 정지 간격과 접근은 공용 패턴과 같은 규칙입니다(UAgBossPattern_Common).
		const float MaxTravel = UAgAnimNotifyState_AttackWindow::GetTravelBeforeFirstWindow(Montage) + Pattern->MaxApproachDistance;
		UAgAbilityTask_AttackMovement::KeepGap(this, Target, Data->StopDistance, MaxTravel)->ReadyForActivation();
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage, Boss->GetPatternPlayRate());
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageFinished);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::HandleMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->ReadyForActivation();

	// 몽타주를 재생하지 못하면 OnCancelled가 그 자리에서 어빌리티를 끝냅니다. 끝난 어빌리티에서 발사를 기다리지 않습니다.
	if (!IsActive())
	{
		return;
	}

	WaveIndex = 0;
	RotationLockTag = nullptr;
	UAbilityTask_WaitGameplayEvent* ReleaseTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Event_Boss_SwordWave, nullptr, /*OnlyTriggerOnce*/ false);
	ReleaseTask->EventReceived.AddDynamic(this, &ThisClass::HandleRelease);
	ReleaseTask->ReadyForActivation();

	ReleaseTimes.Reset();
	for (const FAnimNotifyEvent& Notify : Montage->Notifies)
	{
		const UAgAnimNotify_GameplayEvent* Event = Cast<UAgAnimNotify_GameplayEvent>(Notify.Notify);
		if (Event && Event->EventTag == AgGameplayTags::Event_Boss_SwordWave)
		{
			ReleaseTimes.Add(Notify.GetTriggerTime());
		}
	}
	ReleaseTimes.Sort();
	WaitRotationStop();
}

void UAgBossPattern_SwordWave::WaitRotationStop()
{
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	if (!Data || !ReleaseTimes.IsValidIndex(WaveIndex))
	{
		return;
	}
	UAgAbilityTask_WaitMontagePosition* StopTask = UAgAbilityTask_WaitMontagePosition::WaitMontagePosition(this, Montage, ReleaseTimes[WaveIndex] - Data->GetRotationStopLeadInMontage());
	StopTask->OnReached.AddDynamic(this, &ThisClass::HandleRotationStop);
	StopTask->ReadyForActivation();
}

void UAgBossPattern_SwordWave::HandleRotationStop()
{
	if (!RotationLockTag)
	{
		RotationLockTag = UAgAbilityTask_HoldTag::HoldTag(this, AgGameplayTags::State_Boss_RotationLocked);
		RotationLockTag->ReadyForActivation();
	}
}

void UAgBossPattern_SwordWave::HandleRelease(FGameplayEventData Payload)
{
	// 쏘고 나면 다시 돌다가 다음 발사 전에 멈춥니다.
	const int32 Index = WaveIndex++;
	if (RotationLockTag)
	{
		RotationLockTag->EndTask();
		RotationLockTag = nullptr;
	}
	WaitRotationStop();

	AAgBossCharacter* Boss = Cast<AAgBossCharacter>(GetAgCharacter());
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	const FAgBossPattern* Pattern = Boss ? Boss->FindPattern(GetCurrentAbilitySpecHandle()) : nullptr;
	if (!Data || !Data->SwordWaveClass || !Pattern || !Pattern->Hits.IsValidIndex(Index))
	{
		return;
	}

	// 보스 앞의 바닥에서 정면으로 쏩니다.
	const FVector Direction = Boss->GetActorForwardVector().GetSafeNormal2D();
	const UCapsuleComponent* Capsule = Boss->GetCapsuleComponent();
	const FVector Location = Boss->GetActorLocation() + Direction * Capsule->GetScaledCapsuleRadius() - FVector(0.f, 0.f, Capsule->GetScaledCapsuleHalfHeight());

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = Boss;
	SpawnParameters.Instigator = Boss;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AAgSwordWave* Wave = GetWorld()->SpawnActor<AAgSwordWave>(Data->SwordWaveClass, Location, Direction.Rotation(), SpawnParameters))
	{
		Wave->Launch(Boss, Pattern->Hits[Index], Direction, Data->SwordWaveSpeed, Data->SwordWaveRange);
	}
}

void UAgBossPattern_SwordWave::HandleMontageFinished()
{
	EndSelf(false);
}

void UAgBossPattern_SwordWave::HandleMontageCancelled()
{
	EndSelf(true);
}
