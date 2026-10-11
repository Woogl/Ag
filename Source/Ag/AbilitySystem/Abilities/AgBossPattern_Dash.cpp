// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgBossPattern_Dash.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/Tasks/AgAbilityTask_Dash.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Character/AgBossCharacter.h"
#include "Combat/AgCombatLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"

void UAgBossPattern_Dash::OnPatternStarted()
{
	bDashing = bHolding = bRootMotionIgnored = false;
	DashTask = nullptr;

	UAbilityTask_WaitGameplayEvent* StartTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Event_Boss_DashStart, nullptr, /*OnlyTriggerOnce*/ true);
	StartTask->EventReceived.AddDynamic(this, &ThisClass::HandleDashStart);
	StartTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* HoldTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Event_Boss_DashHold, nullptr, /*OnlyTriggerOnce*/ true);
	HoldTask->EventReceived.AddDynamic(this, &ThisClass::HandleDashHold);
	HoldTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* BlockedTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Event_AttackBlocked, nullptr, /*OnlyTriggerOnce*/ true);
	BlockedTask->EventReceived.AddDynamic(this, &ThisClass::HandleAttackBlocked);
	BlockedTask->ReadyForActivation();
}

void UAgBossPattern_Dash::HandleDashStart(FGameplayEventData Payload)
{
	AAgBossCharacter* Boss = GetBoss();
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	const AActor* Target = GetTarget();
	if (!Boss || !Data || !Target)
	{
		return;
	}

	// 방향: 이 순간의 대상 쪽으로 고정합니다. 목표 지점: 대상을 지나, 최대 이동 거리까지.
	const FVector Direction = UAgCombatLibrary::GetAttackDirection(Boss, Target);
	if (!Direction.IsNearlyZero())
	{
		Boss->SetActorRotation(FRotator(0.f, Direction.Rotation().Yaw, 0.f));
	}
	const float Distance = FMath::Min(UAgCombatLibrary::GetHorizontalDistance(Boss, Target) + Data->DashOvershoot, Data->DashMaxDistance);

	// 몽타주가 루트 모션을 주는 동안 캐릭터 무브먼트는 다른 루트 모션을 쓰지 않으므로, 여기서부터 모션 자체의 루트 모션은
	// 무시합니다. 돌진이 보스를 움직이고, 나머지 모션은 창을 거두기만 합니다.
	if (UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance())
	{
		bRootMotionIgnored = true;
		SavedRootMotionMode = AnimInstance->RootMotionMode;
		AnimInstance->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
	}

	DashTask = UAgAbilityTask_Dash::Dash(this, Boss->GetActorForwardVector(), Data->DashSpeed * Boss->GetPatternSpeed(), Distance);
	DashTask->OnFinished.AddDynamic(this, &ThisClass::HandleDashFinished);
	DashTask->ReadyForActivation();

	// 판정 구간은 돌진 태스크가 생긴 뒤에 엽니다. 여는 순간의 판정이 가드·패리에 막히면 그 자리에서 돌진을 끝낼 수 있어야
	// 하기 때문입니다. 돌진이 시작하자마자 끝났으면(HandleDashFinished가 DashTask를 비움) 열지 않습니다.
	if (DashTask)
	{
		SetDashing(true);
	}
}

void UAgBossPattern_Dash::HandleDashHold(FGameplayEventData Payload)
{
	// The thrust pose holds until the dash ends.
	UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance();
	if (bDashing && AnimInstance)
	{
		bHolding = true;
		AnimInstance->Montage_Pause(GetMontage());
	}
}

void UAgBossPattern_Dash::HandleDashFinished()
{
	DashTask = nullptr;
	SetDashing(false);

	UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance();
	if (bHolding && AnimInstance)
	{
		bHolding = false;
		AnimInstance->Montage_Resume(GetMontage());
	}
}

void UAgBossPattern_Dash::HandleAttackBlocked(FGameplayEventData Payload)
{
	// 가드·패리에 막히면 벽에 막혔을 때처럼 그 자리에서 멈춥니다.
	if (bDashing && DashTask)
	{
		DashTask->Finish();
	}
}

void UAgBossPattern_Dash::SetDashing(bool bNewDashing)
{
	AAgCharacterBase* Boss = GetAgCharacter();
	if (bDashing == bNewDashing || !Boss)
	{
		return;
	}
	bDashing = bNewDashing;

	// No turning during the dash, and it passes through the player.
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (bDashing)
	{
		ASC->AddLooseGameplayTag(AgGameplayTags::State_Boss_RotationLocked);
		Boss->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		Boss->BeginAttackWindow(0);
	}
	else
	{
		ASC->RemoveLooseGameplayTag(AgGameplayTags::State_Boss_RotationLocked);
		if (!Boss->IsDead())
		{
			Boss->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		}
		Boss->EndAttackWindow(0);
	}
}

void UAgBossPattern_Dash::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	SetDashing(false);
	if (bRootMotionIgnored)
	{
		bRootMotionIgnored = false;
		if (UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance())
		{
			AnimInstance->SetRootMotionMode(SavedRootMotionMode);
		}
	}
	if (bHolding)
	{
		bHolding = false;
		if (UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance())
		{
			AnimInstance->Montage_Resume(GetMontage());
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
