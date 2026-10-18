// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_CameraShake.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "Game/AgPlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

UAbilityTask* FAgCameraShakeParams::MakeTask(const FAgTaskContext& Context) const
{
	return UAgAbilityTask_CameraShake::CameraShake(&Context.Runner, Shake, Range);
}

UAgAbilityTask_CameraShake* UAgAbilityTask_CameraShake::CameraShake(UGameplayAbility* Owner, EAgCameraShake InShake, float InRange)
{
	UAgAbilityTask_CameraShake* Task = NewAbilityTask<UAgAbilityTask_CameraShake>(Owner);
	Task->Shake = InShake;
	Task->Range = InRange;
	return Task;
}

void UAgAbilityTask_CameraShake::Activate()
{
	// 전투 대상과 관계없이 로컬 플레이어 화면을 흔듭니다.
	const AActor* Owner = GetAvatarActor();
	AAgPlayerController* Controller = Cast<AAgPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (Owner && Pawn && Owner->GetHorizontalDistanceTo(Pawn) <= Range)
	{
		Controller->PlayCameraShake(Shake);
	}
	EndTask();
}
