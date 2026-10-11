// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_KnockUp.h"

#include "Abilities/Tasks/AbilityTask_ApplyRootMotionJumpForce.h"
#include "Abilities/Tasks/AbilityTask_WaitMovementModeChange.h"
#include "Abilities/Tasks/AbilityTask_WaitVelocityChange.h"
#include "AbilitySystem/Tasks/AgAbilityTask_IgnoreRootMotion.h"
#include "AbilitySystem/Tasks/AgAbilityTask_PlayReaction.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/AgCharacterBase.h"
#include "Combat/AgCombatLibrary.h"
#include "Core/AgAssetManager.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "Data/AgCombatRules.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	const FName KnockUpLandSection(TEXT("Land"));
}

UAgAbility_KnockUp::UAgAbility_KnockUp()
{
	SetupHitReaction(AgGameplayTags::Ability_Reaction_HitReaction_KnockUp, AgGameplayTags::Event_Hit_KnockUp);
}

UAnimMontage* UAgAbility_KnockUp::GetFallMontage() const
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	return Data ? Data->KnockUpFallMontage.Get() : nullptr;
}

void UAgAbility_KnockUp::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	const UAgCombatRules& Rules = UAgAssetManager::Get().GetCombatRules();
	const UAnimMontage* FallMontage = GetFallMontage();
	AAgCharacterBase* Character = GetAgCharacter();
	if (!Data || !Data->KnockUpMontage || !FallMontage || !Character || Rules.KnockUpAirTime <= 0.f || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	bFalling = false;

	// 공격 쪽으로 돌아, 모션이 공격 반대쪽으로 밀려나게 합니다(전투 시스템 '피격 반응').
	UAgCombatLibrary::TurnToward(Character, TriggerEventData ? TriggerEventData->Instigator.Get() : nullptr);

	ReactionTask = UAgAbilityTask_PlayReaction::PlayReaction(this, Data->KnockUpMontage, Data->GetUpMontage);
	ReactionTask->OnCompleted.AddDynamic(this, &ThisClass::HandleReactionCompleted);
	ReactionTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleReactionInterrupted);
	ReactionTask->ReadyForActivation();
	if (!IsActive())
	{
		return;
	}

	// 반응이 끝날 때까지 모션의 루트 이동을 꺼서, 점프가 캐릭터를 옮기고 착지는 떨어진 자리에서 재생합니다.
	UAgAbilityTask_IgnoreRootMotion::IgnoreRootMotion(this)->ReadyForActivation();

	// 전투 시스템 '넉업': 맞은 자리에서 높이와 체공 시간의 포물선으로 곧게 오르내립니다(중력과 무관).
	// 착지하면 점프가 끝나고, 공중에서 맞았으면 착지할 때까지 경로의 마지막 속도로 떨어집니다.
	const float AirTime = Rules.KnockUpAirTime;
	UAbilityTask_ApplyRootMotionJumpForce* JumpTask = UAbilityTask_ApplyRootMotionJumpForce::ApplyRootMotionJumpForce(this, NAME_None,
		Character->GetActorRotation(), /*Distance*/ 0.f, Rules.KnockUpHeight, AirTime, /*MinimumLandedTriggerTime*/ 0.5f,
		/*bFinishOnLanded*/ true, ERootMotionFinishVelocityMode::SetVelocity, FVector::ZeroVector, 0.f, nullptr, nullptr);
	JumpTask->ReadyForActivation();

	// 점프는 다음 이동 갱신부터 속도를 정하므로, 지금 경로의 첫 속도로 날기 시작해 아래 낙하 검사가 맞기 전 속도를 보지 않게 합니다.
	UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	Movement->SetMovementMode(MOVE_Falling);
	Movement->Velocity = FVector(0.f, 0.f, 4.f * Rules.KnockUpHeight / AirTime);

	// 낙하 자세의 블렌드는 정점이 가운데에 오도록 정점보다 블렌드 시간의 절반 먼저 시작합니다.
	// 경로의 상승은 8 × 높이 ÷ 체공 시간² 의 중력처럼 느려집니다.
	const float PathGravity = 8.f * Rules.KnockUpHeight / FMath::Square(AirTime);
	const float FallBlendSpeed = PathGravity * FallMontage->BlendIn.GetBlendTime() * 0.5f;
	UAbilityTask_WaitVelocityChange* FallTask = UAbilityTask_WaitVelocityChange::CreateWaitVelocityChange(this, FVector::DownVector, -FallBlendSpeed);
	FallTask->OnVelocityChage.AddDynamic(this, &ThisClass::HandleFalling);
	FallTask->ReadyForActivation();

	UAbilityTask_WaitMovementModeChange* LandTask = UAbilityTask_WaitMovementModeChange::CreateWaitMovementModeChange(this, MOVE_Walking);
	LandTask->OnChange.AddDynamic(this, &ThisClass::HandleLanded);
	LandTask->ReadyForActivation();
}

void UAgAbility_KnockUp::HandleFalling()
{
	if (!bFalling)
	{
		bFalling = true;
		ReactionTask->PlayMontage(GetFallMontage());
	}
}

void UAgAbility_KnockUp::HandleLanded(EMovementMode NewMovementMode)
{
	// 낙하 자세와 착지 모션의 첫 자세가 같아, Land로 넘어가는 것이 보이지 않습니다.
	// 낙하 자세가 시작되기 전에 착지하면 몽타주를 Land부터 재생합니다.
	UAnimMontage* FallMontage = GetFallMontage();
	UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance();
	if (bFalling && AnimInstance && AnimInstance->Montage_IsPlaying(FallMontage))
	{
		AnimInstance->Montage_JumpToSection(KnockUpLandSection, FallMontage);
		return;
	}
	bFalling = true;
	ReactionTask->PlayMontage(FallMontage, KnockUpLandSection);
}

void UAgAbility_KnockUp::HandleReactionCompleted()
{
	EndSelf(false);
}

void UAgAbility_KnockUp::HandleReactionInterrupted()
{
	EndSelf(true);
}
