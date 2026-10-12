// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgBossPattern_Leap.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitMovementModeChange.h"
#include "Abilities/Tasks/AbilityTask_WaitVelocityChange.h"
#include "AbilitySystem/AgEffect_Damage.h"
#include "AbilitySystem/Tasks/AgAbilityTask_HoldTag.h"
#include "Ag.h"
#include "Animation/AnimMontage.h"
#include "Character/AgBossCharacter.h"
#include "Combat/AgCombatLibrary.h"
#include "Core/AgAssetManager.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgAttackData.h"
#include "Data/AgCameraData.h"
#include "Data/AgCharacterData.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const FName LeapEndSection(TEXT("End"));
}

UAgBossPattern_Leap::UAgBossPattern_Leap()
{
	SetupBossPattern();
}

void UAgBossPattern_Leap::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AAgBossCharacter* Boss = CastChecked<AAgBossCharacter>(GetAgCharacter());
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	const FAgBossPattern* Pattern = Boss->FindPattern(Handle);
	UAnimMontage* Montage = Pattern ? Data->FindPatternMontage(Pattern->Id) : nullptr;
	const FAgBossLeapParams* Params = GetPatternParams<FAgBossLeapParams>();
	if (!Montage || !Params || Pattern->Hits.IsEmpty() || !Boss->GetTarget() || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogAg, Verbose, TEXT("Boss pattern %s starts (play rate %.2f)"), *Pattern->Id.ToString(), Data->PatternPlayRate);
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage, Data->PatternPlayRate);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->ReadyForActivation();

	// 몽타주를 재생하지 못하면 OnCancelled가 그 자리에서 어빌리티를 끝냅니다. 끝난 어빌리티에서 도약하지 않습니다.
	if (!IsActive())
	{
		return;
	}
	RotationLockTag = nullptr;
	LeapStart = Boss->GetActorLocation();

	// 궤적: 체공 시간과 엔진 중력으로 처음 속도와 최고 높이가 정해집니다.
	AirTime = Params->AirTime;
	Gravity = -Boss->GetCharacterMovement()->GetGravityZ();
	const FVector ToTarget = (GetLandingTarget() - LeapStart) * FVector(1.f, 1.f, 0.f);
	if (!ToTarget.IsNearlyZero())
	{
		Boss->SetActorRotation(FRotator(0.f, ToTarget.Rotation().Yaw, 0.f));
	}
	Boss->LaunchCharacter(ToTarget / AirTime + FVector(0.f, 0.f, Gravity * AirTime * 0.5f), /*bXYOverride*/ true, /*bZOverride*/ true);

	// 회전: 공중에서도 대상을 향해 돌고, 착지(판정 구간)보다 회전 정지 시간만큼 앞서 멈춥니다. 가드 불가 예고는 몽타주의
	// 떠오르는 구간에 있는 노티파이가 켭니다.
	RotationStopLead = Data->RotationStopLead;
	if (RotationStopLead >= AirTime)
	{
		SetRotationLocked(true);
	}

	// 공중의 시각은 떠오른 뒤부터 잽니다. 땅에서는 아직 도약 속도가 적용되지 않아 수직 속도로 시각을 잴 수 없습니다.
	UAbilityTask_WaitMovementModeChange* TakeOffTask = UAbilityTask_WaitMovementModeChange::CreateWaitMovementModeChange(this, MOVE_Falling);
	TakeOffTask->OnChange.AddDynamic(this, &ThisClass::HandleTakeOff);
	TakeOffTask->ReadyForActivation();

	UAbilityTask_WaitMovementModeChange* LandTask = UAbilityTask_WaitMovementModeChange::CreateWaitMovementModeChange(this, MOVE_Walking);
	LandTask->OnChange.AddDynamic(this, &ThisClass::HandleLanded);
	LandTask->ReadyForActivation();
}

FVector UAgBossPattern_Leap::GetLandingTarget() const
{
	const AActor* Target = CastChecked<AAgBossCharacter>(GetAgCharacter())->GetTarget();
	const FAgBossLeapParams* Params = GetPatternParams<FAgBossLeapParams>();
	if (!Target || !Params)
	{
		return LeapStart;
	}
	const FVector ToTarget = (Target->GetActorLocation() - LeapStart) * FVector(1.f, 1.f, 0.f);
	const float Distance = FMath::Clamp(ToTarget.Size() - Params->TargetOffset, 0.f, Params->MaxDistance);
	return LeapStart + ToTarget.GetSafeNormal() * Distance;
}

float UAgBossPattern_Leap::GetDescentSpeedAt(float RemainingTime) const
{
	// 중력만 받는 포물선이라 수직 속도가 남은 체공 시간을 정합니다. 처음 수직 속도가 중력 × 체공 시간 / 2이므로, 남은 시간이
	// t일 때 하강 속도는 중력 × (체공 시간 / 2 − t)입니다.
	return Gravity * (AirTime * 0.5f - RemainingTime);
}

void UAgBossPattern_Leap::HandleTakeOff(EMovementMode NewMovementMode)
{
	// 시각을 시간이 아니라 보스의 수직 속도로 판정합니다. 히트스톱은 보스의 이동을 멈추므로, 공중에서 맞으면 이 시각들도 비행과
	// 함께 늦어집니다(전투 시스템 '히트스톱 중의 시간').
	if (RotationStopLead < AirTime)
	{
		UAbilityTask_WaitVelocityChange* TurnTask = UAbilityTask_WaitVelocityChange::CreateWaitVelocityChange(this, FVector::DownVector, GetDescentSpeedAt(RotationStopLead));
		TurnTask->OnVelocityChage.AddDynamic(this, &ThisClass::HandleRotationStop);
		TurnTask->ReadyForActivation();
	}

	// 최고점: 내려가기 시작하는 순간입니다.
	UAbilityTask_WaitVelocityChange* ApexTask = UAbilityTask_WaitVelocityChange::CreateWaitVelocityChange(this, FVector::DownVector, 0.f);
	ApexTask->OnVelocityChage.AddDynamic(this, &ThisClass::HandleApex);
	ApexTask->ReadyForActivation();
}

void UAgBossPattern_Leap::HandleApex()
{
	// 남은 체공 시간도 수직 속도로 셉니다(GetDescentSpeedAt의 식을 거꾸로 푼 것).
	AAgBossCharacter* Boss = CastChecked<AAgBossCharacter>(GetAgCharacter());
	const float Remaining = AirTime * 0.5f + Boss->GetCharacterMovement()->Velocity.Z / Gravity;
	if (Remaining <= UE_KINDA_SMALL_NUMBER)
	{
		return;
	}
	const FVector ToTarget = (GetLandingTarget() - Boss->GetActorLocation()) * FVector(1.f, 1.f, 0.f);
	Boss->LaunchCharacter(ToTarget / Remaining, /*bXYOverride*/ true, /*bZOverride*/ false);
}

void UAgBossPattern_Leap::HandleLanded(EMovementMode NewMovementMode)
{
	AAgBossCharacter* Boss = CastChecked<AAgBossCharacter>(GetAgCharacter());
	const FAgBossPattern* Pattern = Boss->FindPattern(GetCurrentAbilitySpecHandle());
	const FAgBossLeapParams* Params = GetPatternParams<FAgBossLeapParams>();
	if (!Pattern || Pattern->Hits.IsEmpty() || !Params)
	{
		return;
	}
	SetRotationLocked(false);

	MontageJumpToSection(LeapEndSection);

	// 플레이어가 가까우면 착지가 카메라를 흔듭니다(카메라 '상황별 단계'). 화면에 관한 것이라 AI의 대상이 아니라 로컬 플레이어를
	// 기준으로 합니다.
	const AActor* ViewPlayer = UGameplayStatics::GetPlayerPawn(this, 0);
	const UAgCameraData& Camera = UAgAssetManager::Get().GetCameraData();
	if (ViewPlayer && UAgCombatLibrary::GetHorizontalDistance(Boss, ViewPlayer) <= Camera.LeapLandingShakeRange)
	{
		UAgCombatLibrary::PlayCameraShake(Boss, Camera.LeapLandingShake);
	}

	// 착지가 판정 구간입니다. 착지 지점 반경 안에서 땅에 있는 대상만 한 번 맞습니다(점프 중이면 맞지 않음).
	AAgCharacterBase* Target = Boss->GetTarget();
	if (Target && !Target->GetCharacterMovement()->IsFalling() && UAgCombatLibrary::GetHorizontalDistance(Boss, Target) <= Params->HitRadius)
	{
		UAgEffect_Damage::ApplyAttack(Boss, Boss, Target, FAgPlayerAttackHit(Pattern->Hits[0]), Boss->GetActorLocation());
	}
}

void UAgBossPattern_Leap::HandleRotationStop()
{
	SetRotationLocked(true);
}

void UAgBossPattern_Leap::SetRotationLocked(bool bLocked)
{
	if (bLocked && !RotationLockTag)
	{
		RotationLockTag = UAgAbilityTask_HoldTag::HoldTag(this, AgGameplayTags::State_Boss_RotationLocked);
		RotationLockTag->ReadyForActivation();
	}
	else if (!bLocked && RotationLockTag)
	{
		RotationLockTag->EndTask();
		RotationLockTag = nullptr;
	}
}

void UAgBossPattern_Leap::HandleMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UAgBossPattern_Leap::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
