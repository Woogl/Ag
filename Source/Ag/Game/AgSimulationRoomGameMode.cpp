// Copyright Woogle. All Rights Reserved.

#include "Game/AgSimulationRoomGameMode.h"

#include "Ag.h"
#include "Algo/Sort.h"
#include "Character/AgBossCharacter.h"
#include "Character/AgCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "Data/AgCharacterData.h"
#include "EngineUtils.h"
#include "Game/AgEnemySpawnPoint.h"
#include "Game/AgPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Damage.h"
#include "UI/AgCharacterPreview.h"
#include "UI/AgEnemySelectScreen.h"
#include "UI/AgUILayout.h"
#include "UObject/Package.h"

namespace AgSimulationRoomGameModePrivate
{
	const TCHAR* LineupOption = TEXT("Enemies");

	// 표시 액터는 레벨 위 높은 곳에 적 종류마다 떨어뜨려 띄워, 레벨의 KillZ와 원거리 차폐, 서로의 조명이 닿지 않게 합니다.
	const FVector PreviewOrigin(0., 0., 100000.);
	const double PreviewSpacing = 2000.;

	bool IsBossClass(const TSubclassOf<AAgCharacterBase>& EnemyClass)
	{
		return EnemyClass && EnemyClass->IsChildOf<AAgBossCharacter>();
	}

	bool IsEnemyTeam(const AAgCharacterBase& Character)
	{
		return Character.GetGenericTeamId() == FGenericTeamId(static_cast<uint8>(EAgTeam::Enemy));
	}
} // namespace AgSimulationRoomGameModePrivate

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
	if (!AgSimulationRoomGameModePrivate::IsBossClass(EnemyClass))
	{
		return true;
	}
	for (int32 Index = 0; Index < InLineup.Num(); ++Index)
	{
		if (Index != SlotIndex && AgSimulationRoomGameModePrivate::IsBossClass(InLineup[Index]))
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
	UGameplayStatics::OpenLevel(this, FName(MapName), true, FString::Printf(TEXT("%s=%s"), AgSimulationRoomGameModePrivate::LineupOption, *FString::Join(Ids, TEXT(","))));
}

UTextureRenderTarget2D* AAgSimulationRoomGameMode::GetEnemyPreview(TSubclassOf<AAgCharacterBase> EnemyClass)
{
	const int32 CandidateIndex = EnemyClasses.IndexOfByKey(EnemyClass);
	if (!EnemyClass || !EnemyPreviewClass || CandidateIndex == INDEX_NONE)
	{
		return nullptr;
	}

	TObjectPtr<AAgCharacterPreview>& Preview = EnemyPreviews.FindOrAdd(EnemyClass);
	if (!Preview)
	{
		Preview = GetWorld()->SpawnActor<AAgCharacterPreview>(EnemyPreviewClass, AgSimulationRoomGameModePrivate::PreviewOrigin + FVector(0., CandidateIndex * AgSimulationRoomGameModePrivate::PreviewSpacing, 0.), FRotator::ZeroRotator);
		Preview->ShowCharacter(EnemyClass);
	}
	return Preview->GetRenderTarget();
}

void AAgSimulationRoomGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	CollectSpawnPoints();
	Lineup.SetNum(SpawnPoints.Num());

	TArray<FString> Ids;
	UGameplayStatics::ParseOption(Options, AgSimulationRoomGameModePrivate::LineupOption).ParseIntoArray(Ids, TEXT(","), false);
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

	// 엔진은 플레이어 참가를 월드 BeginPlay 전에 처리하므로, 배치한 적의 사망도 게임 모드의 BeginPlay가 구독합니다.
	if (APawn* Player = NewPlayer ? NewPlayer->GetPawn() : nullptr)
	{
		SpawnLineup(*Player);
	}
}

bool AAgSimulationRoomGameMode::IsStageCleared(const AAgCharacterBase& DeadCharacter) const
{
	for (TActorIterator<AAgCharacterBase> It(GetWorld()); It; ++It)
	{
		if (AgSimulationRoomGameModePrivate::IsEnemyTeam(**It) && !It->IsDead())
		{
			return false;
		}
	}
	return AgSimulationRoomGameModePrivate::IsEnemyTeam(DeadCharacter);
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
		if (Enemy)
		{
			// 시야와 관계없이 전투에 들어가도록 플레이어에게 맞은 것과 같은 자극을 줍니다. 엔진은 Strength가 0인 자극을 일어나지
			// 않은 것으로 보아 기억에 넣지 않으므로, 공격에 맞았을 때와 같은 1로 보냅니다.
			const FVector PlayerLocation = Player.GetActorLocation();
			UAISense_Damage::ReportDamageEvent(this, Enemy, &Player, 1.f, PlayerLocation, PlayerLocation);
		}
	}
}
