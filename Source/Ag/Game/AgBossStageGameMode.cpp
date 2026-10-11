// Copyright Woogle. All Rights Reserved.

#include "Game/AgBossStageGameMode.h"

#include "Character/AgBossCharacter.h"
#include "Character/AgCharacterBase.h"
#include "Character/AgPlayerCharacter.h"
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
	// 결과와 HUD는 플레이어와 보스의 사망으로만 정해집니다(게임 플로우 'Stage Clear', 'Game Over').
	const bool bBossDied = DeadCharacter->IsA<AAgBossCharacter>();
	if (!bBossDied && !DeadCharacter->IsA<AAgPlayerCharacter>())
	{
		return;
	}

	// 사망 연출이 시작되는 순간부터 HUD를 숨깁니다(전투 HUD '상황별 표시 규칙').
	if (AAgPlayerController* PlayerController = Cast<AAgPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		PlayerController->HideHUD();
	}

	// 결과가 정해진 뒤의 사망은 연출만 재생하고 아무것도 바꾸지 않습니다.
	if (bResultDecided)
	{
		return;
	}
	bResultDecided = true;
	bStageCleared = bBossDied;

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
