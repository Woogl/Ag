// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_TimeStop.h"

#include "Abilities/GameplayAbility.h"
#include "Character/AgCharacterBase.h"
#include "Combat/AgTimeSubsystem.h"
#include "Engine/World.h"

UAgAbilityTask_TimeStop* UAgAbilityTask_TimeStop::TimeStop(UGameplayAbility* OwningAbility)
{
	return NewAbilityTask<UAgAbilityTask_TimeStop>(OwningAbility);
}

void UAgAbilityTask_TimeStop::Activate()
{
	Super::Activate();

	AAgCharacterBase* Avatar = Cast<AAgCharacterBase>(GetAvatarActor());
	if (!Avatar)
	{
		EndTask();
		return;
	}
	UAgTimeSubsystem* Subsystem = GetWorld()->GetSubsystem<UAgTimeSubsystem>();
	TimeSubsystem = Subsystem;
	const bool bOwnerWasActive = Ability && Ability->IsActive();
	// 시작 알림에서 Task가 끝나도 이미 획득한 정지를 해제합니다.
	bAcquired = true;
	if (!Subsystem->StartTimeStop(*this, *Avatar))
	{
		bAcquired = false;
		EndTask();
		return;
	}
	// 활성 Task 목록 등록 전 취소는 엔진의 소유자 종료 통지를 받지 못합니다.
	if (bOwnerWasActive && !IsFinished() && (!Ability || !Ability->IsActive()))
	{
		EndTask();
	}
}

void UAgAbilityTask_TimeStop::OnDestroy(bool bInOwnerFinished)
{
	if (UAgTimeSubsystem* Subsystem = TimeSubsystem.Get(); Subsystem && bAcquired)
	{
		bAcquired = false;
		Subsystem->EndTimeStop(*this);
	}

	Super::OnDestroy(bInOwnerFinished);
}
