// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgDamageResultGameplayEffectComponent.h"
#include "AbilitySystem/AgGameplayEffectContext.h"

#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Ag.h"
#include "Character/AgCharacterBase.h"
#include "Character/AgHitPhysicsComponent.h"
#include "Combat/AgProjectile.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/AgAssetManager.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgAttackData.h"
#include "Data/AgCharacterData.h"
#include "Data/AgCombatRules.h"
#include "Data/AgEffectData.h"
#include "GameplayEffect.h"
#include "KismetAnimationLibrary.h"
#include "Perception/AISense_Damage.h"

namespace AgDamageResultGameplayEffectComponentPrivate
{
	const FName GuardCounterChanceEffectId(TEXT("GuardCounterChance"));
	const FName MoveLockEffectId(TEXT("MoveLock"));

	void SendEvent(AActor* Target, const FGameplayTag& EventTag, AActor* Instigator, const FGameplayEffectContextHandle& Context = FGameplayEffectContextHandle(), const FGameplayTagContainer& InstigatorTags = FGameplayTagContainer::EmptyContainer)
	{
		FGameplayEventData Payload;
		Payload.EventTag = EventTag;
		Payload.Instigator = Instigator;
		Payload.Target = Target;
		Payload.ContextHandle = Context;
		Payload.InstigatorTags = InstigatorTags;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Target, EventTag, Payload);
	}

	FGameplayTag GetKnockReactionEvent(const FGameplayTagContainer& SpecTags)
	{
		if (SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Reaction_KnockUp))
		{
			return AgGameplayTags::Event_Hit_KnockUp;
		}
		if (SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Reaction_KnockDown))
		{
			return AgGameplayTags::Event_Hit_KnockDown;
		}
		return FGameplayTag();
	}

	FGameplayTag ResolveHitReactionEvent(const AAgCharacterBase& Target, const FGameplayTagContainer& SpecTags)
	{
		const UAbilitySystemComponent* ASC = Target.GetAbilitySystemComponent();
		if (ASC->HasMatchingGameplayTag(AgGameplayTags::State_SuperArmor))
		{
			return FGameplayTag();
		}

		EAgToughness Toughness = Target.GetCharacterData()->Toughness;
		if (SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_IgnoreToughness))
		{
			Toughness = EAgToughness::Small;
		}
		else if (ASC->HasMatchingGameplayTag(AgGameplayTags::State_Groggy) && Toughness != EAgToughness::Small)
		{
			Toughness = static_cast<EAgToughness>(static_cast<uint8>(Toughness) - 1);
		}

		const FGameplayTag KnockEvent = GetKnockReactionEvent(SpecTags);
		if (KnockEvent.IsValid() && Toughness == EAgToughness::Small)
		{
			return KnockEvent;
		}
		const bool bFlinch = (SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Reaction_Flinch) && Toughness == EAgToughness::Small) ||
		                     (KnockEvent.IsValid() && Toughness == EAgToughness::Medium);
		if (!bFlinch)
		{
			return FGameplayTag();
		}
		if (ASC->HasMatchingGameplayTag(AgGameplayTags::State_HitReaction_KnockUp) && ASC->HasMatchingGameplayTag(AgGameplayTags::State_Airborne))
		{
			return AgGameplayTags::Event_Hit_Flinch;
		}
		const bool bOtherReaction = ASC->HasMatchingGameplayTag(AgGameplayTags::State_HitReaction) &&
		                            !ASC->HasMatchingGameplayTag(AgGameplayTags::State_HitReaction_Flinch);
		return bOtherReaction ? FGameplayTag() : AgGameplayTags::Event_Hit_Stagger;
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

	FGameplayTag ResolveStaggerEvent(const AActor& Causer, const AAgCharacterBase& Target, const FGameplayEffectContextHandle& Context)
	{
		const float Angle = UKismetAnimationLibrary::CalculateDirection(Causer.GetActorLocation() - Target.GetActorLocation(), Target.GetActorRotation());
		if (FMath::Abs(Angle) >= 135.f)
		{
			return AgGameplayTags::Event_Hit_Stagger_Back;
		}
		if (FMath::Abs(Angle) > 45.f)
		{
			return Angle > 0.f ? AgGameplayTags::Event_Hit_Stagger_Right : AgGameplayTags::Event_Hit_Stagger_Left;
		}

		// 사람형 캐릭터가 모두 SK_Mannequin 스켈레톤이라 기준 뼈 이름을 상수로 둡니다.
		static const TMap<FName, FGameplayTag> ZoneBones = {
			{TEXT("neck_01"), AgGameplayTags::Event_Hit_Stagger_Head},
			{TEXT("spine_04"), AgGameplayTags::Event_Hit_Stagger_Head},
			{TEXT("clavicle_r"), AgGameplayTags::Event_Hit_Stagger_ArmRight},
			{TEXT("clavicle_l"), AgGameplayTags::Event_Hit_Stagger_ArmLeft},
			{TEXT("pelvis"), AgGameplayTags::Event_Hit_Stagger_Body},
		};
		const FHitResult* Contact = Context.GetHitResult();
		const USkeletalMeshComponent* Mesh = Target.GetMesh();
		for (FName Bone = Contact ? Contact->BoneName : NAME_None; !Bone.IsNone(); Bone = Mesh->GetParentBone(Bone))
		{
			if (const FGameplayTag* Zone = ZoneBones.Find(Bone))
			{
				return *Zone;
			}
		}
		return AgGameplayTags::Event_Hit_Stagger_Body;
	}

	FGameplayTag StartHitReaction(AActor* Causer, AAgCharacterBase* Target, const FGameplayTagContainer& SpecTags, const FGameplayEffectContextHandle& Context)
	{
		const UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
		const bool bHasReaction = SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Reaction_Flinch) || GetKnockReactionEvent(SpecTags).IsValid();
		// 처형은 두 캐릭터의 모션을 프레임 단위로 맞물리므로 흔들지도 않습니다.
		if (!bHasReaction || TargetASC->HasMatchingGameplayTag(AgGameplayTags::State_Execution))
		{
			return FGameplayTag();
		}
		FGameplayTag Event = ResolveHitReactionEvent(*Target, SpecTags);
		if (Event.IsValid())
		{
			Event = Event == AgGameplayTags::Event_Hit_Stagger ? ResolveStaggerEvent(*Causer, *Target, Context) : Event;
			SendEvent(Target, Event, Causer);
		}
		Target->GetHitPhysics()->Play(MakePhysicsHit(*Causer, *Target, Context, false));
		return Event;
	}

	FGameplayTag StartGuardReaction(AActor* Causer, AAgCharacterBase* Target, const FGameplayTagContainer& SpecTags, const FGameplayEffectContextHandle& Context)
	{
		const bool bPushback = GetKnockReactionEvent(SpecTags).IsValid();
		if (!bPushback && !SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Reaction_Flinch))
		{
			return FGameplayTag();
		}
		FGameplayTag Event;
		if (bPushback && !Target->GetAbilitySystemComponent()->HasMatchingGameplayTag(AgGameplayTags::State_SuperArmor))
		{
			Event = AgGameplayTags::Event_Guard_Pushback;
			SendEvent(Target, Event, Causer);
		}
		else
		{
			Target->PlayAnimMontage(Target->GetGuardFlinchMontage());
		}
		Target->GetHitPhysics()->Play(MakePhysicsHit(*Causer, *Target, Context, true));
		return Event;
	}

	void ApplyHitstop(UAgAbilitySystemComponent& ASC, float Duration)
	{
		const FGameplayEffectSpecHandle Spec = ASC.MakeEffectSpec(EAgEffect::Hitstop, 1.f, ASC.MakeEffectContext());
		Spec.Data->SetDuration(Duration, true);
		ASC.ApplyGameplayEffectSpecToSelf(*Spec.Data);
	}

	void StartPerfectDodge(AAgCharacterBase& Dodger, AAgCharacterBase& Attacker)
	{
		const FGameplayTagContainer WindowTag(AgGameplayTags::State_PerfectDodgeWindow);
		if (Dodger.GetAbilitySystemComponent()->RemoveActiveEffectsWithGrantedTags(WindowTag) == 0)
		{
			return;
		}
		SendEvent(&Dodger, AgGameplayTags::Event_PerfectDodge, &Attacker);
		FGameplayCueParameters Parameters;
		Parameters.Instigator = &Attacker;
		Parameters.Location = Dodger.GetActorLocation();
		Dodger.GetAbilitySystemComponent()->ExecuteGameplayCue(AgGameplayTags::GameplayCue_PerfectDodge, Parameters);
	}

	void DamageParriedAttacker(UAgAbilitySystemComponent& DefenderASC, AAgCharacterBase& Attacker)
	{
		const FGameplayEffectSpecHandle Spec = DefenderASC.MakeEffectSpec(EAgEffect::Damage, 1.f, DefenderASC.MakeEffectContext());
		if (!Spec.IsValid())
		{
			return;
		}
		const UAgCombatRules& Rules = UAgAssetManager::Get().GetCombatRules();
		FAgDamage Damage;
		Damage.HPMultiplier = Rules.ParryHPMultiplier;
		Damage.PPMultiplier = Rules.ParryPPMultiplier;
		Damage.AddToSpec(*Spec.Data);
		Spec.Data->AddDynamicAssetTag(AgGameplayTags::Effect_Damage_Parry);
		Spec.Data->GetContext().AddOrigin(Attacker.GetActorLocation());
		DefenderASC.ApplyGameplayEffectSpecToTarget(*Spec.Data, Attacker.GetAbilitySystemComponent());
	}

	void GrantGuardCounterChance(UAbilitySystemComponent& DefenderASC)
	{
		DefenderASC.ApplyGameplayEffectToSelf(UAgAssetManager::Get().FindEffect(GuardCounterChanceEffectId), 1.f,
			DefenderASC.MakeEffectContext());
	}

	void LockMove(UAbilitySystemComponent& DefenderASC)
	{
		DefenderASC.ApplyGameplayEffectToSelf(UAgAssetManager::Get().FindEffect(MoveLockEffectId), 1.f, DefenderASC.MakeEffectContext());
	}

} // namespace AgDamageResultGameplayEffectComponentPrivate

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
		AgDamageResultGameplayEffectComponentPrivate::StartPerfectDodge(*Target, *Attacker);
		return;
	}

	const bool bParry = SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Result_Parry);
	const bool bGuard = SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Result_Guard);
	const bool bHit = SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Result_Hit);
	if (!bParry && !bGuard && !bHit)
	{
		return;
	}
	UAISense_Damage::ReportDamageEvent(Target, Target, Attacker, 1.f, Attacker->GetActorLocation(), Context.GetOrigin());
	const UAgAttributeSet* TargetStats = Target->GetAttributeSet();
	UE_LOG(LogAg, Verbose, TEXT("Hit: %s -> %s, parry %d, guard %d, HP now %.0f, PP now %.0f"),
		*Attacker->GetName(), *Target->GetName(), bParry, bGuard, TargetStats->GetHP(), TargetStats->GetPP());

	// 사망 판정(어트리뷰트 세트)이 이 처리보다 먼저이므로, 이번 타격으로 죽었으면 반응하지 않습니다.
	FGameplayTag Reaction;
	if (!Target->IsDead())
	{
		if (bParry)
		{
			TargetASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(AgGameplayTags::State_ParryWindow));
			if (!Projectile)
			{
				AgDamageResultGameplayEffectComponentPrivate::DamageParriedAttacker(*Target->GetAbilitySystemComponent(), *Attacker);
			}
			AgDamageResultGameplayEffectComponentPrivate::SendEvent(Target, AgGameplayTags::Event_Parry, Causer, Context);
		}
		else if (bGuard)
		{
			Reaction = AgDamageResultGameplayEffectComponentPrivate::StartGuardReaction(Causer, Target, SpecTags, Context);
			AgDamageResultGameplayEffectComponentPrivate::LockMove(*TargetASC);
		}
		else
		{
			Reaction = AgDamageResultGameplayEffectComponentPrivate::StartHitReaction(Causer, Target, SpecTags, Context);
		}
		// 패리 어빌리티가 발동하며 남은 기회를 지우므로, 패리 이벤트를 보낸 뒤에 겁니다.
		if (bParry || bGuard)
		{
			AgDamageResultGameplayEffectComponentPrivate::GrantGuardCounterChance(*TargetASC);
		}
	}

	if (!bHit)
	{
		AgDamageResultGameplayEffectComponentPrivate::SendEvent(Attacker, AgGameplayTags::Event_AttackBlocked, Target, Context);
	}

	FGameplayCueParameters CueParameters;
	CueParameters.Location = Context.GetOrigin();
	CueParameters.Instigator = Attacker;
	CueParameters.EffectCauser = Causer;
	CueParameters.AggregatedSourceTags = SpecTags;
	// 이전 타격의 상태가 남아 있어도 이번에 실제로 발생한 반응만 연출에 전달합니다.
	if (Reaction.IsValid())
	{
		CueParameters.AggregatedTargetTags.AddTag(Reaction);
	}
	// 패리 대미지는 패리가 막은 쪽에서 낸 연출과 히트스톱이 대신하므로 결과 연출과 대미지 숫자를 내지 않습니다.
	const bool bParryDamage = SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Parry);
	if (!bParryDamage)
	{
		const FGameplayTag ResultCue = bParry   ? AgGameplayTags::GameplayCue_Parry
		                               : bGuard ? AgGameplayTags::GameplayCue_Guard
		                                        : AgGameplayTags::GameplayCue_Hit;
		TargetASC->ExecuteGameplayCue(ResultCue, CueParameters);
	}
	// 엔진이 범위로 고정하기 전의 변화량이라, 남은 HP로 줄이지 않은 피해량을 보여 줍니다.
	if (const FGameplayEffectModifiedAttribute* HPChange = std::as_const(GESpec).GetModifiedAttribute(UAgAttributeSet::GetHPAttribute());
		!bParryDamage && HPChange && HPChange->TotalMagnitude < 0.f)
	{
		FGameplayCueParameters NumberParameters = CueParameters;
		NumberParameters.RawMagnitude = -HPChange->TotalMagnitude;
		if (SpecTags.HasTagExact(AgGameplayTags::Ability_Action_Execution))
		{
			NumberParameters.Location = Target->GetActorLocation();
		}
		TargetASC->ExecuteGameplayCue(AgGameplayTags::GameplayCue_DamageFloater, NumberParameters);
	}

	if (!SpecTags.HasTagExact(AgGameplayTags::Ability_Action_Execution) && !bParryDamage)
	{
		const float HitstopDuration = UAgAssetManager::Get().GetCombatRules().HitstopDuration;
		AgDamageResultGameplayEffectComponentPrivate::ApplyHitstop(*Target->GetAbilitySystemComponent(), HitstopDuration);
		if (Causer == Attacker)
		{
			AgDamageResultGameplayEffectComponentPrivate::ApplyHitstop(*Attacker->GetAbilitySystemComponent(), HitstopDuration);
		}
	}

	const TSharedPtr<FAgHitState> HitState = FAgGameplayEffectContext::GetHitState(Context);
	if (bHit && HitState && !HitState->bLanded)
	{
		// 이 이벤트로 발동한 어빌리티가 다시 적중을 일으킬 수 있으므로 보내기 전에 표시합니다.
		HitState->bLanded = true;
		AgDamageResultGameplayEffectComponentPrivate::SendEvent(Attacker, AgGameplayTags::Event_AttackHit, Target, Context, SpecTags);
	}
	if (bParry && IsValid(Projectile) && !Target->IsDead())
	{
		Projectile->Reflect(*Target);
	}
	if (!bParry && IsValid(Projectile))
	{
		Projectile->OnDamageApplied();
	}
}
