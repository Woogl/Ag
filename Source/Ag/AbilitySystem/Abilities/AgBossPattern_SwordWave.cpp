// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgBossPattern_SwordWave.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Character/AgBossCharacter.h"
#include "Combat/AgSwordWave.h"
#include "Components/CapsuleComponent.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgAttackData.h"
#include "Data/AgCharacterData.h"
#include "Engine/World.h"

void UAgBossPattern_SwordWave::OnPatternStarted()
{
	WaveIndex = 0;
	UAbilityTask_WaitGameplayEvent* ReleaseTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Event_Boss_SwordWave, nullptr, /*OnlyTriggerOnce*/ false);
	ReleaseTask->EventReceived.AddDynamic(this, &ThisClass::HandleRelease);
	ReleaseTask->ReadyForActivation();
}

void UAgBossPattern_SwordWave::GetParryableHitStarts(TArray<float>& OutTimes) const
{
	// Each wave's launch is its attack window (보스 사양 'A6').
	const FAgBossPattern* Pattern = GetPattern();
	TArray<float> Launches;
	FindEventNotifyTimes(AgGameplayTags::Event_Boss_SwordWave, Launches);
	for (int32 Index = 0; Index < Launches.Num(); ++Index)
	{
		if (Pattern && Pattern->Hits.IsValidIndex(Index) && Pattern->Hits[Index].bParryable)
		{
			OutTimes.Add(Launches[Index]);
		}
	}
}

void UAgBossPattern_SwordWave::HandleRelease(FGameplayEventData Payload)
{
	AAgBossCharacter* Boss = GetBoss();
	const UAgBossData* Data = GetCharacterData<UAgBossData>();
	const FAgBossPattern* Pattern = GetPattern();
	if (!Boss || !Data || !Data->SwordWaveClass || !Pattern || !Pattern->Hits.IsValidIndex(WaveIndex))
	{
		return;
	}

	// From the ground in front of the boss, straight ahead.
	const FVector Direction = Boss->GetActorForwardVector().GetSafeNormal2D();
	const UCapsuleComponent* Capsule = Boss->GetCapsuleComponent();
	const FVector Location = Boss->GetActorLocation() + Direction * Capsule->GetScaledCapsuleRadius() - FVector(0.f, 0.f, Capsule->GetScaledCapsuleHalfHeight());

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = Boss;
	SpawnParameters.Instigator = Boss;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AAgSwordWave* Wave = GetWorld()->SpawnActor<AAgSwordWave>(Data->SwordWaveClass, Location, Direction.Rotation(), SpawnParameters))
	{
		const FVector Size(Data->SwordWaveDepth, Data->SwordWaveWidth, Data->SwordWaveHeight);
		Wave->Launch(Boss, Pattern->Hits[WaveIndex], Direction, Data->SwordWaveSpeed, Data->SwordWaveRange, Size);
	}
	++WaveIndex;
}
