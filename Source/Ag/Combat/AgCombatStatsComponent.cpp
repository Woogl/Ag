// Copyright Woogle. All Rights Reserved.

#include "Combat/AgCombatStatsComponent.h"

#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "Character/AgEnemyCharacter.h"
#include "Combat/AgCombatSubsystem.h"
#include "Core/AgGameplayTags.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Game/AgGameState.h"
#include "GameplayEffect.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/DateTime.h"

namespace AgCombatStatsComponentPrivate
{
	bool IsStageResultDecided(const UWorld& World)
	{
		const AAgGameState* StageState = World.GetGameState<AAgGameState>();
		return StageState && StageState->GetStageResult() != EAgStageResult::None;
	}
} // namespace AgCombatStatsComponentPrivate

UAgCombatStatsComponent::UAgCombatStatsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAgCombatStatsComponent::BeginPlay()
{
	Super::BeginPlay();

	UAgAbilitySystemComponent* ASC = CastChecked<AAgCharacterBase>(GetOwner())->GetAbilitySystemComponent();
	ParryHandle = ASC->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_Parry).AddUObject(this, &ThisClass::HandleParry);
	PerfectDodgeHandle = ASC->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_PerfectDodge).AddUObject(this, &ThisClass::HandlePerfectDodge);
	ASC->OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &ThisClass::HandleEffectApplied);

	GetWorld()->GetSubsystem<UAgCombatSubsystem>()->OnCombatantsChanged.AddUObject(this, &ThisClass::HandleCombatantsChanged);
	if (AAgGameState* StageState = GetWorld()->GetGameState<AAgGameState>())
	{
		StageState->OnStageResultChanged.AddUObject(this, &ThisClass::HandleStageResultChanged);
	}
	HandleCombatantsChanged();
}

void UAgCombatStatsComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UAgAbilitySystemComponent* ASC = CastChecked<AAgCharacterBase>(GetOwner())->GetAbilitySystemComponent();
	ASC->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_Parry).Remove(ParryHandle);
	ASC->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_PerfectDodge).Remove(PerfectDodgeHandle);
	ASC->OnGameplayEffectAppliedDelegateToSelf.RemoveAll(this);

	if (UAgCombatSubsystem* CombatSubsystem = GetWorld()->GetSubsystem<UAgCombatSubsystem>())
	{
		CombatSubsystem->OnCombatantsChanged.RemoveAll(this);
	}
	if (AAgGameState* StageState = GetWorld()->GetGameState<AAgGameState>())
	{
		StageState->OnStageResultChanged.RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);
}

void UAgCombatStatsComponent::HandleCombatantsChanged()
{
	// 레벨을 내릴 때 적의 StateTree가 멈추며 전투 목록을 비우지만, 그것은 전투가 끝난 것이 아닙니다.
	if (GetWorld()->bIsTearingDown)
	{
		return;
	}
	TArray<AAgEnemyCharacter*> Combatants;
	GetWorld()->GetSubsystem<UAgCombatSubsystem>()->GetCombatants(Combatants);
	if (!Recording)
	{
		if (Combatants.IsEmpty() || AgCombatStatsComponentPrivate::IsStageResultDecided(*GetWorld()))
		{
			return;
		}
		Recording.Emplace();
		Recording->Level = UGameplayStatics::GetCurrentLevelName(this, true);
		Recording->Attempt = GetWorld()->GetGameInstance()->GetSubsystem<UAgCombatStatsSubsystem>()->GetAttempt();
		// 일시정지에는 멈추고 시간 배율은 받지 않는 시계입니다.
		StartTime = GetWorld()->GetAudioTimeSeconds();
	}

	for (const AAgEnemyCharacter* Enemy : Combatants)
	{
		if (Enemies.Contains(Enemy))
		{
			continue;
		}
		Enemies.Add(Enemy);
		const FVector Home = Enemy->GetHomeTransform().GetLocation();
		FAgCombatEnemyStats& EnemyStats = Recording->Enemies.AddDefaulted_GetRef();
		EnemyStats.Id = Enemy->GetCharacterDataId();
		EnemyStats.Location = FIntVector(FMath::RoundToInt32(Home.X), FMath::RoundToInt32(Home.Y), FMath::RoundToInt32(Home.Z));
	}

	if (!Combatants.IsEmpty())
	{
		return;
	}
	const bool bEnemyAlive = Enemies.ContainsByPredicate([](const TWeakObjectPtr<const AAgEnemyCharacter>& Enemy)
	{ return Enemy.IsValid() && !Enemy->IsDead(); });
	if (CastChecked<AAgCharacterBase>(GetOwner())->IsDead())
	{
		EndRecording(EAgCombatResult::Death);
	}
	else
	{
		EndRecording(bEnemyAlive ? EAgCombatResult::Disengage : EAgCombatResult::Victory);
	}
}

void UAgCombatStatsComponent::HandleStageResultChanged()
{
	// Game Over는 적이 모두 대기로 빠져 전투 중인 적이 없어질 때 끝냅니다. 쓰러뜨린 공격의 적용 알림(피격 횟수)이 그 공격의
	// 사망 처리보다 늦게 오기 때문입니다.
	const AAgGameState* StageState = GetWorld()->GetGameState<AAgGameState>();
	if (Recording && StageState->GetStageResult() == EAgStageResult::Clear)
	{
		EndRecording(EAgCombatResult::Victory);
	}
}

void UAgCombatStatsComponent::HandleParry(const FGameplayEventData* Payload)
{
	if (Recording)
	{
		++Recording->ParryCount;
	}
}

void UAgCombatStatsComponent::HandlePerfectDodge(const FGameplayEventData* Payload)
{
	if (Recording)
	{
		++Recording->PerfectDodgeCount;
	}
}

void UAgCombatStatsComponent::HandleEffectApplied(UAbilitySystemComponent* Source, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle)
{
	if (Recording && Spec.GetDynamicAssetTags().HasTagExact(AgGameplayTags::Effect_Damage_Result_Hit))
	{
		++Recording->HitCount;
	}
}

void UAgCombatStatsComponent::EndRecording(EAgCombatResult Result)
{
	Recording->Result = Result;
	Recording->Duration = static_cast<float>(GetWorld()->GetAudioTimeSeconds() - StartTime);
	Recording->EndedAt = FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M:%S"));
	if (AgCombatStatsComponentPrivate::IsStageResultDecided(*GetWorld()))
	{
		StageResultStats = Recording;
	}
	GetWorld()->GetGameInstance()->GetSubsystem<UAgCombatStatsSubsystem>()->Save(*Recording);
	Recording.Reset();
	Enemies.Reset();
}
