// Copyright Woogle. All Rights Reserved.

#include "Game/AgBossStageGameMode.h"

#include "Character/AgBossCharacter.h"
#include "Character/AgCharacterBase.h"
#include "Data/AgCharacterData.h"
#include "Game/AgPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

void AAgBossStageGameMode::RegisterCombatant(AAgCharacterBase* Combatant)
{
	if (Combatant)
	{
		Combatant->OnDied.AddUObject(this, &ThisClass::HandleCombatantDied);
	}
}

void AAgBossStageGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	if (AAgPlayerController* PlayerController = Cast<AAgPlayerController>(NewPlayer))
	{
		PlayerController->ShowHUD(HUDWidgetClass);
	}
}

void AAgBossStageGameMode::HandleCombatantDied(AAgCharacterBase* DeadCharacter)
{
	// The HUD hides from the moment any death presentation starts (전투 HUD '상황별 표시 규칙').
	if (AAgPlayerController* PlayerController = Cast<AAgPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		PlayerController->HideHUD();
	}

	// A death after the result is decided plays its presentation but changes nothing.
	if (bResultDecided)
	{
		return;
	}
	bResultDecided = true;
	bStageCleared = DeadCharacter->IsA<AAgBossCharacter>();

	const UAgCharacterData* Data = DeadCharacter->GetCharacterData();
	const float Delay = Data ? Data->DeathPresentationTime : 0.f;
	if (Delay > 0.f)
	{
		GetWorldTimerManager().SetTimer(ResultTimer, this, &ThisClass::ShowResult, Delay);
	}
	else
	{
		ShowResult();
	}
}

void AAgBossStageGameMode::ShowResult()
{
	if (AAgPlayerController* PlayerController = Cast<AAgPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		PlayerController->ShowResult(ResultScreenClass, bStageCleared);
	}
}
