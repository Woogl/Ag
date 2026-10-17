// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/Tasks/AgAbilityTask_Attack.h"
#include "AbilitySystem/Tasks/AgAbilityTask_HoldTag.h"
#include "AbilitySystem/Tasks/AgAbilityTask_TimeStop.h"
#include "AbilitySystem/Tasks/AgAbilityTask_PlayCutscene.h"
#include "AbilitySystem/Tasks/AgAbilityTask_SetCollisionResponseToChannel.h"
#include "AbilitySystem/Tasks/AgAbilityTask_LockOn.h"
#include "AbilitySystem/Tasks/AgAbilityTask_Dash.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionJumpForce.h"
#include "Abilities/Tasks/AbilityTask_WaitVelocityChange.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/AgCharacterBase.h"
#include "Character/AgPlayerCharacter.h"
#include "Character/AgBossCharacter.h"
#include "Combat/AgTimeSubsystem.h"
#include "Core/AgAssetManager.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCharacterData.h"
#include "Data/AgEffectData.h"
#include "Data/AgCombatRules.h"
#include "Data/AgCameraData.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Game/AgPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Ag.h"

void UAgGameplayAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
    FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* Trigger)
{
	bEnding = false;
	bRootMotionTranslationFromVelocity = false;
	const FAgAbilityData* Definition = FindDefinition(Handle, ActorInfo);
	if (!Definition)
	{
		Finish(true);
		return;
	}
	ExecutionDefinition = *Definition;
	AAgCharacterBase* Character = GetAgCharacter();
	AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(Character);
	AAgBossCharacter* Boss = Cast<AAgBossCharacter>(Character);
	const UAgCharacterData* Data = Character->GetCharacterData();
	ExecutionMontages = Data->Montages;
	ExecutionProjectiles = UAgAssetManager::Get().GetProjectiles();
	WarpStopDistance = Data->StopDistance;
	if (const UAgPlayerData* PlayerData = Cast<UAgPlayerData>(Data))
	{
		ComboMontageIds = PlayerData->BasicAttackMontageIds;
	}
	const FName Id = GetAbilityId();
	ExecutionTarget = Id == TEXT("Execution") ? FindExecutionTarget(Player) : nullptr;
	const float BeforeSP = Character->GetAttributeSet()->GetSP();
	if (!CheckStartConditions(ExecutionDefinition, ActorInfo) || !CheckResources(ExecutionDefinition, ActorInfo) ||
	    !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		Finish(true);
		return;
	}
	if (!IsActive())
	{
		return;
	}
	SPSpent = BeforeSP - Character->GetAttributeSet()->GetSP();
	bPerfectDodged = bHoldingDash = bDashFinished = bGettingUp = false;
	ExecutionStrike = 0;
	const FAgAbilityTasks& Tasks = ExecutionDefinition.Tasks;
	if (Tasks.bHasDrainResource)
	{
		// 주기마다 초당 소모량 × 주기만큼 줄입니다.
		const FGameplayEffectSpecHandle Spec = MakeEffectSpec(Handle, ActorInfo, EAgEffect::ResourceDrain);
		if (Spec.IsValid())
		{
			Tasks.DrainResource.AddToSpec(*Spec.Data);
		}
		const FActiveGameplayEffectHandle Applied = ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
		if (!IsActive())
		{
			if (Applied.IsValid())
			{
				ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(Applied);
			}
			return;
		}
		DrainEffect = Applied;
		if (!DrainEffect.IsValid())
		{
			Finish(true);
			return;
		}
	}
	if (Id == TEXT("BasicAttack"))
	{
		Listen(AgGameplayTags::Input_Attack);
		StartComboHit(0);
		return;
	}
	if (Id == TEXT("Dodge"))
	{
		Listen(AgGameplayTags::Event_PerfectDodge);
		const FVector Input = Player->GetMoveInputDirection();
		if (!Input.IsNearlyZero())
		{
			Player->SetActorRotation(FRotator(0.f, Input.Rotation().Yaw, 0.f));
		}
		PlayMotion(Input.IsNearlyZero() ? TEXT("Backstep") : TEXT("Roll"), true);
		return;
	}
	if (Id == TEXT("Guard"))
	{
		Listen(AgGameplayTags::Input_Guard);
		Listen(AgGameplayTags::Input_GuardReleased);
		Listen(AgGameplayTags::Event_Parry);
		StartGuard(!(Trigger && Trigger->EventTag == AgGameplayTags::Event_Guard_Resume));
		return;
	}
	if (Id == TEXT("Jump"))
	{
		BeginBusy();
		if (!IsActive())
		{
			return;
		}
		WaitForLanding();
		Player->Jump();
		PlayMotion(TEXT("Jump"));
		return;
	}
	if (Id == TEXT("LockOn"))
	{
		Listen(AgGameplayTags::Input_LockOn);
		UAgAbilityTask_LockOn* Follow = UAgAbilityTask_LockOn::FollowTarget(this, UAgAbilityTask_LockOn::FindTarget(*Player));
		LockOnTask = Follow;
		Follow->OnTargetLost.AddDynamic(this, &ThisClass::HandleTargetLost);
		Follow->ReadyForActivation();
		return;
	}
	if (Id == TEXT("Groggy"))
	{
		Listen(AgGameplayTags::Event_Executed);
		if (Character->GetCharacterMovement()->IsFalling())
		{
			WaitForLanding();
		}
		else
		{
			StartGroggy();
		}
		return;
	}
	if (Id == TEXT("Death"))
	{
		StartDeath(Trigger);
		return;
	}
	if (Id == TEXT("KnockUp"))
	{
		FaceActor(Trigger ? Trigger->Instigator.Get() : nullptr);
		StartKnockUp();
		return;
	}
	if (ExecutionDefinition.Tags.HasTag(AgGameplayTags::Ability_Reaction_HitReaction))
	{
		FaceActor(Trigger ? Trigger->Instigator.Get() : nullptr);
	}
	if (Id == TEXT("Execution"))
	{
		if (!ExecutionTarget.IsValid())
		{
			Finish(true);
			return;
		}
		WarpStopDistance = CastChecked<UAgPlayerData>(Data)->ExecutionDistance;
		BeginBusy();
		if (!IsActive())
		{
			return;
		}
		FGameplayEventData Event;
		Event.EventTag = AgGameplayTags::Event_Executed;
		Event.Instigator = Character;
		Event.Target = ExecutionTarget.Get();
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(ExecutionTarget.Get(), Event.EventTag, Event);
		if (!IsActive())
		{
			return;
		}
	}
	if (Id == TEXT("Ultimate"))
	{
		BeginBusy();
		if (!IsActive())
		{
			return;
		}
		UAgAbilityTask_TimeStop* Stop = UAgAbilityTask_TimeStop::TimeStop(this);
		UltimateTimeStop = Stop;
		Stop->ReadyForActivation();
		if (!IsActive() || Stop->IsFinished())
		{
			Finish(true);
			return;
		}
	}
	if (Id == TEXT("JumpAttack") || Id == TEXT("LeapSlam"))
	{
		bRootMotionTranslationFromVelocity = true;
		WaitForLanding();
	}
	if (Id == TEXT("DashThrust"))
	{
		Listen(AgGameplayTags::Event_AttackBlocked);
	}
	if (Tasks.bHasPlayMontageAndWait)
	{
		const FAgPlayMontageAndWaitParams& Params = Tasks.PlayMontageAndWait;
		UAnimMontage* Montage = ExecutionMontages.FindRef(Params.MontageId);
		float Scale = 1.f;
		if (Params.bHasRootMotionDistance && Montage)
		{
			const float OriginalDistance =
			    Montage->ExtractRootMotionFromTrackRange(0.f, Montage->GetPlayLength(), FAnimExtractContext()).GetTranslation().Size2D();
			if (OriginalDistance <= UE_SMALL_NUMBER)
			{
				Finish(true);
				return;
			}
			Scale = Params.RootMotionDistance / OriginalDistance;
		}
		const bool bAttackPattern = Boss && ExecutionDefinition.Tags.HasTag(AgGameplayTags::Ability_Boss_Pattern) && Id != TEXT("Backstep");
		const bool bPlayerAction = Player && ExecutionDefinition.Tags.HasTag(AgGameplayTags::Ability_Action);
		if (!PlayMotion(
		        Params.MontageId, bPlayerAction, bAttackPattern ? CastChecked<UAgBossData>(Data)->PatternPlayRate : 1.f, NAME_None, Scale))
		{
			return;
		}
	}
	if (!IsActive())
	{
		return;
	}
	if (Id == TEXT("JumpAttack"))
	{
		NotifyBasicAttackStarted();
	}
	if (Id == TEXT("LeapSlam"))
	{
		if (!Boss->GetTarget())
		{
			Finish(true);
			return;
		}
		const FAgLaunchToTargetParams& Params = Tasks.LaunchToTarget;
		const FVector Delta = (Boss->GetTarget()->GetActorLocation() - Character->GetActorLocation()) * FVector(1.f, 1.f, 0.f);
		const float Distance = FMath::Clamp(Delta.Size() - Params.TargetOffset, 0.f, Params.MaxDistance);
		const float Gravity = -Character->GetCharacterMovement()->GetGravityZ();
		Character->LaunchCharacter(
		    Delta.GetSafeNormal() * Distance / Params.AirTime + FVector(0.f, 0.f, Gravity * Params.AirTime * 0.5f), true, true);
	}
	if (Id == TEXT("Ultimate"))
	{
		const FAgPlayCutsceneParams* Params = Tasks.PlayCutscene.Find(TEXT("UltimateCamera"));
		if (!Params)
		{
			Finish(true);
			return;
		}
		UAgAbilityTask_PlayCutscene* Cutscene = UAgAbilityTask_PlayCutscene::PlayCutscene(this, Params->Sequence, Character);
		Cutscene->OnFinished.AddDynamic(this, &ThisClass::HandleCutsceneFinished);
		Cutscene->ReadyForActivation();
	}
}

void UAgGameplayAbility::StartComboHit(int32 Index)
{
	if (!ComboMontageIds.IsValidIndex(Index))
	{
		Finish(true);
		return;
	}
	ComboIndex = Index;
	if (PlayMotion(ComboMontageIds[Index], true))
	{
		NotifyBasicAttackStarted();
	}
}

void UAgGameplayAbility::NotifyBasicAttackStarted()
{
	FGameplayEventData Event;
	Event.EventTag = AgGameplayTags::Event_BasicAttackStarted;
	Event.Instigator = GetAgCharacter();
	Event.Target = Motion.Target.Get();
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetAgCharacter(), Event.EventTag, Event);
}

void UAgGameplayAbility::FaceActor(const AActor* Actor)
{
	if (!Actor)
	{
		return;
	}
	const FVector Direction = (Actor->GetActorLocation() - GetAgCharacter()->GetActorLocation()).GetSafeNormal2D();
	if (!Direction.IsNearlyZero())
	{
		GetAgCharacter()->SetActorRotation(FRotator(0.f, Direction.Rotation().Yaw, 0.f));
	}
}

UAgAbilityTask_HoldTag* UAgGameplayAbility::HoldConfiguredTag(FName ParamsId)
{
	const FAgHoldTagParams* Params = ExecutionDefinition.Tasks.HoldTag.Find(ParamsId);
	if (!Params)
	{
		UE_LOG(LogAg, Error, TEXT("%s Abilities.%s.Tasks.HoldTag.%s: missing"), *GetAgCharacter()->GetCharacterData()->GetName(),
		    *GetAbilityId().ToString(), *ParamsId.ToString());
		return nullptr;
	}
	UAgAbilityTask_HoldTag* Task = UAgAbilityTask_HoldTag::HoldTag(this, Params->Tag);
	Task->ReadyForActivation();
	return Task;
}

void UAgGameplayAbility::ApplyReward(FName ParamsId)
{
	const FAgGrantResourcesParams* Params = ExecutionDefinition.Tasks.GrantResources.Find(ParamsId);
	if (!Params)
	{
		return;
	}
	const FGameplayEffectSpecHandle Spec = MakeEffectSpec(EAgEffect::Resource);
	if (Spec.IsValid())
	{
		Params->AddToSpec(*Spec.Data);
		ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, Spec);
	}
}

FActiveGameplayEffectHandle UAgGameplayAbility::ApplyTimedTag(const FAgTimedTagParams& Params)
{
	const FGameplayEffectSpecHandle Spec = MakeEffectSpec(EAgEffect::TimedTag);
	if (!Spec.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}
	Spec.Data->SetDuration(Params.Duration, /*bLockDuration*/ true);
	Spec.Data->DynamicGrantedTags.AddTag(Params.Tag);
	return ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, Spec);
}

void UAgGameplayAbility::ApplySingleAttack(
    FName ParamsId, const TArray<AAgCharacterBase*>& Targets, bool bFinisher, TOptional<FVector> HitLocation)
{
	if (const FAgAttackParams* Params = ExecutionDefinition.Tasks.Attack.Find(ParamsId))
	{
		UAgAbilityTask_Attack::Strike(this, *Params, Targets, bFinisher, HitLocation)->ReadyForActivation();
	}
}

AAgCharacterBase* UAgGameplayAbility::FindExecutionTarget(const AAgPlayerCharacter* Player)
{
	if (!Player)
	{
		return nullptr;
	}
	const FAgAbilityData* Definition = Player->GetCharacterData()->FindAbility(TEXT("Execution"));
	const FAgTargetQueryParams* Query = Definition ? Definition->Tasks.TargetQuery.Find(TEXT("Execution")) : nullptr;
	if (!Query)
	{
		return nullptr;
	}
	return Player->FindEnemy(Query->Range,
	    [](const AAgCharacterBase& Enemy)
	    {
		    const UAbilitySystemComponent* ASC = Enemy.GetAbilitySystemComponent();
		    return ASC->HasMatchingGameplayTag(AgGameplayTags::State_Groggy) &&
		           !ASC->HasMatchingGameplayTag(AgGameplayTags::State_Execution) && !Enemy.GetCharacterMovement()->IsFalling();
	    });
}

void UAgGameplayAbility::StartGuard(bool bOpenParry)
{
	if (!IsActive() || bEnding)
	{
		return;
	}
	SetGuarding(true);
	if (!IsActive() || bEnding)
	{
		return;
	}
	CloseParryWindow();
	if (!IsActive() || bEnding)
	{
		return;
	}
	if (bOpenParry)
	{
		const FAgTimedTagParams* Params = ExecutionDefinition.Tasks.TimedTag.Find(TEXT("ParryWindow"));
		if (Params)
		{
			UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
			const FActiveGameplayEffectHandle Applied = ApplyTimedTag(*Params);
			// 태그 획득 콜백이 Ability를 끝내면 EndAbility는 아직 이 핸들을 받지 못했습니다.
			if (!IsActive() || bEnding)
			{
				if (Applied.IsValid())
				{
					ASC->RemoveActiveGameplayEffect(Applied);
				}
				return;
			}
			ParryWindow = Applied;
		}
	}
}

void UAgGameplayAbility::SetGuarding(bool bGuarding)
{
	if (bGuarding && !GuardingTag)
	{
		UAgAbilityTask_HoldTag* Task = HoldConfiguredTag(TEXT("Guarding"));
		if (IsActive() && !bEnding && Task && !Task->IsFinished())
		{
			GuardingTag = Task;
		}
	}
	else if (!bGuarding && GuardingTag)
	{
		UAgAbilityTask_HoldTag* Task = GuardingTag.Get();
		GuardingTag = nullptr;
		Task->EndTask();
	}
}

void UAgGameplayAbility::CloseParryWindow()
{
	const FActiveGameplayEffectHandle Previous = ParryWindow;
	ParryWindow.Invalidate();
	if (Previous.IsValid())
	{
		GetAbilitySystemComponentFromActorInfo()->RemoveActiveGameplayEffect(Previous);
	}
}

void UAgGameplayAbility::HandleEvent(FGameplayEventData Event)
{
	if (!IsActive() || bEnding)
	{
		return;
	}
	const FName Id = GetAbilityId();
	if (Event.EventTag == AgGameplayTags::Input_Attack && Id == TEXT("BasicAttack"))
	{
		if (!BusyTag && !ComboMontageIds.IsEmpty())
		{
			StartComboHit((ComboIndex + 1) % ComboMontageIds.Num());
		}
	}
	else if (Event.EventTag == AgGameplayTags::Input_LockOn && Id == TEXT("LockOn"))
	{
		Finish();
	}
	else if (Event.EventTag == AgGameplayTags::Input_Guard && Id == TEXT("Guard") && Motion.MontageId == TEXT("Parry"))
	{
		SetGuarding(true);
		if (!IsActive() || bEnding)
		{
			return;
		}
		if (!BusyTag)
		{
			ClearMotion(true);
			EndBusy();
			StartGuard(true);
		}
	}
	else if (Event.EventTag == AgGameplayTags::Input_GuardReleased && Id == TEXT("Guard"))
	{
		if (Motion.MontageId == TEXT("Parry"))
		{
			SetGuarding(false);
		}
		else
		{
			Finish();
		}
	}
	else if (Event.EventTag == AgGameplayTags::Event_Parry && Id == TEXT("Guard") && Motion.MontageId != TEXT("Parry"))
	{
		ApplyReward(TEXT("Parry"));
		if (!IsActive())
		{
			return;
		}
		if (const AAgCharacterBase* Attacker = Cast<AAgCharacterBase>(Event.ContextHandle.GetInstigator()))
		{
			const FGameplayEffectSpecHandle Spec = MakeEffectSpec(EAgEffect::ParryPPLoss);
			if (Spec.IsValid())
			{
				GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(
				    *Spec.Data, Attacker->GetAbilitySystemComponent());
			}
		}
		if (!IsActive())
		{
			return;
		}
		FaceActor(Event.Instigator.Get());
		CloseParryWindow();
		PlayMotion(TEXT("Parry"), true);
	}
	else if (Event.EventTag == AgGameplayTags::Event_PerfectDodge && Id == TEXT("Dodge") && !bPerfectDodged)
	{
		bPerfectDodged = true;
		const FAgTimedTagParams Chance = ExecutionDefinition.Tasks.TimedTag.FindChecked(TEXT("CounterChance"));
		const FAgSlowMotionParams Slow = ExecutionDefinition.Tasks.SlowMotion.FindChecked(TEXT("PerfectDodge"));
		ApplyReward(TEXT("PerfectDodge"));
		if (!IsActive())
		{
			return;
		}
		const FGameplayEffectSpecHandle RefundSpec = MakeEffectSpec(EAgEffect::Resource);
		if (RefundSpec.IsValid())
		{
			// 이 회피에 쓴 SP를 돌려받습니다.
			FAgGrantResourcesParams Refund;
			Refund.SP = SPSpent;
			Refund.AddToSpec(*RefundSpec.Data);
			ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, RefundSpec);
		}
		if (!IsActive())
		{
			return;
		}
		ApplyTimedTag(Chance);
		if (!IsActive())
		{
			return;
		}
		GetWorld()->GetSubsystem<UAgTimeSubsystem>()->StartSlowMotion(Slow.Speed, Slow.RealDuration);
	}
	else if (Event.EventTag == AgGameplayTags::Event_AttackBlocked && Id == TEXT("DashThrust") && DashTask)
	{
		DashTask->Finish();
	}
	else if (Event.EventTag == AgGameplayTags::Event_Executed && Id == TEXT("Groggy"))
	{
		HoldConfiguredTag(TEXT("Executed"));
		if (!IsActive())
		{
			return;
		}
		const FAgCollisionResponseParams& Collision =
		    ExecutionDefinition.Tasks.SetCollisionResponseToChannel.FindChecked(TEXT("PassThroughPawn"));
		UAgAbilityTask_SetCollisionResponseToChannel::SetCollisionResponseToChannel(
		    this, GetAgCharacter()->GetCapsuleComponent(), Collision.Channel, Collision.Response)
		    ->ReadyForActivation();
		if (!IsActive())
		{
			return;
		}
		FaceActor(Event.Instigator.Get());
		if (GroggyWait)
		{
			GroggyWait->EndTask();
			GroggyWait = nullptr;
		}
		PlayMotion(TEXT("Executed"));
	}
}

void UAgGameplayAbility::HandleMontageEvent(int32 InstanceId, FGameplayTag Event)
{
	if (!OwnsMontage(InstanceId) || bEnding)
	{
		return;
	}
	if (Event == AgGameplayTags::Event_RecoveryStarted)
	{
		EndBusy();
		if (GetAbilityId() == TEXT("Guard") && Motion.MontageId == TEXT("Parry") && GuardingTag)
		{
			ClearMotion(true);
			StartGuard(false);
		}
	}
	else if (Event == AgGameplayTags::Event_DownGrounded)
	{
		if (!DownedTag)
		{
			DownedTag = HoldConfiguredTag(TEXT("Downed"));
		}
	}
	else if (Event == AgGameplayTags::Event_JumpAttackPlunge && ExecutionDefinition.Tasks.bHasLaunchVertical)
	{
		if (GetAgCharacter()->GetCharacterMovement()->IsFalling())
		{
			GetAgCharacter()->LaunchCharacter(FVector(0.f, 0.f, ExecutionDefinition.Tasks.LaunchVertical.VelocityZ), false, true);
		}
	}
	else if (Event == AgGameplayTags::Event_Boss_DashHold && DashTask && !DashTask->IsFinished())
	{
		bHoldingDash = true;
		GetActorInfo().GetAnimInstance()->Montage_Pause(Motion.Montage);
	}
	else if (Event == AgGameplayTags::Event_ExecutionBlow && GetAbilityId() == TEXT("Execution"))
	{
		if (ExecutionStrike < 2 && ExecutionTarget.IsValid())
		{
			const bool bLast = ExecutionStrike++ == 1;
			ApplySingleAttack(bLast ? TEXT("Finish") : TEXT("Stab"), {ExecutionTarget.Get()}, bLast);
		}
	}
	else if (Event == AgGameplayTags::Event_UltimateFinish && GetAbilityId() == TEXT("Ultimate"))
	{
		K2_RemoveGameplayCue(AgGameplayTags::GameplayCue_UltimateSlashes);
		UAgAbilityTask_TimeStop* Stop = UltimateTimeStop.Get();
		UltimateTimeStop = nullptr;
		if (Stop)
		{
			Stop->EndTask();
		}
		if (!IsActive())
		{
			return;
		}
		const FAgTargetQueryParams& Query = ExecutionDefinition.Tasks.TargetQuery.FindChecked(TEXT("Finish"));
		ApplySingleAttack(TEXT("Finish"), CastChecked<AAgPlayerCharacter>(GetAgCharacter())->FindEnemies(Query.Range), true);
	}
}

void UAgGameplayAbility::HandleCutsceneFinished()
{
	if (!IsActive())
	{
		return;
	}
	JumpToMotionSection(TEXT("Judgment"));
	K2_AddGameplayCue(AgGameplayTags::GameplayCue_UltimateSlashes, FGameplayEffectContextHandle(), true);
}

void UAgGameplayAbility::HandleTargetLost()
{
	Finish();
}

void UAgGameplayAbility::HandleMotionCompleted()
{
	if (!IsActive() || bEnding)
	{
		return;
	}
	const FName Id = GetAbilityId();
	if (Id == TEXT("Jump"))
	{
		ClearMotion(false);
		return;
	}
	if (Id == TEXT("KnockUp") && Motion.MontageId != TEXT("GetUp"))
	{
		return;
	}
	if (Id == TEXT("Guard"))
	{
		ClearMotion(false);
		EndBusy();
		if (GuardingTag)
		{
			StartGuard(false);
		}
		else
		{
			Finish();
		}
		return;
	}
	if (Id == TEXT("Groggy") || Id == TEXT("PhaseTransition"))
	{
		ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, MakeEffectSpec(EAgEffect::PPRefill));
		if (Id == TEXT("PhaseTransition") && IsActive())
		{
			CastChecked<AAgBossCharacter>(GetAgCharacter())->EnterPhase2();
		}
	}
	if (Id == TEXT("Death"))
	{
		GetAgCharacter()->StartRagdoll(FVector::ZeroVector);
	}
	Finish();
}

void UAgGameplayAbility::HandleMotionCancelled()
{
	Finish(true);
}

void UAgGameplayAbility::HandleMotionBlendOut()
{
	if (!IsActive() || bEnding)
	{
		return;
	}
	if (GetAbilityId() == TEXT("KnockUp") && Motion.MontageId == TEXT("KnockUpFall") && !bGettingUp)
	{
		bGettingUp = true;
		PlayMotion(TEXT("GetUp"));
	}
	if (GetAbilityId() == TEXT("Death"))
	{
		GetAgCharacter()->StartRagdoll(FVector::ZeroVector);
		Finish();
	}
}

void UAgGameplayAbility::StartGroggy()
{
	if (MotionTask)
	{
		return;
	}
	UAnimMontage* Montage = ExecutionMontages.FindRef(TEXT("Groggy"));
	if (!Montage)
	{
		Finish(true);
		return;
	}
	const int32 EndSection = Montage->GetSectionIndex(TEXT("End"));
	const float EndLength = EndSection == INDEX_NONE ? 0.f : Montage->GetSectionLength(EndSection);
	if (!PlayMotion(TEXT("Groggy")))
	{
		return;
	}
	CastChecked<AAgBossCharacter>(GetAgCharacter())->SetGroggyTimerRunning(true);
	GroggyWait =
	    UAbilityTask_WaitDelay::WaitDelay(this, FMath::Max(0.f, UAgAssetManager::Get().GetCombatRules().GroggyDuration - EndLength));
	GroggyWait->OnFinish.AddDynamic(this, &ThisClass::HandleGroggyEndDue);
	GroggyWait->ReadyForActivation();
}

void UAgGameplayAbility::HandleGroggyEndDue()
{
	GroggyWait = nullptr;
	if (IsActive())
	{
		JumpToMotionSection(TEXT("End"));
	}
}

void UAgGameplayAbility::StartKnockUp()
{
	const UAgCombatRules& Rules = UAgAssetManager::Get().GetCombatRules();
	if (!PlayMotion(TEXT("KnockUp")))
	{
		return;
	}
	AAgCharacterBase* Character = GetAgCharacter();
	UAbilityTask_ApplyRootMotionJumpForce* Jump = UAbilityTask_ApplyRootMotionJumpForce::ApplyRootMotionJumpForce(this, NAME_None,
	    Character->GetActorRotation(), 0.f, Rules.KnockUpHeight, Rules.KnockUpAirTime, 0.5f, true,
	    ERootMotionFinishVelocityMode::SetVelocity, FVector::ZeroVector, 0.f, nullptr, nullptr);
	Jump->ReadyForActivation();
	const UAnimMontage* FallMontage = ExecutionMontages.FindRef(TEXT("KnockUpFall"));
	FallBlendSpeed = 8.f * Rules.KnockUpHeight / FMath::Square(Rules.KnockUpAirTime) * FallMontage->BlendIn.GetBlendTime() * 0.5f;
	UAbilityTask_WaitVelocityChange* Rise =
	    UAbilityTask_WaitVelocityChange::CreateWaitVelocityChange(this, FVector::UpVector, FallBlendSpeed);
	Rise->OnVelocityChage.AddDynamic(this, &ThisClass::HandleRising);
	Rise->ReadyForActivation();
	WaitForLanding();
}

void UAgGameplayAbility::HandleRising()
{
	if (!IsActive() || Motion.MontageId != TEXT("KnockUp"))
	{
		return;
	}
	UAbilityTask_WaitVelocityChange* Fall =
	    UAbilityTask_WaitVelocityChange::CreateWaitVelocityChange(this, FVector::DownVector, -FallBlendSpeed);
	Fall->OnVelocityChage.AddDynamic(this, &ThisClass::HandleFalling);
	Fall->ReadyForActivation();
}

void UAgGameplayAbility::HandleFalling()
{
	if (IsActive() && Motion.MontageId == TEXT("KnockUp"))
	{
		PlayMotion(TEXT("KnockUpFall"));
	}
}

void UAgGameplayAbility::HandleLanded(EMovementMode Mode)
{
	if (!IsActive() || bEnding)
	{
		return;
	}
	const FName Id = GetAbilityId();
	if (Id == TEXT("Groggy"))
	{
		StartGroggy();
	}
	else if (Id == TEXT("JumpAttack"))
	{
		JumpToMotionSection(TEXT("Land"));
	}
	else if (Id == TEXT("KnockUp"))
	{
		if (Motion.MontageId == TEXT("KnockUpFall"))
		{
			JumpToMotionSection(TEXT("Land"));
		}
		else
		{
			PlayMotion(TEXT("KnockUpFall"), false, 1.f, TEXT("Land"));
		}
	}
	else if (Id == TEXT("Jump"))
	{
		const AAgPlayerCharacter* Player = CastChecked<AAgPlayerCharacter>(GetAgCharacter());
		UAnimMontage* Land = ExecutionMontages.FindRef(TEXT("Land"));
		UAnimInstance* Anim = GetActorInfo().GetAnimInstance();
		ClearMotion(true);
		if (Land && Anim && Player->GetMoveInputDirection().IsNearlyZero())
		{
			Anim->Montage_Play(Land);
		}
		Finish();
	}
	else if (Id == TEXT("LeapSlam"))
	{
		JumpToMotionSection(TEXT("End"));
		AAgBossCharacter* Boss = CastChecked<AAgBossCharacter>(GetAgCharacter());
		AAgPlayerController* Controller = Cast<AAgPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
		const UAgCameraData& Camera = UAgAssetManager::Get().GetCameraData();
		if (Controller && Controller->GetPawn() && Boss->GetHorizontalDistanceTo(Controller->GetPawn()) <= Camera.LeapLandingShakeRange)
		{
			Controller->PlayCameraShake(Camera.LeapLandingShake);
		}
		const FAgTargetQueryParams& Query = ExecutionDefinition.Tasks.TargetQuery.FindChecked(TEXT("Landing"));
		AAgCharacterBase* Target = Boss->GetTarget();
		if (Target && !Target->GetCharacterMovement()->IsFalling() && Boss->GetHorizontalDistanceTo(Target) <= Query.Range)
		{
			ApplySingleAttack(TEXT("LandingHit"), {Target}, false, Boss->GetActorLocation());
		}
	}
}

void UAgGameplayAbility::StartDeath(const FGameplayEventData* Event)
{
	AAgCharacterBase* Character = GetAgCharacter();
	UAnimInstance* Anim = GetActorInfo().GetAnimInstance();
	const bool bExecuted = Cast<AAgBossCharacter>(Character) && Event &&
	                       Event->InstigatorTags.HasTagExact(AgGameplayTags::Ability_Action_Execution) &&
	                       GetAbilitySystemComponentFromActorInfo()->HasMatchingGameplayTag(AgGameplayTags::State_Execution);
	UAnimMontage* Executed = bExecuted ? ExecutionMontages.FindRef(TEXT("Executed")) : nullptr;
	if (Executed && Anim && ExecutionMontages.Contains(TEXT("ExecutedDeath")))
	{
		const float Position = Anim->Montage_GetPosition(Executed);
		Character->Die(FVector::ZeroVector, this, false);
		if (IsActive())
		{
			PlayMotion(TEXT("ExecutedDeath"), false, 1.f, NAME_None, 1.f, Position);
		}
		return;
	}
	const AActor* Attacker = Event ? Event->Instigator.Get() : nullptr;
	const FVector Push =
	    Attacker ? (Character->GetActorLocation() - Attacker->GetActorLocation()).GetSafeNormal2D() * 300.f : FVector::ZeroVector;
	Character->Die(Push, this);
	Finish();
}
