// Copyright Woogle. All Rights Reserved.

#include "AI/AgBossAIController.h"

#include "AbilitySystem/Abilities/AgBossPatternAbility.h"
#include "AbilitySystemComponent.h"
#include "Character/AgBossCharacter.h"
#include "Combat/AgCombatLibrary.h"
#include "Data/AgCharacterData.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AAgBossAIController::AAgBossAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

int32 AAgBossAIController::PickWeighted(TConstArrayView<float> Weights, float Random01)
{
	float Total = 0.f;
	for (const float Weight : Weights)
	{
		Total += FMath::Max(0.f, Weight);
	}
	if (Total <= 0.f)
	{
		return INDEX_NONE;
	}

	// 뽑힐 확률 = 그 패턴의 가중치 ÷ 후보 가중치의 합
	float Threshold = FMath::Clamp(Random01, 0.f, 1.f) * Total;
	int32 LastPositive = INDEX_NONE;
	for (int32 Index = 0; Index < Weights.Num(); ++Index)
	{
		const float Weight = FMath::Max(0.f, Weights[Index]);
		if (Weight <= 0.f)
		{
			continue;
		}
		LastPositive = Index;
		if (Threshold < Weight)
		{
			return Index;
		}
		Threshold -= Weight;
	}
	return LastPositive;
}

void AAgBossAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	Boss = Cast<AAgBossCharacter>(InPawn);
	const UAgBossData* Data = GetBossData();
	if (!Boss.IsValid() || !Data)
	{
		return;
	}

	Boss->OnDied.AddUObject(this, &ThisClass::HandleBossDied);
	Boss->GetAbilitySystemComponent()->OnAbilityEnded.AddUObject(this, &ThisClass::HandleAbilityEnded);

	// 레벨이 시작되면 패턴 후딜레이 대기 시간만큼 기다린 뒤 패턴 선택을 시작합니다.
	Wait(Data->FixedRecoveryWait);
}

void AAgBossAIController::OnUnPossess()
{
	GetWorldTimerManager().ClearTimer(WaitTimer);
	GetWorldTimerManager().ClearTimer(RetryTimer);
	State = EState::Stopped;

	Super::OnUnPossess();
}

void AAgBossAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// No candidate: walk to the player (direct movement input, no NavMesh).
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (State == EState::Approaching && Boss.IsValid() && Player)
	{
		const FVector ToPlayer = (Player->GetActorLocation() - Boss->GetActorLocation()).GetSafeNormal2D();
		Boss->AddMovementInput(ToPlayer, 1.f);
	}
}

const UAgBossData* AAgBossAIController::GetBossData() const
{
	return Boss.IsValid() ? Cast<UAgBossData>(Boss->GetCharacterData()) : nullptr;
}

void AAgBossAIController::Wait(float Seconds)
{
	State = EState::Waiting;
	GetWorldTimerManager().ClearTimer(RetryTimer);
	if (Seconds > 0.f)
	{
		GetWorldTimerManager().SetTimer(WaitTimer, this, &ThisClass::SelectPattern, Seconds, false);
	}
	else
	{
		SelectPattern();
	}
}

void AAgBossAIController::SelectPattern()
{
	const UAgBossData* Data = GetBossData();
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Data || !Boss.IsValid() || Boss->IsDead())
	{
		State = EState::Stopped;
		return;
	}

	// 1. Candidates: usable in the current distance band and phase, cooldown over.
	const float Distance = UAgCombatLibrary::GetHorizontalDistance(Boss.Get(), Player);
	TArray<const FAgBossPattern*> Candidates;
	TArray<float> Weights;
	for (const FAgBossPattern& Pattern : Data->Patterns)
	{
		const float Weight = Distance <= Data->NearDistance ? Pattern.WeightNear : (Distance <= Data->FarDistance ? Pattern.WeightMid : Pattern.WeightFar);
		const bool bPhaseAllowed = GetPhase() == 1 ? Pattern.bPhase1 : Pattern.bPhase2;
		const FGameplayAbilitySpec* Spec = Boss->FindPatternSpec(Pattern.Pattern);
		if (Weight > 0.f && bPhaseAllowed && Spec && Spec->Ability && Spec->Ability->CanActivateAbility(Spec->Handle, Boss->GetAbilitySystemComponent()->AbilityActorInfo.Get()))
		{
			Candidates.Add(&Pattern);
			Weights.Add(Weight);
		}
	}

	// 2. Pick by weight.
	const int32 Picked = PickWeighted(Weights, FMath::FRand());
	if (Picked != INDEX_NONE)
	{
		const FAgBossPattern& Pattern = *Candidates[Picked];
		RunningPattern = Pattern.Pattern;
		State = EState::Pattern;
		if (Boss->GetAbilitySystemComponent()->TryActivateAbility(Boss->FindPatternSpec(Pattern.Pattern)->Handle))
		{
			return;
		}
		RunningPattern = FGameplayTag();
	}

	// 3. No candidate: walk to the player and try again shortly.
	State = EState::Approaching;
	GetWorldTimerManager().SetTimer(RetryTimer, this, &ThisClass::SelectPattern, Data->RetryInterval, false);
}

void AAgBossAIController::HandleAbilityEnded(const FAbilityEndedData& EndedData)
{
	const UAgBossData* Data = GetBossData();
	const FGameplayAbilitySpec* Spec = Boss.IsValid() ? Boss->GetAbilitySystemComponent()->FindAbilitySpecFromHandle(EndedData.AbilitySpecHandle) : nullptr;
	if (State != EState::Pattern || !Data || !Spec || UAgBossPatternAbility::GetPatternTag(*Spec) != RunningPattern)
	{
		return;
	}

	// 패턴 후딜레이
	RunningPattern = FGameplayTag();
	const FFloatInterval& WaitRange = GetPhase() == 1 ? Data->PatternRecoveryWaitPhase1 : Data->PatternRecoveryWaitPhase2;
	Wait(FMath::FRandRange(WaitRange.Min, WaitRange.Max));
}

void AAgBossAIController::HandleBossDied(AAgCharacterBase* DeadBoss)
{
	GetWorldTimerManager().ClearTimer(WaitTimer);
	GetWorldTimerManager().ClearTimer(RetryTimer);
	State = EState::Stopped;
}
