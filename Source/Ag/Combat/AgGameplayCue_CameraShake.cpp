// Copyright Woogle. All Rights Reserved.

#include "Combat/AgGameplayCue_CameraShake.h"

#include "Ag.h"
#include "Camera/CameraShakeBase.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

bool UAgGameplayCue_CameraShake::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	// The shake is the player's view, whichever character the cue ran on.
	APlayerController* PlayerController = MyTarget ? UGameplayStatics::GetPlayerController(MyTarget, 0) : nullptr;
	if (PlayerController && Shake)
	{
		PlayerController->ClientStartCameraShake(Shake);
		UE_LOG(LogAg, Verbose, TEXT("Camera shake %s"), *Shake->GetName());
	}
	return true;
}
