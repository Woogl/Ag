// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgDamageResultGameplayEffectComponent.h"
#include "AbilitySystem/AgGameplayEffectContext.h"
#include "Camera/AgCameraShake.h"

#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/AgGameplayAbility.h"
#include "AbilitySystem/AgSlowMotionGameplayEffectComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Ag.h"
#include "Character/AgCharacterBase.h"
#include "Character/AgHitPhysicsComponent.h"
#include "Character/AgPlayerCharacter.h"
#include "Combat/AgProjectile.h"
#include "Core/AgAssetManager.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgAbilityData.h"
#include "Data/AgCharacterData.h"
#include "Data/AgCombatRules.h"
#include "Data/AgEffectData.h"
#include "GameplayEffect.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const FName GuardFlinchMontageId(TEXT("GuardFlinch"));

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

	FAgPhysicsHit MakePhysicsHit(const AActor& Causer, const AAgCharacterBase& Target, const FGameplayEffectContextHandle& Context, bool bGuard)
	{
		FAgPhysicsHit Hit;
		Hit.bGuard = bGuard;
		Hit.Location = Context.GetOrigin();
		// WeaponHit은 HitResult의 TraceStart·TraceEnd에 날의 맞은 지점이 지나온 선을 담습니다.
		const FHitResult* Contact = Context.GetHitResult();
		const FVector BladeMotion = Contact ? Contact->TraceEnd - Contact->TraceStart : FVector::ZeroVector;
		Hit.Direction = BladeMotion.IsNearlyZero() ? (Target.GetActorLocation() - Causer.GetActorLocation()).GetSafeNormal2D() : BladeMotion.GetSafeNormal();
		Hit.Bone = Contact ? Contact->BoneName : NAME_None;
		return Hit;
	}

	void StartHitReaction(AActor* Instigator, AAgCharacterBase* Target, const FGameplayTagContainer& SpecTags, const FGameplayEffectContextHandle& Context)
	{
		const UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
		const FGameplayTag Event = SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Reaction_Flinch) ? AgGameplayTags::Event_Hit_Flinch
		                                                                                               : GetReactionEvent(SpecTags);
		// 처형은 두 캐릭터의 모션을 프레임 단위로 맞물리므로 흔들지도 않습니다.
		if (!Event.IsValid() || TargetASC->HasMatchingGameplayTag(AgGameplayTags::State_Execution))
		{
			return;
		}
		if (Event.IsValid() && !TargetASC->HasMatchingGameplayTag(AgGameplayTags::State_Groggy))
		{
			SendEvent(Target, Event, Instigator);
		}
		Target->GetHitPhysics()->Play(MakePhysicsHit(*Instigator, *Target, Context, false));
	}

	void StartGuardReaction(AActor* Instigator, AAgCharacterBase* Target, const FGameplayTagContainer& SpecTags, const FGameplayEffectContextHandle& Context)
	{
		const bool bPushback = GetReactionEvent(SpecTags).IsValid();
		if (!bPushback && !SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Reaction_Flinch))
		{
			return;
		}
		if (bPushback)
		{
			SendEvent(Target, AgGameplayTags::Event_Guard_Pushback, Instigator);
		}
		else
		{
			Target->PlayAnimMontage(Target->GetCharacterData()->FindMontage(GuardFlinchMontageId));
		}
		Target->GetHitPhysics()->Play(MakePhysicsHit(*Instigator, *Target, Context, true));
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
		// 화면 효과의 길이는 함께 거는 효과 중 슬로우모션의 길이를 따릅니다.
		FGameplayCueParameters ScreenParameters;
		if (const UAgPlayerData* Data = Cast<UAgPlayerData>(Dodger.GetCharacterData()))
		{
			for (FName Id : Data->PerfectDodgeEffects)
			{
				const UGameplayEffect* Effect = UAgAssetManager::Get().FindEffect(Id);
				ApplyToSelf(*ASC, Effect);
				if (const UAgSlowMotionGameplayEffectComponent* SlowMotion = Effect ? Effect->FindComponent<UAgSlowMotionGameplayEffectComponent>() : nullptr)
				{
					ScreenParameters.RawMagnitude = SlowMotion->Duration;
				}
			}
		}
		ASC->ExecuteGameplayCue(AgGameplayTags::GameplayCue_PerfectDodgeScreen, ScreenParameters);
	}

	void ApplyParryAttackerEffects(AAgCharacterBase& Defender, AAgCharacterBase& Attacker)
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
	}

	void ApplyParry(AAgCharacterBase& Defender, AAgCharacterBase& Attacker, bool bProjectile)
	{
		if (!bProjectile)
		{
			ApplyParryAttackerEffects(Defender, Attacker);
		}
		Defender.GetAbilitySystemComponent()->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(AgGameplayTags::State_ParryWindow));
	}

} // namespace

void UAgDamageResultGameplayEffectComponent::OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
	UAbilitySystemComponent* TargetASC = ActiveGEContainer.Owner;
	AAgCharacterBase* Target = CastChecked<AAgCharacterBase>(TargetASC->GetAvatarActor());
	const FGameplayEffectContextHandle Context = GESpec.GetContext();
	AAgCharacterBase* Attacker = CastChecked<AAgCharacterBase>(Context.GetInstigator());
	AActor* Causer = Context.GetEffectCauser();
	AAgProjectile* Projectile = Cast<AAgProjectile>(Causer);

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
	// 반사탄의 PP 효과도 피격 반응 전에 적용해야 그로기가 넉백보다 우선합니다.
	if (bHit && Projectile && Projectile->IsReflected() && !Target->IsDead())
	{
		ApplyParryAttackerEffects(*Attacker, *Target);
	}
	const UAgAttributeSet* TargetStats = Target->GetAttributeSet();
	UE_LOG(LogAg, Verbose, TEXT("Hit: %s -> %s, parry %d, guard %d, guard break %d, HP now %.0f, PP now %.0f, SP now %.0f"),
		*Attacker->GetName(), *Target->GetName(), bParry, bGuard, bGuardBreak, TargetStats->GetHP(), TargetStats->GetPP(),
		TargetStats->GetSP());

	// 사망·그로기 판정(어트리뷰트 세트)이 피격 반응보다 먼저입니다. 이번 타격으로 죽었으면 반응하지 않고, 그로기가
	// 됐으면 그로기 태그가 이미 붙어 흔들기만 합니다.
	if (!Target->IsDead())
	{
		if (bParry)
		{
			ApplyParry(*Target, *Attacker, Projectile != nullptr);
			SendEvent(Target, AgGameplayTags::Event_Parry, Causer, Context);
		}
		else if (bGuard)
		{
			StartGuardReaction(Causer, Target, SpecTags, Context);
		}
		else if (bGuardBreak)
		{
			SendEvent(Target, AgGameplayTags::Event_Guard_Break, Causer);
		}
		else
		{
			StartHitReaction(Causer, Target, SpecTags, Context);
		}
	}

	if (!bHit)
	{
		SendEvent(Attacker, AgGameplayTags::Event_AttackBlocked, Target, Context);
	}

	FGameplayCueParameters CueParameters;
	CueParameters.Location = Context.GetOrigin();
	CueParameters.Instigator = Attacker;
	CueParameters.EffectCauser = Causer;
	const FGameplayTag ResultCue = bParry                    ? AgGameplayTags::GameplayCue_Parry
	                               : (bGuard || bGuardBreak) ? AgGameplayTags::GameplayCue_Guard
	                                                         : AgGameplayTags::GameplayCue_Hit;
	TargetASC->ExecuteGameplayCue(ResultCue, CueParameters);
	if (AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(Target, 0)))
	{
		AgCameraShake::Play(Player, Player->GetCombatCameraShake(SpecTags, Attacker->IsA<AAgPlayerCharacter>()));
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

	const TSharedPtr<FAgHitRewardState> Reward = FAgGameplayEffectContext::GetHitReward(Context);
	if (bHit && Reward && !Reward->bGranted)
	{
		UAgAbilitySystemComponent* AttackerASC = CastChecked<UAgAbilitySystemComponent>(Attacker->GetAbilitySystemComponent());
		const FGameplayEffectSpecHandle ChargeSpec = AttackerASC->MakeEffectSpec(EAgEffect::Resource, 1.f, FGameplayEffectContextHandle());
		if (ChargeSpec.IsValid())
		{
			// 자원 변경 콜백이 다시 적중을 일으킬 수 있으므로 적용 전에 지급 기회를 사용합니다.
			Reward->bGranted = true;
			AgResource::SetAmounts(*ChargeSpec.Data, 0.f, Reward->Amounts.MP, Reward->Amounts.UP);
			AttackerASC->ApplyGameplayEffectSpecToSelf(*ChargeSpec.Data);
		}
	}
	if (bParry && IsValid(Projectile) && !Target->IsDead())
	{
		Projectile->Reflect(*Target);
	}
}
