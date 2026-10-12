// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_Ultimate.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/AgEffect_Cost.h"
#include "AbilitySystem/AgEffect_Damage.h"
#include "AbilitySystem/Tasks/AgAbilityTask_PlayCutscene.h"
#include "AbilitySystem/Tasks/AgAbilityTask_TimeStop.h"
#include "AbilitySystem/Tasks/AgAbilityTask_WaitMontagePosition.h"
#include "AbilitySystemComponent.h"
#include "Ag.h"
#include "Animation/AnimMontage.h"
#include "Character/AgPlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"

namespace
{
	/** 궁극기 몽타주에서 컷신이 끝나면 넘어가는 베기 단계(칼 넣기) 섹션 */
	const FName JudgmentSection(TEXT("Judgment"));

	/** 베기 선이 나오는 방향의 폭: 카메라가 보는 방향에서 좌우로 이만큼(도). 카메라 시야각(90°) 안쪽이라 화면에 들어옵니다. */
	constexpr float SlashHalfAngle = 35.f;
}

UAgAbility_Ultimate::UAgAbility_Ultimate()
{
	SetupPlayerAction(AgGameplayTags::Ability_Action_Ultimate);
	CostGameplayEffectClass = UAgEffect_Cost::StaticClass();

	// 보호: 시작부터 끝까지 무적이고, 후딜에서 모션 캔슬로 끝나도 풀립니다.
	ActivationOwnedTags.AddTag(AgGameplayTags::State_Invincible);
}

void UAgAbility_Ultimate::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	if (!Data->UltimateMontage || !Data->UltimateCutscene || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	SlashIndex = 0;

	// 마무리 전까지는 어떤 행동도 궁극기를 끊지 못합니다.
	BeginBusy();

	// 시간 정지: 시작부터 마무리까지 플레이어만 정상 속도로 움직입니다.
	TimeStopTask = UAgAbilityTask_TimeStop::TimeStop(this);
	TimeStopTask->ReadyForActivation();

	// 모션: 컷신 동안 발도 자세로 기다리다가(Start → Hold 반복), 컷신이 끝나면 Judgment로 넘깁니다.
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Data->UltimateMontage);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* FinishTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Event_UltimateFinish);
	FinishTask->EventReceived.AddDynamic(this, &ThisClass::HandleFinish);
	FinishTask->ReadyForActivation();

	// 궁극기 컷신: 카메라만 담은 시퀀스를 플레이어 위치·방향을 기준으로 재생합니다. HUD와 보스의 월드 위젯은 컷신의
	// 시네마틱 모드를 보고 숨습니다. 시간 정지를 건 뒤에 시작해야 컷신이 정상 속도로 재생됩니다.
	UAgAbilityTask_PlayCutscene* CutsceneTask = UAgAbilityTask_PlayCutscene::PlayCutscene(this, Data->UltimateCutscene, GetAgCharacter());
	CutsceneTask->OnFinished.AddDynamic(this, &ThisClass::HandleCutsceneFinished);
	CutsceneTask->ReadyForActivation();
}

void UAgAbility_Ultimate::HandleCutsceneFinished()
{
	MontageJumpToSection(JudgmentSection);

	// 베기 선 큐: 켜 두고 베기 선마다 실행하며, 마무리에 끕니다. 마무리 전에 어빌리티가 끝나면 엔진이 끕니다.
	K2_AddGameplayCue(AgGameplayTags::GameplayCue_UltimateSlashes, FGameplayEffectContextHandle(), /*bRemoveOnAbilityEnd*/ true);
	WaitNextSlash();
}

void UAgAbility_Ultimate::WaitNextSlash()
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	if (SlashIndex >= Data->UltimateSlashCount)
	{
		return;
	}
	// 몽타주 위치로 세므로 플레이어의 시간(시간 정지 보정, 히트스톱)을 따릅니다.
	const UAnimMontage* Montage = Data->UltimateMontage;
	float Start = 0.f;
	float End = 0.f;
	Montage->GetSectionStartAndEndTime(Montage->GetSectionIndex(JudgmentSection), Start, End);
	const float Position = Start + (End - Start) * SlashIndex / Data->UltimateSlashCount;
	UAgAbilityTask_WaitMontagePosition* SlashTask = UAgAbilityTask_WaitMontagePosition::WaitMontagePosition(this, Data->UltimateMontage, Position);
	SlashTask->OnReached.AddDynamic(this, &ThisClass::HandleSlashTime);
	SlashTask->ReadyForActivation();
}

void UAgAbility_Ultimate::HandleSlashTime()
{
	SpawnSlash();
	++SlashIndex;
	WaitNextSlash();
}

void UAgAbility_Ultimate::SpawnSlash()
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	const AAgCharacterBase* Character = GetAgCharacter();

	// 위치: 카메라가 보는 쪽으로 플레이어 앞 일정 거리, 바닥에서 일정 높이 안의 무작위 위치입니다.
	// 방향: 카메라를 마주 보는 면 안의 무작위 방향이라, 카메라 쪽을 향해 점처럼 보이지 않습니다.
	const float ViewYaw = Character->GetControlRotation().Yaw;
	const float Yaw = ViewYaw + FMath::FRandRange(-SlashHalfAngle, SlashHalfAngle);
	const float Distance = FMath::FRandRange(Data->UltimateSlashDistance.Min, Data->UltimateSlashDistance.Max);
	const float Height = FMath::FRandRange(Data->UltimateSlashHeight.Min, Data->UltimateSlashHeight.Max);
	const FVector Feet = Character->GetActorLocation() - FVector(0.f, 0.f, Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	const float Angle = FMath::DegreesToRadians(FMath::FRandRange(0.f, 180.f));

	FGameplayCueParameters Parameters;
	Parameters.Location = Feet + FRotator(0.f, Yaw, 0.f).Vector() * Distance + FVector(0.f, 0.f, Height);
	Parameters.Normal = FRotator(0.f, ViewYaw + 90.f, 0.f).Vector() * FMath::Cos(Angle) + FVector::UpVector * FMath::Sin(Angle);
	K2_ExecuteGameplayCueWithParams(AgGameplayTags::GameplayCue_UltimateSlashes, Parameters);
}

void UAgAbility_Ultimate::HandleFinish(FGameplayEventData Payload)
{
	// 마무리: 베기 선이 모두 사라지고 시간 정지가 끝난 뒤, 범위 안의 적마다 1타를 적용합니다. 시간이 다시 흐른 뒤에 적용해야
	// 적중 후 처리의 히트스톱과 슬로우모션이 정상 시간 위에서 시작합니다. 여기서부터 후딜(칼 뽑기)입니다.
	K2_RemoveGameplayCue(AgGameplayTags::GameplayCue_UltimateSlashes);
	TimeStopTask->EndTask();
	EndBusy();

	// 이 어빌리티가 만든 스펙에는 엔진이 어빌리티 태그(Ability.Action.Ultimate)를 넣어, 대미지 GE가 궁극기 마무리 이펙트와
	// 슬로우모션을 고릅니다. 대미지 숫자는 적의 캡슐 중심에 뜹니다.
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	AAgPlayerCharacter* Player = CastChecked<AAgPlayerCharacter>(GetAgCharacter());
	FAgPlayerAttackHit Hit = Data->UltimateHit;
	Hit.bFinisher = true;
	for (AAgCharacterBase* Enemy : Player->FindEnemies(Data->UltimateRadius))
	{
		const FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(UAgEffect_Damage::StaticClass());
		UAgEffect_Damage::SetAttack(*Spec.Data, Hit, Enemy->GetActorLocation());
		GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(*Spec.Data, Enemy->GetAbilitySystemComponent());
		UE_LOG(LogAg, Verbose, TEXT("Ultimate finish: %s, HP now %.0f"), *Enemy->GetName(), Enemy->GetAttributeSet()->GetHP());
	}
}

void UAgAbility_Ultimate::HandleMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UAgAbility_Ultimate::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

bool UAgAbility_Ultimate::CanPayCost(const UAgAttributeSet& Stats) const
{
	// UP가 가득 찼을 때만 쓸 수 있습니다(플레이어 사양 '궁극기').
	return Stats.GetMaxUP() > 0.f && Stats.GetUP() >= Stats.GetMaxUP();
}

FAgAbilityCost UAgAbility_Ultimate::GetCost() const
{
	// UP를 모두 씁니다.
	return { 0.f, 0.f, GetAgCharacter()->GetAttributeSet()->GetUP() };
}
