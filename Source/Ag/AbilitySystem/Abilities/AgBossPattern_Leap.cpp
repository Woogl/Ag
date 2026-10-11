// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgBossPattern_Leap.h"

#include "Abilities/Tasks/AbilityTask_Repeat.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitMovementModeChange.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Character/AgBossCharacter.h"
#include "Combat/AgCombatLibrary.h"
#include "Core/AgGameplayTags.h"
#include "Core/AgSettings.h"
#include "Data/AgAttackData.h"
#include "Data/AgCameraData.h"
#include "Data/AgCharacterData.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const FName LeapEndSection(TEXT("End"));

	/** 공중에서 착지 목표를 다시 맞추는 간격 */
	constexpr float AimInterval = 1.f / 30.f;
}

void UAgBossPattern_Leap::OnPatternStarted()
{
	AAgBossCharacter* Boss = GetBoss();
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	const FAgBossPattern* Pattern = GetPattern();
	if (!Boss || !Data || !Pattern || Pattern->Hits.IsEmpty() || !GetTarget())
	{
		EndSelf(true);
		return;
	}
	bLanded = false;
	LeapStart = Boss->GetActorLocation();

	// 궤적: 체공 시간(2페이즈에서는 짧아짐)과 엔진 중력으로 처음 속도와 최고 높이가 정해집니다.
	const float PatternSpeed = Boss->GetPatternSpeed();
	const float AirTime = Data->LeapAirTime / PatternSpeed;
	const float Gravity = -Boss->GetCharacterMovement()->GetGravityZ();
	const FVector ToTarget = (GetLandingTarget() - LeapStart) * FVector(1.f, 1.f, 0.f);
	if (!ToTarget.IsNearlyZero())
	{
		Boss->SetActorRotation(FRotator(0.f, ToTarget.Rotation().Yaw, 0.f));
	}
	Boss->LaunchCharacter(ToTarget / AirTime + FVector(0.f, 0.f, Gravity * AirTime * 0.5f), /*bXYOverride*/ true, /*bZOverride*/ true);
	LandingTime = Boss->GetWorld()->GetTimeSeconds() + AirTime;

	// 회전: 공중에서도 대상을 향해 돌고, 착지(판정 구간)보다 회전 정지 시간만큼 앞서 멈춥니다. 2페이즈에서는 둘 다
	// 같은 비율로 짧아집니다.
	const float TurnTime = (Data->LeapAirTime - Data->RotationStopLead) / PatternSpeed;
	if (TurnTime > 0.f)
	{
		UAbilityTask_WaitDelay* TurnTask = UAbilityTask_WaitDelay::WaitDelay(this, TurnTime);
		TurnTask->OnFinish.AddDynamic(this, &ThisClass::HandleRotationStop);
		TurnTask->ReadyForActivation();
	}
	else
	{
		SetRotationLocked(true);
	}

	// 섬광: 가드 불가면 붉은 섬광, 패리 가능하면 예고 섬광을 착지보다 그 시간만큼 앞서 켭니다(2페이즈 규칙도 같음).
	// 그때까지는 착지 목표를 대상 쪽으로 다시 맞춥니다.
	const FAgAttackHit& Hit = Pattern->Hits[0];
	const float TelegraphLead = !Hit.bGuardable ? Data->UnblockableTelegraphLead : (Hit.bParryable ? Data->ParryTelegraphLead : 0.f);
	const float TelegraphTime = (Data->LeapAirTime - TelegraphLead) / PatternSpeed;
	if (TelegraphTime > AimInterval)
	{
		AimTask = UAbilityTask_Repeat::RepeatAction(this, AimInterval, FMath::CeilToInt(TelegraphTime / AimInterval));
		AimTask->OnPerformAction.AddDynamic(this, &ThisClass::HandleAim);
		AimTask->ReadyForActivation();
	}
	if (TelegraphLead > 0.f)
	{
		UAbilityTask_WaitDelay* TelegraphTask = UAbilityTask_WaitDelay::WaitDelay(this, FMath::Max(TelegraphTime, 0.f));
		TelegraphTask->OnFinish.AddDynamic(this, &ThisClass::HandleTelegraphTime);
		TelegraphTask->ReadyForActivation();
	}

	UAbilityTask_WaitMovementModeChange* LandTask = UAbilityTask_WaitMovementModeChange::CreateWaitMovementModeChange(this, MOVE_Walking);
	LandTask->OnChange.AddDynamic(this, &ThisClass::HandleLanded);
	LandTask->ReadyForActivation();
}

FVector UAgBossPattern_Leap::GetLandingTarget() const
{
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	const AActor* Target = GetTarget();
	if (!Data || !Target)
	{
		return LeapStart;
	}
	const FVector ToTarget = (Target->GetActorLocation() - LeapStart) * FVector(1.f, 1.f, 0.f);
	const float Distance = FMath::Clamp(ToTarget.Size() - Data->LeapTargetOffset, 0.f, Data->LeapMaxDistance);
	return LeapStart + ToTarget.GetSafeNormal() * Distance;
}

void UAgBossPattern_Leap::HandleAim(int32 ActionNumber)
{
	// 첫 동작은 도약과 같은 프레임이라, 다시 Launch하면 아직 적용되지 않은 도약 속도를 덮어씁니다.
	AAgBossCharacter* Boss = GetBoss();
	const float Remaining = Boss ? static_cast<float>(LandingTime - Boss->GetWorld()->GetTimeSeconds()) : 0.f;
	if (ActionNumber == 0 || bLanded || !Boss || Remaining <= UE_KINDA_SMALL_NUMBER)
	{
		return;
	}
	const FVector ToTarget = (GetLandingTarget() - Boss->GetActorLocation()) * FVector(1.f, 1.f, 0.f);
	Boss->LaunchCharacter(ToTarget / Remaining, /*bXYOverride*/ true, /*bZOverride*/ false);
}

void UAgBossPattern_Leap::HandleTelegraphTime()
{
	StopAiming();
	const FAgBossPattern* Pattern = GetPattern();
	if (bLanded || !Pattern || Pattern->Hits.IsEmpty())
	{
		return;
	}
	if (!Pattern->Hits[0].bGuardable)
	{
		SetUnblockableTelegraph(true);
	}
	else
	{
		SetParryTelegraph(true);
	}
}

void UAgBossPattern_Leap::StopAiming()
{
	if (AimTask)
	{
		AimTask->EndTask();
		AimTask = nullptr;
	}
}

void UAgBossPattern_Leap::HandleLanded(EMovementMode NewMovementMode)
{
	AAgBossCharacter* Boss = GetBoss();
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	const FAgBossPattern* Pattern = GetPattern();
	if (bLanded || !Boss || !Data || !Pattern || Pattern->Hits.IsEmpty())
	{
		return;
	}
	bLanded = true;
	StopAiming();
	SetRotationLocked(false);
	SetUnblockableTelegraph(false);
	SetParryTelegraph(false);

	if (UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance())
	{
		AnimInstance->Montage_JumpToSection(LeapEndSection, GetMontage());
	}

	// 플레이어가 가까우면 착지가 카메라를 흔듭니다(카메라 '상황별 단계'). 화면에 관한 것이라 AI의 대상이 아니라 로컬 플레이어를
	// 기준으로 합니다.
	const AActor* ViewPlayer = UGameplayStatics::GetPlayerPawn(this, 0);
	const UAgCameraData& Camera = UAgSettings::Get()->GetCameraData();
	if (ViewPlayer && UAgCombatLibrary::GetHorizontalDistance(Boss, ViewPlayer) <= Camera.LeapLandingShakeRange)
	{
		UAgCombatLibrary::PlayCameraShake(Boss, Camera.LeapLandingShake);
	}

	// 착지가 판정 구간입니다. 착지 지점 반경 안에서 땅에 있는 대상만 한 번 맞습니다(점프 중이면 맞지 않음).
	AAgCharacterBase* Target = GetTarget();
	if (Target && !Target->GetCharacterMovement()->IsFalling() && UAgCombatLibrary::GetHorizontalDistance(Boss, Target) <= Data->LeapHitRadius)
	{
		UAgCombatLibrary::ProcessHit(Boss, Target, FAgPlayerAttackHit(Pattern->Hits[0]), Boss->GetActorLocation());
	}
}

void UAgBossPattern_Leap::HandleRotationStop()
{
	if (!bLanded)
	{
		SetRotationLocked(true);
	}
}

void UAgBossPattern_Leap::SetRotationLocked(bool bLocked)
{
	if (bRotationLocked == bLocked)
	{
		return;
	}
	bRotationLocked = bLocked;
	if (bLocked)
	{
		GetAbilitySystemComponentFromActorInfo()->AddLooseGameplayTag(AgGameplayTags::State_Boss_RotationLocked);
	}
	else
	{
		GetAbilitySystemComponentFromActorInfo()->RemoveLooseGameplayTag(AgGameplayTags::State_Boss_RotationLocked);
	}
}

void UAgBossPattern_Leap::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 공중에서 끊기면(사망, 그로기 등) 그때의 속도로 중력에 맡겨 떨어집니다.
	StopAiming();
	SetRotationLocked(false);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
