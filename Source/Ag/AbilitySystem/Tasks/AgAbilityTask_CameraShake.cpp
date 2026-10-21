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
	const AActor* Owner = GetAvatarActor();
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); Owner && It; ++It)
	{
		APlayerController* Controller = It->Get();
		APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		if (Pawn && Owner->GetHorizontalDistanceTo(Pawn) <= Params.MaxDistance)
		{
			AgCameraShake::Play(Pawn, Params.Shake);
		}
	}
	EndTask();
}
