// Copyright Woogle. All Rights Reserved.

#include "AI/AgEnemyAIController.h"

#include "AbilitySystemComponent.h"
#include "Character/AgEnemyCharacter.h"
#include "Components/StateTreeAIComponent.h"
#include "Core/AgAssetManager.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "Data/AgCombatRules.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"

AAgEnemyAIController::AAgEnemyAIController()
{
	PrimaryActorTick.bCanEverTick = true;

	StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTree"));
	// 실행할 StateTree는 빙의할 때 적 캐릭터에서 가져오므로 BeginPlay에 자동으로 시작하지 않습니다.
	StateTreeComponent->SetStartLogicAutomatically(false);

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("Sight"));
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = false;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
	DamageConfig = CreateDefaultSubobject<UAISenseConfig_Damage>(TEXT("Damage"));

	UAIPerceptionComponent* Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));
	Perception->ConfigureSense(*SightConfig);
	Perception->ConfigureSense(*DamageConfig);
	Perception->SetDominantSense(SightConfig->GetSenseImplementation());
	SetPerceptionComponent(*Perception);
}

AAgEnemyCharacter* AAgEnemyAIController::GetEnemy() const
{
	return GetPawn<AAgEnemyCharacter>();
}

const UAgEnemyData* AAgEnemyAIController::GetEnemyData() const
{
	const AAgEnemyCharacter* Enemy = GetEnemy();
	return Enemy ? Cast<UAgEnemyData>(Enemy->GetCharacterData()) : nullptr;
}

AAgCharacterBase* AAgEnemyAIController::GetTarget() const
{
	const AAgEnemyCharacter* Enemy = GetEnemy();
	return Enemy ? Enemy->GetLockOnTarget() : nullptr;
}

bool AAgEnemyAIController::IsEnemyReacting() const
{
	const AAgEnemyCharacter* Enemy = GetEnemy();
	const UAbilitySystemComponent* ASC = Enemy ? Enemy->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return false;
	}

	FGameplayTagContainer ReactionTags;
	ReactionTags.AddTag(AgGameplayTags::State_HitReaction);
	ReactionTags.AddTag(AgGameplayTags::State_Groggy);
	ReactionTags.AddTag(AgGameplayTags::State_Execution);
	return ASC->HasAnyMatchingGameplayTags(ReactionTags);
}

bool AAgEnemyAIController::IsTargetNoticed() const
{
	if (bTargetInSight)
	{
		return true;
	}
	return LastNoticedTime >= 0. && GetWorld()->GetTimeSeconds() - LastNoticedTime < UAgAssetManager::Get().GetCombatRules().TargetLostTime;
}

FGenericTeamId AAgEnemyAIController::GetGenericTeamId() const
{
	const AAgEnemyCharacter* Enemy = GetEnemy();
	return Enemy ? Enemy->GetGenericTeamId() : FGenericTeamId::NoTeam;
}

ETeamAttitude::Type AAgEnemyAIController::GetTeamAttitudeTowards(const AActor& Other) const
{
	const AAgEnemyCharacter* Enemy = GetEnemy();
	return Enemy ? Enemy->GetTeamAttitudeTowards(Other) : ETeamAttitude::Neutral;
}

void AAgEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	const AAgEnemyCharacter* Enemy = GetEnemy();
	if (!Enemy)
	{
		return;
	}

	bTargetInSight = false;
	LastNoticedTime = -1.;
	GetAIPerceptionComponent()->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &ThisClass::HandleTargetPerceptionUpdated);
	ConfigureSight(*GetEnemyData());

	if (Enemy->AIStateTree)
	{
		StateTreeComponent->SetStateTree(Enemy->AIStateTree);
		StateTreeComponent->StartLogic();
	}
}

void AAgEnemyAIController::OnUnPossess()
{
	ClearFocus(EAIFocusPriority::Gameplay);
	StateTreeComponent->StopLogic(TEXT("UnPossess"));
	GetAIPerceptionComponent()->OnTargetPerceptionUpdated.RemoveDynamic(this, &ThisClass::HandleTargetPerceptionUpdated);
	if (TargetASC.IsValid())
	{
		TargetASC->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_BasicAttackStarted).Remove(BasicAttackHandle);
	}
	TargetASC.Reset();
	BasicAttackHandle.Reset();

	Super::OnUnPossess();
}

void AAgEnemyAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	BindTargetEvents();
}

void AAgEnemyAIController::ConfigureSight(const UAgEnemyData& Data)
{
	SightConfig->SightRadius = Data.SightDistance;
	SightConfig->LoseSightRadius = Data.SightDistance;
	SightConfig->PeripheralVisionAngleDegrees = Data.SightAngle * 0.5f;
	// 다시 등록해야 바뀐 값과 빙의한 적의 팀으로 시야 검사를 새로 만듭니다.
	GetAIPerceptionComponent()->ConfigureSense(*SightConfig);
}

void AAgEnemyAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	// 대미지 감각은 진영을 가리지 않으므로, 노리지 않는 상대(샌드백)에게 맞은 것은 여기서 거릅니다.
	if (!Actor || GetTeamAttitudeTowards(*Actor) != ETeamAttitude::Hostile)
	{
		return;
	}
	if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
	{
		bTargetInSight = Stimulus.WasSuccessfullySensed();
	}
	// 보이기 시작하거나 놓치거나 맞은 순간이므로, 놓친 시간은 여기서부터 셉니다.
	LastNoticedTime = GetWorld()->GetTimeSeconds();
}

void AAgEnemyAIController::BindTargetEvents()
{
	// 적은 플레이어가 스폰되기 전에 빙의하므로, 빙의할 때가 아니라 대상이 생긴 뒤의 틱에서 한 번 구독합니다.
	if (TargetASC.IsValid())
	{
		return;
	}
	if (const AAgCharacterBase* Target = GetTarget())
	{
		TargetASC = Target->GetAbilitySystemComponent();
		BasicAttackHandle = TargetASC->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_BasicAttackStarted)
		                        .AddUObject(this, &ThisClass::HandleTargetBasicAttack);
	}
}

void AAgEnemyAIController::HandleTargetBasicAttack(const FGameplayEventData* Payload)
{
	const AAgEnemyCharacter* Enemy = GetEnemy();
	if (Payload && Enemy && Payload->Target.Get() == Enemy)
	{
		StateTreeComponent->SendStateTreeEvent(AgGameplayTags::Event_BasicAttackStarted);
	}
}
