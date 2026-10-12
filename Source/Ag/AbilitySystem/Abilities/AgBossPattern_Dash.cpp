// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgBossPattern_Dash.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/Tasks/AgAbilityTask_Dash.h"
#include "AbilitySystem/Tasks/AgAbilityTask_IgnoreCollision.h"
#include "AbilitySystem/Tasks/AgAbilityTask_IgnoreRootMotion.h"
#include "Ag.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/AgBossCharacter.h"
#include "Combat/AgCombatLibrary.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"

UAgBossPattern_Dash::UAgBossPattern_Dash()
{
	SetupBossPattern();
}

void UAgBossPattern_Dash::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AAgBossCharacter* Boss = CastChecked<AAgBossCharacter>(GetAgCharacter());
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	const FAgBossPattern* Pattern = Boss->FindPattern(Handle);
	UAnimMontage* Montage = Pattern ? Data->FindPatternMontage(Pattern->Id) : nullptr;
	if (!Montage || !GetPatternParams<FAgBossDashParams>() || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogAg, Verbose, TEXT("Boss pattern %s starts (play rate %.2f)"), *Pattern->Id.ToString(), Data->PatternPlayRate);
	bHolding = false;
	DashTask = nullptr;
	IgnoreCollisionTask = nullptr;
	Boss->SetActiveAttack(Pattern->Hits);

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage, Data->PatternPlayRate);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->ReadyForActivation();

	// 몽타주를 재생하지 못하면 OnCancelled가 그 자리에서 어빌리티를 끝냅니다. 끝난 어빌리티에서 돌진을 기다리지 않습니다.
	if (!IsActive())
	{
		return;
	}

	UAbilityTask_WaitGameplayEvent* StartTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Event_Boss_DashStart, nullptr, /*OnlyTriggerOnce*/ true);
	StartTask->EventReceived.AddDynamic(this, &ThisClass::HandleDashStart);
	StartTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* HoldTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Event_Boss_DashHold, nullptr, /*OnlyTriggerOnce*/ true);
	HoldTask->EventReceived.AddDynamic(this, &ThisClass::HandleDashHold);
	HoldTask->ReadyForActivation();
}

void UAgBossPattern_Dash::HandleDashStart(FGameplayEventData Payload)
{
	AAgBossCharacter* Boss = CastChecked<AAgBossCharacter>(GetAgCharacter());
	const FAgBossDashParams* Params = GetPatternParams<FAgBossDashParams>();
	const AActor* Target = Boss->GetTarget();
	if (!Target || !Params)
	{
		return;
	}

	// 방향: 이 순간의 대상 쪽으로 고정합니다. 목표 지점: 대상을 지나, 최대 이동 거리까지.
	const FVector Direction = UAgCombatLibrary::GetHorizontalDirection(Boss, Target);
	if (!Direction.IsNearlyZero())
	{
		Boss->SetActorRotation(FRotator(0.f, Direction.Rotation().Yaw, 0.f));
	}
	const float Distance = FMath::Min(UAgCombatLibrary::GetHorizontalDistance(Boss, Target) + Params->Overshoot, Params->MaxDistance);

	// 몽타주가 루트 모션을 주는 동안 캐릭터 무브먼트는 다른 루트 모션을 쓰지 않으므로, 여기서부터 모션 자체의 루트 모션은
	// 무시합니다. 돌진이 보스를 움직이고, 나머지 모션은 창을 거두기만 합니다.
	UAgAbilityTask_IgnoreRootMotion::IgnoreRootMotion(this)->ReadyForActivation();

	DashTask = UAgAbilityTask_Dash::Dash(this, Boss->GetActorForwardVector(), Params->Speed, Distance);
	DashTask->OnFinished.AddDynamic(this, &ThisClass::HandleDashFinished);
	DashTask->ReadyForActivation();

	// 돌진이 시작하자마자 끝났으면(HandleDashFinished가 DashTask를 비움) 판정 구간을 열지 않습니다.
	if (!DashTask)
	{
		return;
	}

	// 막힘은 돌진을 시작한 뒤부터 기다립니다. 검기를 막아도 보스에게 막힘 이벤트가 오므로, 그 전부터 기다리면 돌진 전에
	// 대기가 끝나 버립니다.
	UAbilityTask_WaitGameplayEvent* BlockedTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Event_AttackBlocked, nullptr, /*OnlyTriggerOnce*/ true);
	BlockedTask->EventReceived.AddDynamic(this, &ThisClass::HandleAttackBlocked);
	BlockedTask->ReadyForActivation();

	// 돌진하는 동안이 판정 구간이라 회전하지 않고, 플레이어를 뚫고 지나갑니다. 판정 구간을 열자마자 막히면 그 안에서 돌진이
	// 끝나므로, 막힘 대기와 충돌 무시를 판정 구간보다 먼저 겁니다.
	IgnoreCollisionTask = UAgAbilityTask_IgnoreCollision::IgnoreCollision(this, ECC_Pawn);
	IgnoreCollisionTask->ReadyForActivation();
	Boss->BeginAttackWindow(0);
}

void UAgBossPattern_Dash::HandleDashHold(FGameplayEventData Payload)
{
	// 창을 내지른 자세는 돌진이 끝날 때까지 유지합니다.
	// 몽타주를 넘기지 않으면 모든 몽타주가 멈추므로, 이 어빌리티가 재생 중인 몽타주가 있을 때만 멈춥니다.
	UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance();
	UAnimMontage* Montage = GetCurrentMontage();
	if (DashTask && AnimInstance && Montage)
	{
		bHolding = true;
		AnimInstance->Montage_Pause(Montage);
	}
}

void UAgBossPattern_Dash::HandleDashFinished()
{
	// 돌진이 끝나면 판정 구간을 닫고 다시 플레이어를 막으며, 멈춰 둔 자세에서 창을 거둡니다.
	DashTask = nullptr;
	if (IgnoreCollisionTask)
	{
		IgnoreCollisionTask->EndTask();
		IgnoreCollisionTask = nullptr;
	}
	GetAgCharacter()->EndAttackWindow(0);

	UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance();
	UAnimMontage* Montage = GetCurrentMontage();
	if (bHolding && AnimInstance && Montage)
	{
		bHolding = false;
		AnimInstance->Montage_Resume(Montage);
	}
}

void UAgBossPattern_Dash::HandleAttackBlocked(FGameplayEventData Payload)
{
	// 가드·패리에 막히면 벽에 막혔을 때처럼 그 자리에서 멈춥니다.
	if (DashTask)
	{
		DashTask->Finish();
	}
}

void UAgBossPattern_Dash::HandleMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UAgBossPattern_Dash::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UAgBossPattern_Dash::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bHolding)
	{
		bHolding = false;
		UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance();
		if (UAnimMontage* Montage = GetCurrentMontage(); AnimInstance && Montage)
		{
			AnimInstance->Montage_Resume(Montage);
		}
	}

	// 판정 구간은 공격 데이터와 함께 지워집니다. 충돌 무시, 몽타주의 루트 모션 무시, 돌진은 태스크가 어빌리티와 함께 끝나며
	// 되돌립니다.
	GetAgCharacter()->ClearActiveAttack();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
