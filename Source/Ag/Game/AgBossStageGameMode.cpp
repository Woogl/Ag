// Copyright Woogle. All Rights Reserved.

#include "Game/AgBossStageGameMode.h"

#include "Character/AgBossCharacter.h"
#include "Character/AgCharacterBase.h"
#include "Character/AgPlayerCharacter.h"
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

void AAgBossStageGameMode::RestartStage()
{
	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		PlayerController->RestartLevel();
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
	const bool bBossDied = DeadCharacter->IsA<AAgBossCharacter>();
	if (!bBossDied && !DeadCharacter->IsA<AAgPlayerCharacter>())
	{
		return;
	}

	if (AAgPlayerController* PlayerController = Cast<AAgPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		PlayerController->HideHUD();
	}

	if (bResultDecided)
	{
		return;
	}
	bResultDecided = true;
	bStageCleared = bBossDied;

	if (DeathPresentationTime > 0.f)
	{
		GetWorldTimerManager().SetTimer(ResultTimer, this, &ThisClass::ShowResult, DeathPresentationTime);
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
