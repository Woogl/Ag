// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgDamageResultGameplayEffectComponent.h"

#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Ag.h"
#include "Character/AgCharacterBase.h"
#include "Character/AgPlayerCharacter.h"
#include "Combat/AgTimeSubsystem.h"
#include "Core/AgAssetManager.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgAbilityData.h"
#include "Data/AgCameraData.h"
#include "Data/AgCharacterData.h"
#include "Data/AgCombatRules.h"
#include "Data/AgEffectData.h"
#include "GameplayEffect.h"
#include "Game/AgPlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	void SendEvent(AActor* Target, const FGameplayTag& EventTag, AActor* Instigator, const FGameplayEffectContextHandle& Context = FGameplayEffectContextHandle())
	{
		FGameplayEventData Payload;
		Payload.EventTag = EventTag;
		Payload.Instigator = Instigator;
		Payload.Target = Target;
		Payload.ContextHandle = Context;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Target, EventTag, Payload);
	}

	FGameplayTag GetReactionEvent(const FGameplayTagContainer& SpecTags)
	{
		if (SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Reaction_KnockUp))
		{
			return AgGameplayTags::Event_Hit_KnockUp;
		}
		if (SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Reaction_KnockBack))
		{
			return AgGameplayTags::Event_Hit_KnockBack;
		}
		return FGameplayTag();
	}

	void StartHitReaction(AActor* Instigator, AAgCharacterBase* Target, const FGameplayTagContainer& SpecTags)
	{
		const UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
		const FGameplayTag Event = GetReactionEvent(SpecTags);
		if (Event.IsValid() && !TargetASC->HasMatchingGameplayTag(AgGameplayTags::State_Groggy) &&
			!TargetASC->HasMatchingGameplayTag(AgGameplayTags::State_Execution))
		{
			SendEvent(Target, Event, Instigator);
		}
		else if ((Event.IsValid() || SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Reaction_Flinch)) &&
				 !TargetASC->HasMatchingGameplayTag(AgGameplayTags::State_HitReaction))
		{
			Target->PlayFlinch();
		}
	}

	void StartGuardReaction(AActor* Instigator, AAgCharacterBase* Target, const FGameplayTagContainer& SpecTags)
	{
		if (GetReactionEvent(SpecTags).IsValid())
		{
			SendEvent(Target, AgGameplayTags::Event_Guard_Pushback, Instigator);
		}
		else if (SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Reaction_Flinch))
		{
			Target->PlayGuardFlinch();
		}
	}

	void ApplyToSelf(UAbilitySystemComponent& ASC, const UGameplayEffect* Effect)
	{
		ASC.ApplyGameplayEffectToSelf(Effect, 1.f, ASC.MakeEffectContext());
	}

	void ApplyHitstop(UAgAbilitySystemComponent& ASC, float Duration)
	{
		const FGameplayEffectSpecHandle Spec = ASC.MakeEffectSpec(EAgEffect::Hitstop, 1.f, ASC.MakeEffectContext());
		Spec.Data->SetDuration(Duration, true);
		ASC.ApplyGameplayEffectSpecToSelf(*Spec.Data);
	}

	void StartPerfectDodge(AAgCharacterBase& Dodger)
	{
		UAgAbilitySystemComponent* ASC = CastChecked<UAgAbilitySystemComponent>(Dodger.GetAbilitySystemComponent());
		const FGameplayTagContainer WindowTag(AgGameplayTags::State_PerfectDodgeWindow);
		const TArray<FActiveGameplayEffectHandle> Windows = ASC->GetActiveEffects(FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(WindowTag));
		if (Windows.IsEmpty())
		{
			return;
		}
		const FActiveGameplayEffect* Window = ASC->GetActiveGameplayEffect(Windows[0]);
		const UAgGameplayAbility* Dodge =
			Window ? Cast<UAgGameplayAbility>(Window->Spec.GetContext().GetAbilityInstance_NotReplicated()) : nullptr;
		const FAgResourceCost Refund = Dodge ? Dodge->GetCost() : FAgResourceCost();
		ASC->RemoveActiveEffectsWithGrantedTags(WindowTag);
		if (!Refund.IsFree())
		{
			const FGameplayEffectSpecHandle Spec = ASC->MakeEffectSpec(EAgEffect::Resource, 1.f, FGameplayEffectContextHandle());
			if (Spec.IsValid())
			{
				AgResource::SetAmounts(*Spec.Data, Refund.SP, Refund.MP, Refund.UP);
				ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
			}
		}
		if (const UAgPlayerData* Data = Cast<UAgPlayerData>(Dodger.GetCharacterData()))
		{
			for (FName Effect : Data->PerfectDodgeEffects)
			{
				ApplyToSelf(*ASC, UAgAssetManager::Get().FindEffect(Effect));
			}
			Dodger.GetWorld()->GetSubsystem<UAgTimeSubsystem>()->StartSlowMotion(Data->PerfectDodgeSlowMotionSpeed,
				Data->PerfectDodgeSlowMotionDuration);
		}
	}

	void ApplyParry(AAgCharacterBase& Defender, AAgCharacterBase& Attacker)
	{
		UAgAbilitySystemComponent* ASC = CastChecked<UAgAbilitySystemComponent>(Defender.GetAbilitySystemComponent());
		const UAgPlayerData& Data = *CastChecked<UAgPlayerData>(Defender.GetCharacterData());
		for (FName Id : Data.ParryAttackerEffects)
		{
			if (const UGameplayEffect* Effect = UAgAssetManager::Get().FindEffect(Id))
			{
				const FGameplayEffectSpecHandle Spec = ASC->MakeEffectSpec(*Effect, 1.f, ASC->MakeEffectContext());
				ASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, Attacker.GetAbilitySystemComponent());
			}
		}
		ASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(AgGameplayTags::State_ParryWindow));
		for (FName Effect : Data.ParryEffects)
		{
			ApplyToSelf(*ASC, UAgAssetManager::Get().FindEffect(Effect));
		}
	}

	EAgCameraShake GetHitShake(const UAgCameraData& Camera, const FGameplayTagContainer& SpecTags, bool bPlayerAttacked)
	{
		const bool bFinisher = SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Finisher);
		if (SpecTags.HasTagExact(AgGameplayTags::Ability_Action_Execution))
		{
			return bFinisher ? Camera.ExecutionBlowShake : Camera.ExecutionStabShake;
		}

		const bool bHeavy = GetReactionEvent(SpecTags).IsValid();
		if (SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Result_Hit))
		{
			if (bPlayerAttacked)
			{
				return bFinisher ? Camera.FinisherShake : (bHeavy ? Camera.PlayerHitHeavyShake : Camera.PlayerHitShake);
			}
			return bHeavy ? Camera.PlayerHurtHeavyShake : Camera.PlayerHurtShake;
		}
		if (SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Result_Parry))
		{
			return Camera.ParryShake;
		}
		if (SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Result_Guard))
		{
			return bHeavy ? Camera.GuardPushbackShake : Camera.GuardShake;
		}
		return Camera.GuardBreakShake;
	}
} // namespace

void UAgDamageResultGameplayEffectComponent::OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
	UAbilitySystemComponent* TargetASC = ActiveGEContainer.Owner;
	AAgCharacterBase* Target = CastChecked<AAgCharacterBase>(TargetASC->GetAvatarActor());
	const FGameplayEffectContextHandle Context = GESpec.GetContext();
	AAgCharacterBase* Attacker = CastChecked<AAgCharacterBase>(Context.GetInstigator());
	AActor* Causer = Context.GetEffectCauser();

	// 공격의 성질(적용하는 쪽), 피격 결과(실행 계산), 공격한 어빌리티의 태그(엔진)가 모두 스펙 태그에 있습니다.
	const FGameplayTagContainer& SpecTags = std::as_const(GESpec).CapturedSourceTags.GetSpecTags();
	if (SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Result_PerfectDodge))
	{
		StartPerfectDodge(*Target);
		return;
	}

	const bool bParry = SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Result_Parry);
	const bool bGuard = SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Result_Guard);
	const bool bGuardBreak = SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Result_GuardBreak);
	const bool bHit = SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Result_Hit);
	if (!bParry && !bGuard && !bGuardBreak && !bHit)
	{
		return;
	}
	const UAgAttributeSet* TargetStats = Target->GetAttributeSet();
	UE_LOG(LogAg, Verbose, TEXT("Hit: %s -> %s, parry %d, guard %d, guard break %d, HP now %.0f, PP now %.0f, SP now %.0f"),
		*Attacker->GetName(), *Target->GetName(), bParry, bGuard, bGuardBreak, TargetStats->GetHP(), TargetStats->GetPP(),
		TargetStats->GetSP());

	// 사망·그로기 판정(어트리뷰트 세트)이 피격 반응보다 먼저이므로, 이번 타격으로 죽었거나 그로기가 됐으면
	// 반응하지 않습니다.
	const bool bBecameGroggy = !GESpec.CapturedTargetTags.GetActorTags().HasTag(AgGameplayTags::State_Groggy) &&
	                           TargetASC->HasMatchingGameplayTag(AgGameplayTags::State_Groggy);
	if (!Target->IsDead() && !bBecameGroggy)
	{
		if (bParry)
		{
			ApplyParry(*Target, *Attacker);
			SendEvent(Target, AgGameplayTags::Event_Parry, Causer, Context);
		}
		else if (bGuard)
		{
			StartGuardReaction(Causer, Target, SpecTags);
		}
		else if (bGuardBreak)
		{
			SendEvent(Target, AgGameplayTags::Event_Guard_Break, Causer);
		}
		else
		{
			StartHitReaction(Causer, Target, SpecTags);
		}
	}

	if (!bHit)
	{
		SendEvent(Attacker, AgGameplayTags::Event_AttackBlocked, Target);
	}

	FGameplayCueParameters CueParameters;
	CueParameters.Location = Context.GetOrigin();
	CueParameters.Instigator = Attacker;
	CueParameters.EffectCauser = Causer;
	const FGameplayTag ResultCue = bParry                    ? AgGameplayTags::GameplayCue_Parry
	                               : (bGuard || bGuardBreak) ? AgGameplayTags::GameplayCue_Guard
	                                                         : AgGameplayTags::GameplayCue_Hit;
	TargetASC->ExecuteGameplayCue(ResultCue, CueParameters);
	const EAgCameraShake Shake = GetHitShake(UAgAssetManager::Get().GetCameraData(), SpecTags, Attacker->IsA<AAgPlayerCharacter>());
	if (AAgPlayerController* PlayerController = Cast<AAgPlayerController>(UGameplayStatics::GetPlayerController(Target, 0)))
	{
		PlayerController->PlayCameraShake(Shake);
	}
	if (const FGameplayEffectModifiedAttribute* HPChange = std::as_const(GESpec).GetModifiedAttribute(UAgAttributeSet::GetHPAttribute());
		HPChange && HPChange->TotalMagnitude < 0.f)
	{
		FGameplayCueParameters NumberParameters = CueParameters;
		NumberParameters.RawMagnitude = -HPChange->TotalMagnitude;
		if (SpecTags.HasTagExact(AgGameplayTags::Ability_Action_Execution))
		{
			NumberParameters.Location = Target->GetActorLocation();
		}
		TargetASC->ExecuteGameplayCue(AgGameplayTags::GameplayCue_DamageFloater, NumberParameters);
	}

	if (!SpecTags.HasTagExact(AgGameplayTags::Ability_Action_Execution))
	{
		const float HitstopDuration = UAgAssetManager::Get().GetCombatRules().HitstopDuration;
		ApplyHitstop(*CastChecked<UAgAbilitySystemComponent>(TargetASC), HitstopDuration);
		if (Causer == Attacker)
		{
			ApplyHitstop(*CastChecked<UAgAbilitySystemComponent>(Attacker->GetAbilitySystemComponent()), HitstopDuration);
		}
	}

	if (bHit && SpecTags.HasTagExact(AgGameplayTags::Ability_Action_Ultimate))
	{
		const UAgPlayerData* Data = CastChecked<UAgPlayerData>(Attacker->GetCharacterData());
		Target->GetWorld()->GetSubsystem<UAgTimeSubsystem>()->StartSlowMotion(Data->UltimateSlowMotionSpeed,
			Data->UltimateSlowMotionDuration);
	}

	const float MPCharge = GESpec.GetSetByCallerMagnitude(AgGameplayTags::SetByCaller_MP);
	const float UPCharge = GESpec.GetSetByCallerMagnitude(AgGameplayTags::SetByCaller_UP);
	if (bHit && (MPCharge > 0.f || UPCharge > 0.f))
	{
		UAgAbilitySystemComponent* AttackerASC = CastChecked<UAgAbilitySystemComponent>(Attacker->GetAbilitySystemComponent());
		const FGameplayEffectSpecHandle ChargeSpec = AttackerASC->MakeEffectSpec(EAgEffect::Resource, 1.f, FGameplayEffectContextHandle());
		if (ChargeSpec.IsValid())
		{
			AgResource::SetAmounts(*ChargeSpec.Data, 0.f, MPCharge, UPCharge);
			AttackerASC->ApplyGameplayEffectSpecToSelf(*ChargeSpec.Data);
		}
	}
}
