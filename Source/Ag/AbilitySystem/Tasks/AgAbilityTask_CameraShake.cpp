// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/AgAbilityTask_CameraShake.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "Engine/World.h"
#include "Game/AgPlayerController.h"
#include "GameFramework/Pawn.h"

UAbilityTask* FAgCameraShakeParams::MakeTask(const FAgTaskContext& Context) const
{
	return UAgAbilityTask_CameraShake::CameraShake(&Context.Ability, Shake, Range);
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
	const AActor* Owner = GetAvatarActor();
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); Owner && It; ++It)
	{
		AAgPlayerController* Controller = Cast<AAgPlayerController>(It->Get());
		const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		if (Pawn && Owner->GetHorizontalDistanceTo(Pawn) <= Range)
		{
			Controller->PlayCameraShake(Shake);
		}
	}
	EndTask();
}
