// Copyright Woogle. All Rights Reserved.

#include "AI/AgBossAIController.h"

#include "AbilitySystem/Abilities/AgBossPatternAbility.h"
#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/AgBossCharacter.h"
#include "Combat/AgCombatLibrary.h"
#include "Core/AgGameplayTags.h"
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

void AAgBossAIController::GatherCandidates(TConstArrayView<FAgBossPattern> Patterns, float Distance, float NearDistance, float FarDistance, int32 Phase,
	TFunctionRef<bool(const FAgBossPattern&)> IsReady, TArray<int32>& OutIndices, TArray<float>& OutWeights)
{
	OutIndices.Reset();
	OutWeights.Reset();
	for (int32 Index = 0; Index < Patterns.Num(); ++Index)
	{
		// 거리 구간: 근거리 up to NearDistance, 중거리 up to FarDistance, 원거리 beyond.
		const FAgBossPattern& Pattern = Patterns[Index];
		const float Weight = Distance <= NearDistance ? Pattern.WeightNear : (Distance <= FarDistance ? Pattern.WeightMid : Pattern.WeightFar);
		const bool bPhaseAllowed = Phase >= 2 ? Pattern.bPhase2 : Pattern.bPhase1;
		if (Weight > 0.f && bPhaseAllowed && IsReady(Pattern))
		{
			OutIndices.Add(Index);
			OutWeights.Add(Weight);
		}
	}
}

int32 AAgBossAIController::GetPhase() const
{
	return Boss.IsValid() ? Boss->GetPhase() : 1;
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
	UAbilitySystemComponent* ASC = Boss->GetAbilitySystemComponent();
	ASC->OnAbilityEnded.AddUObject(this, &ThisClass::HandleAbilityEnded);
	ASC->RegisterGameplayTagEvent(AgGameplayTags::State_Groggy, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::HandleGroggyChanged);

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

	TryStartPhaseTransition();

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
	const FGameplayAbilityActorInfo* ActorInfo = Boss->GetAbilitySystemComponent()->AbilityActorInfo.Get();
	TArray<int32> Candidates;
	TArray<float> Weights;
	GatherCandidates(Data->Patterns, Distance, Data->NearDistance, Data->FarDistance, GetPhase(), [this, ActorInfo](const FAgBossPattern& Pattern)
	{
		const FGameplayAbilitySpec* Spec = Boss->FindPatternSpec(Pattern.Pattern);
		return Spec && Spec->Ability && Spec->Ability->CanActivateAbility(Spec->Handle, ActorInfo);
	}, Candidates, Weights);

	// 2. Pick by weight.
	const int32 Picked = PickWeighted(Weights, FMath::FRand());
	if (Picked != INDEX_NONE)
	{
		const FAgBossPattern& Pattern = Data->Patterns[Candidates[Picked]];
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

	// 페이즈 전환 3: wait, then pick a phase 2 pattern.
	if (State == EState::PhaseTransition && Data && EndedData.AbilityThatEnded && EndedData.AbilityThatEnded->GetAssetTags().HasTag(AgGameplayTags::Ability_Reaction_PhaseTransition))
	{
		if (Boss.IsValid() && !Boss->IsDead())
		{
			Wait(Data->FixedRecoveryWait);
		}
		return;
	}

	const FGameplayAbilitySpec* Spec = Boss.IsValid() ? Boss->GetAbilitySystemComponent()->FindAbilitySpecFromHandle(EndedData.AbilitySpecHandle) : nullptr;
	if (State != EState::Pattern || !Data || !Spec || UAgBossPatternAbility::GetPatternTag(*Spec) != RunningPattern)
	{
		return;
	}
	// A pattern cut off by groggy waits for the groggy to end instead.
	if (Boss->GetAbilitySystemComponent()->HasMatchingGameplayTag(AgGameplayTags::State_Groggy))
	{
		return;
	}

	// 패턴 후딜레이
	RunningPattern = FGameplayTag();
	const FFloatInterval& WaitRange = GetPhase() == 1 ? Data->PatternRecoveryWaitPhase1 : Data->PatternRecoveryWaitPhase2;
	Wait(FMath::FRandRange(WaitRange.Min, WaitRange.Max));
}

void AAgBossAIController::HandleGroggyChanged(const FGameplayTag Tag, int32 NewCount)
{
	const UAgBossData* Data = GetBossData();
	if (NewCount > 0)
	{
		GetWorldTimerManager().ClearTimer(WaitTimer);
		GetWorldTimerManager().ClearTimer(RetryTimer);
		RunningPattern = FGameplayTag();
		State = EState::Stopped;
	}
	else if (Data && Boss.IsValid() && !Boss->IsDead())
	{
		// 그로기가 끝나면 잠시 기다린 뒤 다음 패턴을 선택합니다.
		Wait(Data->FixedRecoveryWait);
	}
}

void AAgBossAIController::TryStartPhaseTransition()
{
	const UAgBossData* Data = GetBossData();
	if (bPhaseTransitionStarted || !Data || !Boss.IsValid() || Boss->IsDead() || (State != EState::Waiting && State != EState::Approaching))
	{
		return;
	}
	const UAgAttributeSet* Stats = Boss->GetAttributeSet();
	if (Stats->GetHP() > Stats->GetMaxHP() * Data->Phase2HPRatio || Boss->GetAbilitySystemComponent()->HasMatchingGameplayTag(AgGameplayTags::State_Groggy))
	{
		return;
	}

	bPhaseTransitionStarted = true;
	GetWorldTimerManager().ClearTimer(WaitTimer);
	GetWorldTimerManager().ClearTimer(RetryTimer);
	State = EState::PhaseTransition;

	FGameplayEventData Payload;
	Payload.EventTag = AgGameplayTags::Event_PhaseTransition;
	Payload.Target = Boss.Get();
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Boss.Get(), AgGameplayTags::Event_PhaseTransition, Payload);
}

void AAgBossAIController::HandleBossDied(AAgCharacterBase* DeadBoss)
{
	GetWorldTimerManager().ClearTimer(WaitTimer);
	GetWorldTimerManager().ClearTimer(RetryTimer);
	State = EState::Stopped;
}
