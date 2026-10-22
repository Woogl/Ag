// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_CameraShake.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "Camera/AgCameraShake.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

UAbilityTask* FAgCameraShakeParams::MakeTask(const FAgTaskContext& Context) const
{
	UAgAbilityTask_CameraShake* Task = UAbilityTask::NewAbilityTask<UAgAbilityTask_CameraShake>(&Context.Ability);
	Task->Params = *this;
	return Task;
}

void UAgAbilityTask_CameraShake::Activate()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* Controller = It->Get();
		if (APawn* Pawn = Controller ? Controller->GetPawn() : nullptr)
		{
			AgCameraShake::Play(Pawn, Params.Shake);
		}
	}
	EndTask();
}
