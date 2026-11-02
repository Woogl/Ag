// Copyright Woogle. All Rights Reserved.

#include "Game/AgSimulationRoomGameMode.h"

#include "AI/AgEnemyAIController.h"
#include "Ag.h"
#include "Algo/Sort.h"
#include "Character/AgBossCharacter.h"
#include "Character/AgCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "Game/AgEnemySpawnPoint.h"
#include "Game/AgPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UI/AgEnemySelectScreen.h"
#include "UI/AgUILayout.h"
#include "UObject/Package.h"

namespace
{
	const TCHAR* LineupOption = TEXT("Enemies");

	bool IsBossClass(const TSubclassOf<AAgCharacterBase>& EnemyClass)
	{
		return EnemyClass && EnemyClass->IsChildOf<AAgBossCharacter>();
	}

	bool IsEnemyTeam(const AAgCharacterBase& Character)
	{
		return Character.GetGenericTeamId() == FGenericTeamId(static_cast<uint8>(EAgTeam::Enemy));
	}
} // namespace

TSubclassOf<AAgCharacterBase> AAgSimulationRoomGameMode::FindEnemyClass(FName CharacterDataId) const
{
	const TSubclassOf<AAgCharacterBase>* EnemyClass = EnemyClasses.FindByPredicate([CharacterDataId](const TSubclassOf<AAgCharacterBase>& Candidate)
	{ return Candidate && Candidate->GetDefaultObject<AAgCharacterBase>()->GetCharacterDataId() == CharacterDataId; });
	return EnemyClass ? *EnemyClass : nullptr;
}

bool AAgSimulationRoomGameMode::CanPlace(const TArray<TSubclassOf<AAgCharacterBase>>& InLineup, int32 SlotIndex, TSubclassOf<AAgCharacterBase> EnemyClass) const
{
	if (!EnemyClass || !SpawnPoints.IsValidIndex(SlotIndex))
	{
		return false;
	}
	if (!IsBossClass(EnemyClass))
	{
		return true;
	}
	for (int32 Index = 0; Index < InLineup.Num(); ++Index)
	{
		if (Index != SlotIndex && IsBossClass(InLineup[Index]))
		{
			return false;
		}
	}
	return true;
}

void AAgSimulationRoomGameMode::StartLineup(const TArray<TSubclassOf<AAgCharacterBase>>& NewLineup)
{
	TArray<FString> Ids;
	for (const TSubclassOf<AAgCharacterBase>& EnemyClass : NewLineup)
	{
		Ids.Add(EnemyClass ? EnemyClass->GetDefaultObject<AAgCharacterBase>()->GetCharacterDataId().ToString() : FString());
	}

	// PIE의 월드 패키지 이름에는 PIE 접두사가 붙어 있어, 떼어 낸 이름으로 같은 맵을 엽니다.
	const FString MapName = UWorld::RemovePIEPrefix(GetWorld()->GetPackage()->GetName());
	UGameplayStatics::OpenLevel(this, FName(MapName), true, FString::Printf(TEXT("%s=%s"), LineupOption, *FString::Join(Ids, TEXT(","))));
}

void AAgSimulationRoomGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	CollectSpawnPoints();
	Lineup.SetNum(SpawnPoints.Num());

	TArray<FString> Ids;
	UGameplayStatics::ParseOption(Options, LineupOption).ParseIntoArray(Ids, TEXT(","), false);
	for (int32 Index = 0; Index < Ids.Num(); ++Index)
	{
		if (Ids[Index].IsEmpty())
		{
			continue;
		}
		const TSubclassOf<AAgCharacterBase> EnemyClass = FindEnemyClass(FName(*Ids[Index]));
		if (!CanPlace(Lineup, Index, EnemyClass))
		{
			UE_LOG(LogAg, Warning, TEXT("Simulation room: '%s' in slot %d is skipped (not in EnemyClasses, a second boss, or no such spawn point)."), *Ids[Index], Index + 1);
			continue;
		}
		Lineup[Index] = EnemyClass;
	}
}

void AAgSimulationRoomGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	// Start는 모든 자리가 비어 있으면 실행되지 않으므로, 구성이 비어 있으면 타이틀에서 처음 들어온 것입니다.
	if (!Lineup.ContainsByPredicate([](const TSubclassOf<AAgCharacterBase>& EnemyClass)
	{ return EnemyClass != nullptr; }))
	{
		if (AAgPlayerController* PlayerController = Cast<AAgPlayerController>(NewPlayer))
		{
			PlayerController->PushScreen(EAgUILayer::Menu, EnemySelectScreenClass);
		}
		return;
	}

	// 엔진은 플레이어 참가를 월드 BeginPlay 전에 처리하므로, 배치한 적도 게임 모드의 BeginPlay가 전투원으로 등록합니다.
	if (APawn* Player = NewPlayer ? NewPlayer->GetPawn() : nullptr)
	{
		SpawnLineup(*Player);
	}
}

bool AAgSimulationRoomGameMode::IsStageCleared(const AAgCharacterBase& DeadCharacter) const
{
	for (TActorIterator<AAgCharacterBase> It(GetWorld()); It; ++It)
	{
		if (IsEnemyTeam(**It) && !It->IsDead())
		{
			return false;
		}
	}
	return IsEnemyTeam(DeadCharacter);
}

void AAgSimulationRoomGameMode::CollectSpawnPoints()
{
	for (TActorIterator<AAgEnemySpawnPoint> It(GetWorld()); It; ++It)
	{
		SpawnPoints.Add(*It);
	}
	Algo::SortBy(SpawnPoints, &AAgEnemySpawnPoint::GetSlot);

	for (int32 Index = 0; Index < SpawnPoints.Num(); ++Index)
	{
		if (SpawnPoints[Index]->GetSlot() != Index + 1)
		{
			UE_LOG(LogAg, Error, TEXT("Simulation room: spawn point slots must be 1 to %d without gaps or duplicates (%s has %d)."), SpawnPoints.Num(), *SpawnPoints[Index]->GetName(), SpawnPoints[Index]->GetSlot());
		}
	}
}

void AAgSimulationRoomGameMode::SpawnLineup(AActor& Player)
{
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	for (int32 Index = 0; Index < Lineup.Num(); ++Index)
	{
		if (!Lineup[Index])
		{
			continue;
		}
		const AAgCharacterBase* EnemyDefaults = Lineup[Index]->GetDefaultObject<AAgCharacterBase>();
		const FVector Location = SpawnPoints[Index]->GetActorLocation() + FVector(0., 0., EnemyDefaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		const FRotator Rotation = (Player.GetActorLocation() - Location).GetSafeNormal2D().Rotation();
		AAgCharacterBase* Enemy = GetWorld()->SpawnActor<AAgCharacterBase>(Lineup[Index], Location, Rotation, SpawnParameters);
		if (!Enemy || !IsEnemyTeam(*Enemy))
		{
			continue;
		}
		if (AAgEnemyAIController* Controller = Cast<AAgEnemyAIController>(Enemy->GetController()))
		{
			Controller->NoticeTarget(Player);
		}
	}
}
