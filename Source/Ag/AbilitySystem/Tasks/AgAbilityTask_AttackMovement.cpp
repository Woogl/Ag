// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_AttackMovement.h"

#include "AbilitySystemComponent.h"
#include "Animation/AgAnimNotifyState_AttackWindow.h"
#include "Animation/AnimInstance.h"
#include "Character/AgCharacterBase.h"
#include "Combat/AgCombatLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "MotionWarpingComponent.h"

UAgAbilityTask_AttackMovement::UAgAbilityTask_AttackMovement()
{
	bTickingTask = true;
}

UAgAbilityTask_AttackMovement* UAgAbilityTask_AttackMovement::Attack(UGameplayAbility* OwningAbility, int32 InNumHits, float InTargetingRange, float InMaxApproachDistance)
{
	UAgAbilityTask_AttackMovement* Task = NewAbilityTask<UAgAbilityTask_AttackMovement>(OwningAbility);
	Task->NumHits = InNumHits;
	Task->TargetingRange = InTargetingRange;
	Task->MaxApproachDistance = InMaxApproachDistance;
	Task->bKeepGap = true;
	return Task;
}

UAgAbilityTask_AttackMovement* UAgAbilityTask_AttackMovement::FitGap(UGameplayAbility* OwningAbility, AAgCharacterBase* InTarget, float InGap)
{
	UAgAbilityTask_AttackMovement* Task = NewAbilityTask<UAgAbilityTask_AttackMovement>(OwningAbility);
	Task->Target = InTarget;
	Task->Gap = InGap;
	Task->bKeepGap = false;
	return Task;
}

void UAgAbilityTask_AttackMovement::Activate()
{
	Super::Activate();

	AAgCharacterBase* Owner = Cast<AAgCharacterBase>(GetAvatarActor());
	if (!Owner)
	{
		EndTask();
		return;
	}
	Character = Owner;

	if (!bKeepGap)
	{
		// 간격 맞춤은 범위와 거리 상한 없이 대상 앞 간격 지점까지 따라갑니다.
		Origin = Owner->GetActorLocation();
		bTracking = true;
		UpdateWarpTarget();
		return;
	}

	if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
	{
		WindowEndedHandle = ASC->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_AttackWindowEnded).AddUObject(this, &ThisClass::HandleAttackWindowEnded);
	}
	StartHit();
}

void UAgAbilityTask_AttackMovement::StartHit()
{
	AAgCharacterBase* Owner = Character.Get();
	const UAnimInstance* AnimInstance = Owner ? Owner->GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return;
	}

	// 정지 간격은 대상이 타겟팅 범위 안에 있든 없든 지킵니다. 다가가는 거리는 다음 판정 구간 시작까지 모션 자체의 전진에 최대
	// 접근 거리를 더한 만큼까지라, 선딜 중 대상이 물러나면(백스텝) 끝까지 쫓지 않고 그만큼 간격이 벌어집니다.
	AAgCharacterBase* NewTarget = Owner->FindAttackTarget();
	Target = NewTarget;
	Gap = Owner->GetCharacterData()->StopDistance;
	Origin = Owner->GetActorLocation();
	MaxTravel = UAgAnimNotifyState_AttackWindow::GetTravelToNextWindow(*AnimInstance) + MaxApproachDistance;
	Owner->SetStopTarget(NewTarget, Gap);

	bTracking = NewTarget && (TargetingRange < 0.f || UAgCombatLibrary::GetHorizontalDistance(Owner, NewTarget) <= TargetingRange);
	if (bTracking)
	{
		Owner->SetFacingTarget(NewTarget);
		UpdateWarpTarget();
	}
	else
	{
		// 범위 안에 대상이 없으면 모션을 만든 그대로 재생하고, 캐릭터가 정한 대체 대상 쪽으로만 돕니다(플레이어 사양 '공격 방향').
		StopTracking();
		Owner->SetFacingTarget(Owner->GetFallbackFacingTarget());
	}
}

void UAgAbilityTask_AttackMovement::HandleAttackWindowEnded(const FGameplayEventData* Payload)
{
	// 판정 구간이 끝나면 다음 타를 시작합니다. 마지막 타 뒤에는 돌지 않고, 정지 간격은 태스크가 끝날 때까지 지킵니다.
	if (FMath::RoundToInt(Payload->EventMagnitude) < NumHits - 1)
	{
		StartHit();
	}
	else
	{
		StopTracking();
	}
}

void UAgAbilityTask_AttackMovement::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	const AAgCharacterBase* Owner = Character.Get();
	if (!bTracking || !Owner)
	{
		return;
	}
	if (Owner->IsFacingFixed())
	{
		// 바라보는 방향과 함께 접근 목표도 고정합니다. 마지막 목표는 워핑 구간이 끝날 때까지 쓰입니다.
		bTracking = false;
		return;
	}
	UpdateWarpTarget();
}

void UAgAbilityTask_AttackMovement::UpdateWarpTarget()
{
	AAgCharacterBase* Owner = Character.Get();
	const AAgCharacterBase* TargetCharacter = Target.Get();
	const float Distance = UAgCombatLibrary::GetHorizontalDistance(Owner, TargetCharacter);
	if (!Owner || !TargetCharacter || (TargetingRange >= 0.f && Distance > TargetingRange))
	{
		// 범위 밖: 이 타의 추적을 멈춥니다.
		StopTracking();
		return;
	}

	const FVector Location = Owner->GetActorLocation();
	const FVector Direction = UAgCombatLibrary::GetHorizontalDirection(Owner, TargetCharacter);
	FVector Goal = Location;
	if (Distance > Gap || !bKeepGap)
	{
		Goal = TargetCharacter->GetActorLocation() - Direction * Gap;
		Goal.Z = Location.Z;
	}
	if (MaxTravel >= 0.f)
	{
		const FVector Offset = (Goal - Origin).GetClampedToMaxSize2D(MaxTravel);
		Goal = FVector(Origin.X + Offset.X, Origin.Y + Offset.Y, Location.Z);
	}

	// 방향은 늘 대상을 바라보게 둡니다. 워프로 돌릴지는 몽타주 워핑 구간의 방향 워프 설정이 정합니다(처형과 회피 반격만 켬).
	const FRotator Facing = Direction.IsNearlyZero() ? Owner->GetActorRotation() : Direction.Rotation();
	Owner->GetMotionWarping()->AddOrUpdateWarpTargetFromLocationAndRotation(AAgCharacterBase::AttackWarpTarget, Goal, Facing);
}

void UAgAbilityTask_AttackMovement::StopTracking()
{
	bTracking = false;
	if (AAgCharacterBase* Owner = Character.Get())
	{
		if (bKeepGap)
		{
			Owner->SetFacingTarget(nullptr);
		}
		Owner->GetMotionWarping()->RemoveWarpTarget(AAgCharacterBase::AttackWarpTarget);
	}
}

void UAgAbilityTask_AttackMovement::OnDestroy(bool bInOwnerFinished)
{
	if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get(); ASC && WindowEndedHandle.IsValid())
	{
		ASC->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_AttackWindowEnded).Remove(WindowEndedHandle);
	}

	if (AAgCharacterBase* Owner = Character.Get())
	{
		if (bKeepGap)
		{
			Owner->ClearStopTarget();
			Owner->SetFacingTarget(nullptr);
		}
		Owner->GetMotionWarping()->RemoveWarpTarget(AAgCharacterBase::AttackWarpTarget);
	}

	Super::OnDestroy(bInOwnerFinished);
}
