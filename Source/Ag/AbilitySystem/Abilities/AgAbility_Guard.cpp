// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_Guard.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/AgEffect_ParryPPLoss.h"
#include "AbilitySystem/AgEffect_Resource.h"
#include "AbilitySystem/AgEffect_TimedTag.h"
#include "AbilitySystem/Tasks/AgAbilityTask_HoldTag.h"
#include "AbilitySystemComponent.h"
#include "Character/AgPlayerCharacter.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"

UAgAbility_Guard::UAgAbility_Guard()
{
	// 가드 중에도 움직일 수 있어 State.Acting을 갖지 않습니다.
	SetupPlayerAction(AgGameplayTags::Ability_Action_Guard, /*bIgnoresMovement*/ false);

	// 가드 키를 누른 채 가드 밀림이 끝나면 플레이어 캐릭터가 이 이벤트로 가드를 다시 시작합니다(패리 구간 없음).
	FAbilityTriggerData& Trigger = AbilityTriggers.AddDefaulted_GetRef();
	Trigger.TriggerTag = AgGameplayTags::Event_Guard_Resume;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
}

void UAgAbility_Guard::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const AAgPlayerCharacter* Player = CastChecked<AAgPlayerCharacter>(GetAgCharacter());
	if (!Player->IsGuardHeld() || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilityTask_WaitGameplayEvent* PressedTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Input_Guard);
	PressedTask->EventReceived.AddDynamic(this, &ThisClass::HandleGuardPressed);
	PressedTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* ReleasedTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Input_GuardReleased);
	ReleasedTask->EventReceived.AddDynamic(this, &ThisClass::HandleGuardReleased);
	ReleasedTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* ParryEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Event_Parry);
	ParryEventTask->EventReceived.AddDynamic(this, &ThisClass::HandleParry);
	ParryEventTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* RecoveryTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Event_RecoveryStarted);
	RecoveryTask->EventReceived.AddDynamic(this, &ThisClass::HandleParryRecoveryStarted);
	RecoveryTask->ReadyForActivation();

	// 패리 구간: 새 가드 시작이 엽니다. 패리 모션이나 가드 밀림 뒤에 가드로 돌아올 때는 열지 않습니다.
	const bool bResumedAfterPushback = TriggerEventData && TriggerEventData->EventTag == AgGameplayTags::Event_Guard_Resume;
	StartGuard(!bResumedAfterPushback);
}

void UAgAbility_Guard::StartGuard(bool bOpenParryWindow)
{
	SetGuarding(true);
	CloseParryWindow();
	CloseMoveLock();
	const FAgGuardParams* Params = GetActionParams<FAgGuardParams>();
	if (bOpenParryWindow && Params)
	{
		// 패리 구간이 생기는 가드 시작은 이동할 수 없는 시간도 함께 시작합니다(플레이어 입력 '상태별 허용 입력').
		ParryWindow = UAgEffect_TimedTag::Apply(GetAbilitySystemComponentFromActorInfo(), AgGameplayTags::State_ParryWindow, Params->ParryWindowTime);
		MoveLock = UAgEffect_TimedTag::Apply(GetAbilitySystemComponentFromActorInfo(), AgGameplayTags::State_GuardMoveLock, Params->MoveLockTime);
	}
}

void UAgAbility_Guard::SetGuarding(bool bGuarding)
{
	if (bGuarding && !GuardingTag)
	{
		GuardingTag = UAgAbilityTask_HoldTag::HoldTag(this, AgGameplayTags::State_Guarding);
		GuardingTag->ReadyForActivation();
	}
	else if (!bGuarding && GuardingTag)
	{
		GuardingTag->EndTask();
		GuardingTag = nullptr;
	}
}

void UAgAbility_Guard::CloseParryWindow()
{
	if (ParryWindow.IsValid())
	{
		GetAbilitySystemComponentFromActorInfo()->RemoveActiveGameplayEffect(ParryWindow);
		ParryWindow.Invalidate();
	}
}

void UAgAbility_Guard::CloseMoveLock()
{
	if (MoveLock.IsValid())
	{
		GetAbilitySystemComponentFromActorInfo()->RemoveActiveGameplayEffect(MoveLock);
		MoveLock.Invalidate();
	}
}

void UAgAbility_Guard::HandleGuardPressed(FGameplayEventData Payload)
{
	if (!ParryMotionTag)
	{
		return;
	}

	// 패리 모션 중에는 가드가 키를 따릅니다. 후딜에 새로 누르면 패리 모션을 모션 캔슬하고 가드를 새로 시작합니다(플레이어 사양 '가드 시작').
	SetGuarding(true);
	if (!IsBusy())
	{
		CancelParryMotion();
		StartGuard(/*bOpenParryWindow*/ true);
	}
}

void UAgAbility_Guard::HandleGuardReleased(FGameplayEventData Payload)
{
	// 패리 모션 중에 떼면 가드만 풀리고 모션은 이어집니다.
	if (ParryMotionTag)
	{
		SetGuarding(false);
		return;
	}
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UAgAbility_Guard::HandleParry(FGameplayEventData Payload)
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	const FAgGuardParams* Params = GetActionParams<FAgGuardParams>();
	if (ParryMotionTag || !Params)
	{
		return;
	}

	// 패리 보상(플레이어 사양 '패리'): 막은 쪽은 MP·UP를 얻고, 공격한 쪽(대미지 GE 컨텍스트의 Instigator)은 PP를 잃습니다.
	// 공격한 쪽의 그로기 판정은 그쪽 어트리뷰트 세트가 합니다.
	const FGameplayEffectSpecHandle Charge = MakeOutgoingGameplayEffectSpec(UAgEffect_Resource::StaticClass());
	UAgEffect_Resource::SetAmounts(*Charge.Data, 0.f, Params->ParryMPCharge, Params->ParryUPCharge);
	ApplyGameplayEffectSpecToOwner(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), Charge);
	if (const AAgCharacterBase* AttackingCharacter = Cast<AAgCharacterBase>(Payload.ContextHandle.GetInstigator()))
	{
		const FGameplayEffectSpecHandle PPLoss = MakeOutgoingGameplayEffectSpec(UAgEffect_ParryPPLoss::StaticClass());
		GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(*PPLoss.Data, AttackingCharacter->GetAbilitySystemComponent());
	}
	if (!Data->ParryMontage)
	{
		return;
	}

	// 패리 모션은 공격이 온 쪽을 바라봅니다(전투 시스템 '피격 반응').
	AAgCharacterBase* Character = GetAgCharacter();
	if (const AActor* Attacker = Payload.Instigator.Get())
	{
		const FVector ToAttacker = (Attacker->GetActorLocation() - Character->GetActorLocation()).GetSafeNormal2D();
		if (!ToAttacker.IsNearlyZero())
		{
			Character->SetActorRotation(FRotator(0.f, ToAttacker.Rotation().Yaw, 0.f));
		}
	}

	// 패리 모션이 시작되면 패리 구간이 끝납니다. 패리 모션은 행동이라 후딜 전까지 다른 행동을 받지 않고,
	// 모션이 끝날 때까지 이동 입력을 받지 않습니다.
	CloseParryWindow();
	BeginBusy();
	ParryMotionTag = UAgAbilityTask_HoldTag::HoldTag(this, AgGameplayTags::State_Acting);
	ParryMotionTag->ReadyForActivation();

	ParryTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Data->ParryMontage);
	ParryTask->OnCompleted.AddDynamic(this, &ThisClass::HandleParryMotionFinished);
	ParryTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleParryMotionCancelled);
	ParryTask->OnCancelled.AddDynamic(this, &ThisClass::HandleParryMotionCancelled);
	ParryTask->ReadyForActivation();
}

void UAgAbility_Guard::HandleParryRecoveryStarted(FGameplayEventData Payload)
{
	if (!ParryMotionTag || Payload.OptionalObject != GetCurrentMontage())
	{
		return;
	}
	EndBusy();

	// 가드 키를 누른 채 후딜이 시작되면 패리 모션을 모션 캔슬하고 패리 구간 없이 가드로 돌아갑니다
	// (플레이어 사양 '패리 모션 중·종료 후').
	if (GuardingTag)
	{
		CancelParryMotion();
		StartGuard(/*bOpenParryWindow*/ false);
	}
}

void UAgAbility_Guard::HandleParryMotionFinished()
{
	// 가드 키를 누르고 있으면 패리 구간 없이 가드로, 아니면 대기로 돌아갑니다.
	ParryTask = nullptr;
	EndParryMotion();
	if (!GuardingTag)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UAgAbility_Guard::HandleParryMotionCancelled()
{
	ParryTask = nullptr;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UAgAbility_Guard::CancelParryMotion()
{
	// 몽타주 태스크를 끝내는 것만으로는 몽타주가 멈추지 않으므로 직접 멈춥니다. 태스크를 먼저 끝내야, 몽타주가 멈출 때
	// 태스크의 OnInterrupted가 가드 자체를 끝내지 않습니다.
	if (ParryTask)
	{
		ParryTask->EndTask();
		ParryTask = nullptr;
	}
	MontageStop();
	EndParryMotion();
}

void UAgAbility_Guard::EndParryMotion()
{
	if (ParryMotionTag)
	{
		ParryMotionTag->EndTask();
		ParryMotionTag = nullptr;
		EndBusy();
	}
}

void UAgAbility_Guard::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 패리 구간과 이동할 수 없는 시간은 효과라 어빌리티가 끝나도 남으므로 지웁니다. 태그를 붙여 둔 태스크는 어빌리티와 함께 끝납니다.
	CloseParryWindow();
	CloseMoveLock();
	ParryTask = nullptr;
	GuardingTag = nullptr;
	ParryMotionTag = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
