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

AAgEnemyAIController::AAgEnemyAIController()
{
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

bool AAgEnemyAIController::IsEnemyFlinching() const
{
	const AAgEnemyCharacter* Enemy = GetEnemy();
	const UAbilitySystemComponent* ASC = Enemy ? Enemy->GetAbilitySystemComponent() : nullptr;
	return ASC && ASC->HasMatchingGameplayTag(AgGameplayTags::State_HitReaction_Flinch);
}

bool AAgEnemyAIController::IsTargetNoticed() const
{
	// 엔진은 보이지 않게 되거나 맞은 순간부터 자극의 나이를 세고, 감각의 최대 나이가 지나면 잊습니다. 대미지 감각은 진영을
	// 가리지 않지만 적대 목록은 적대 상대만 담으므로, 노리지 않는 상대(샌드백)에게 맞은 것은 빠집니다.
	TArray<AActor*> Hostiles;
	GetAIPerceptionComponent()->GetHostileActors(Hostiles);
	return !Hostiles.IsEmpty();
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

	ConfigurePerception(*Enemy->GetCharacterData());
	// 플레이어 타임라인이 기본 공격 시작 이벤트를 공격 대상에게 보내므로 자기 캐릭터에서 받습니다.
	BasicAttackHandle = Enemy->GetAbilitySystemComponent()->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_BasicAttackStarted).AddUObject(this, &ThisClass::HandleTargetBasicAttack);

	if (Enemy->AIStateTree)
	{
		StateTreeComponent->SetStateTree(Enemy->AIStateTree);
		StateTreeComponent->StartLogic();
	}
}

void AAgEnemyAIController::OnUnPossess()
{
	ClearFocus(EAIFocusPriority::Gameplay);
	// 엔진도 빙의를 풀 때 로직을 멈추지만 폰을 비운 뒤라, 폰을 쓰는 Task의 ExitState를 위해 먼저 멈춥니다.
	StateTreeComponent->StopLogic(TEXT("UnPossess"));
	if (const AAgEnemyCharacter* Enemy = GetEnemy())
	{
		Enemy->GetAbilitySystemComponent()->GenericGameplayEventCallbacks.FindOrAdd(AgGameplayTags::Event_BasicAttackStarted).Remove(BasicAttackHandle);
	}
	BasicAttackHandle.Reset();

	Super::OnUnPossess();
}

void AAgEnemyAIController::ConfigurePerception(const UAgCharacterData& Data)
{
	SightConfig->SightRadius = Data.SightDistance;
	SightConfig->LoseSightRadius = Data.SightDistance;
	SightConfig->PeripheralVisionAngleDegrees = Data.SightAngle * 0.5f;
	const float TargetLostTime = UAgAssetManager::Get().GetCombatRules().TargetLostTime;
	SightConfig->SetMaxAge(TargetLostTime);
	DamageConfig->SetMaxAge(TargetLostTime);
	// 다시 등록해야 바뀐 값과 빙의한 적의 팀으로 감각 검사를 새로 만듭니다.
	UAIPerceptionComponent* Perception = GetAIPerceptionComponent();
	Perception->ConfigureSense(*SightConfig);
	Perception->ConfigureSense(*DamageConfig);
}

void AAgEnemyAIController::HandleTargetBasicAttack(const FGameplayEventData* Payload)
{
	StateTreeComponent->SendStateTreeEvent(AgGameplayTags::Event_BasicAttackStarted);
}
