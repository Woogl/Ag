// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/AgAbility_Execution.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/AgEffect_Damage.h"
#include "AbilitySystem/Tasks/AgAbilityTask_AttackMovement.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Ag.h"
#include "Character/AgPlayerCharacter.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "GameFramework/CharacterMovementComponent.h"

UAgAbility_Execution::UAgAbility_Execution()
{
	SetupPlayerAction(AgGameplayTags::Ability_Action_Execution);

	// 처형하는 동안(상태 우선순위 2) 무적입니다.
	ActivationOwnedTags.AddTag(AgGameplayTags::State_Execution_Executing);
	ActivationOwnedTags.AddTag(AgGameplayTags::State_Invincible);
}

AAgCharacterBase* UAgAbility_Execution::FindExecutionTarget(const AAgPlayerCharacter* Player)
{
	if (!Player)
	{
		return nullptr;
	}
	const UAgPlayerData* Data = CastChecked<UAgPlayerData>(Player->GetCharacterData());

	// 그로기 중이고, 이번 그로기에 아직 처형되지 않았고, 땅에 있는 적입니다(공중에서 그로기가 되면 착지 전에는 처형할 수 없음).
	return Player->FindEnemy(Data->Execution.TargetingRange, [](const AAgCharacterBase& Enemy)
	{
		const UAbilitySystemComponent* EnemyASC = Enemy.GetAbilitySystemComponent();
		return EnemyASC->HasMatchingGameplayTag(AgGameplayTags::State_Groggy) && !EnemyASC->HasMatchingGameplayTag(AgGameplayTags::State_Execution_Executed)
			&& !Enemy.GetCharacterMovement()->IsFalling();
	});
}

bool UAgAbility_Execution::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	const AAgPlayerCharacter* Player = ActorInfo ? Cast<AAgPlayerCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags) && FindExecutionTarget(Player);
}

void UAgAbility_Execution::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AAgPlayerCharacter* Player = CastChecked<AAgPlayerCharacter>(GetAgCharacter());
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	AAgCharacterBase* Enemy = FindExecutionTarget(Player);
	if (!Enemy || !Data->ExecutionMontage || Data->Execution.Hits.IsEmpty() || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	Target = Enemy;
	StrikeIndex = 0;

	// 후딜이 없습니다. 행동 중이 모션 끝까지 이어져 어떤 행동도 처형을 끊지 못합니다.
	BeginBusy();

	// 대상은 같은 프레임에 플레이어를 향하고 처형 피격 몽타주를 시작하므로, 두 몽타주가 한 타임라인을 따릅니다.
	FGameplayEventData Payload;
	Payload.EventTag = AgGameplayTags::Event_Executed;
	Payload.Instigator = Player;
	Payload.Target = Enemy;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Enemy, AgGameplayTags::Event_Executed, Payload);

	// 찌르기(워프 구간의 끝)까지 플레이어는 대상 정면, 두 모션이 맞물리는 간격에 섭니다. 정지 간격은 적용하지 않습니다.
	UAgAbilityTask_AttackMovement::FitGap(this, Enemy, Data->ExecutionDistance)->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* StrikeTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, AgGameplayTags::Event_ExecutionBlow);
	StrikeTask->EventReceived.AddDynamic(this, &ThisClass::HandleStrike);
	StrikeTask->ReadyForActivation();

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Data->ExecutionMontage);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UAgAbility_Execution::HandleStrike(FGameplayEventData Payload)
{
	AAgCharacterBase* Enemy = Target.Get();
	const UAgPlayerData* Data = GetCharacterData<UAgPlayerData>();
	if (!Data->Execution.Hits.IsValidIndex(StrikeIndex))
	{
		return;
	}
	FAgPlayerAttackHit Hit = Data->Execution.Hits[StrikeIndex];
	++StrikeIndex;
	Hit.bFinisher = StrikeIndex == Data->Execution.Hits.Num();

	// 타격마다 처형 대상(보스의 캡슐 중심)에 대미지 GE를 적용합니다. 이 어빌리티가 만든 스펙에는 엔진이 어빌리티 태그
	// (Ability.Action.Execution)를 넣어, 대미지 GE가 처형의 셰이크를 쓰고 히트스톱을 걸지 않으며, 처형으로 죽으면 사망
	// 어빌리티가 처형 사망 모션을 이어서 재생합니다. 앞 타격에 이미 죽은 대상에는 적용되지 않습니다.
	if (Enemy)
	{
		const FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(UAgEffect_Damage::StaticClass());
		UAgEffect_Damage::SetAttack(*Spec.Data, Hit, Enemy->GetActorLocation());
		GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(*Spec.Data, Enemy->GetAbilitySystemComponent());
		UE_LOG(LogAg, Verbose, TEXT("Execution strike %d: HP now %.0f"), StrikeIndex, Enemy->GetAttributeSet()->GetHP());
	}
}

void UAgAbility_Execution::HandleMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UAgAbility_Execution::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
