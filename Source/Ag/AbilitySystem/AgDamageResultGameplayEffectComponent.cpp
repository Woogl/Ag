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
#include "Character/AgPlayerCharacter.h"
#include "Combat/AgProjectile.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/AgAssetManager.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgAbilityData.h"
#include "Data/AgCharacterData.h"
#include "Data/AgCombatRules.h"
#include "Data/AgEffectData.h"
#include "GameplayEffect.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Damage.h"

namespace
{
	const FName GuardFlinchMontageId(TEXT("GuardFlinch"));
	const FName GuardCounterChanceEffectId(TEXT("GuardCounterChance"));

	void SendEvent(AActor* Target, const FGameplayTag& EventTag, AActor* Instigator, const FGameplayEffectContextHandle& Context = FGameplayEffectContextHandle())
	{
		FGameplayEventData Payload;
		Payload.EventTag = EventTag;
		Payload.Instigator = Instigator;
		Payload.Target = Target;
		Payload.ContextHandle = Context;
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

		const bool bGroggy = ASC->HasMatchingGameplayTag(AgGameplayTags::State_Groggy);
		EAgToughness Toughness = Target.GetCharacterData()->Toughness;
		if (bGroggy && Toughness != EAgToughness::Small)
		{
			Toughness = static_cast<EAgToughness>(static_cast<uint8>(Toughness) - 1);
		}

		const FGameplayTag KnockEvent = GetKnockReactionEvent(SpecTags);
		if (KnockEvent.IsValid() && Toughness == EAgToughness::Small)
		{
			return KnockEvent;
		}
		const bool bFlinch = SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Reaction_Flinch) || (KnockEvent.IsValid() && Toughness == EAgToughness::Medium);
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
		return (Toughness == EAgToughness::Small || KnockEvent.IsValid()) && !bOtherReaction ? AgGameplayTags::Event_Hit_Stagger : FGameplayTag();
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
		const FVector ToCauser = (Causer.GetActorLocation() - Target.GetActorLocation()).GetSafeNormal2D();
		if (FVector::DotProduct(ToCauser, -Target.GetActorForwardVector().GetSafeNormal2D()) >= UE_HALF_SQRT_2)
		{
			return AgGameplayTags::Event_Hit_Stagger_Back;
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
			Target->PlayAnimMontage(Target->GetCharacterData()->FindMontage(GuardFlinchMontageId));
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
		if (AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(&Dodger))
		{
			Player->PlayCameraEffect(EAgCameraSituation::PerfectDodge, &Attacker);
		}
	}

	void ApplyParry(AAgCharacterBase& Defender, AAgCharacterBase& Attacker, bool bProjectile)
	{
		if (!bProjectile)
		{
			SendEvent(&Attacker, AgGameplayTags::Event_Parried, &Defender);
		}
		Defender.GetAbilitySystemComponent()->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(AgGameplayTags::State_ParryWindow));
	}

	void GrantGuardCounterChance(UAbilitySystemComponent& DefenderASC)
	{
		DefenderASC.ApplyGameplayEffectToSelf(UAgAssetManager::Get().FindEffect(GuardCounterChanceEffectId), 1.f,
			DefenderASC.MakeEffectContext());
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
		StartPerfectDodge(*Target, *Attacker);
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
	UAISense_Damage::ReportDamageEvent(Target, Target, Attacker, 1.f, Attacker->GetActorLocation(), Context.GetOrigin());
	// 반사탄 적중은 패리당한 것으로 보아 Parried 어빌리티가 PP를 줄이므로, 반사한 피해에는 PP 배율이 없습니다.
	if (bHit && Projectile && Projectile->IsReflected() && !Target->IsDead())
	{
		SendEvent(Target, AgGameplayTags::Event_Parried, Attacker);
	}
	const UAgAttributeSet* TargetStats = Target->GetAttributeSet();
	UE_LOG(LogAg, Verbose, TEXT("Hit: %s -> %s, parry %d, guard %d, guard break %d, HP now %.0f, PP now %.0f, SP now %.0f"),
		*Attacker->GetName(), *Target->GetName(), bParry, bGuard, bGuardBreak, TargetStats->GetHP(), TargetStats->GetPP(),
		TargetStats->GetSP());

	// 사망 판정(어트리뷰트 세트)이 이 처리보다 먼저이므로, 이번 타격으로 죽었으면 반응하지 않습니다.
	FGameplayTag Reaction;
	if (!Target->IsDead())
	{
		if (bParry)
		{
			ApplyParry(*Target, *Attacker, Projectile != nullptr);
			SendEvent(Target, AgGameplayTags::Event_Parry, Causer, Context);
		}
		else if (bGuard)
		{
			Reaction = StartGuardReaction(Causer, Target, SpecTags, Context);
		}
		else if (bGuardBreak)
		{
			SendEvent(Target, AgGameplayTags::Event_Guard_Break, Causer);
		}
		else
		{
			Reaction = StartHitReaction(Causer, Target, SpecTags, Context);
		}
		// 패리 어빌리티가 발동하며 남은 기회를 지우므로, 패리 이벤트를 보낸 뒤에 겁니다.
		if (bParry || bGuard)
		{
			GrantGuardCounterChance(*TargetASC);
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
		Player->PlayDamageCameraEffects(SpecTags, Reaction, *Attacker, *Target);
	}
	// 엔진이 범위로 고정하기 전의 변화량이라, 남은 HP로 줄이지 않은 피해량을 보여 줍니다.
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
		ApplyHitstop(*Target->GetAbilitySystemComponent(), HitstopDuration);
		if (Causer == Attacker && !SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_NoAttackerHitstop))
		{
			ApplyHitstop(*Attacker->GetAbilitySystemComponent(), HitstopDuration);
		}
	}

	const TSharedPtr<FAgHitRewardState> Reward = FAgGameplayEffectContext::GetHitReward(Context);
	if (bHit && Reward && !Reward->bGranted)
	{
		UAgAbilitySystemComponent* AttackerASC = Attacker->GetAbilitySystemComponent();
		const FGameplayEffectSpecHandle ChargeSpec = AttackerASC->MakeEffectSpec(EAgEffect::Resource, 1.f, FGameplayEffectContextHandle());
		if (ChargeSpec.IsValid())
		{
			// 자원 변경 콜백이 다시 적중을 일으킬 수 있으므로 적용 전에 지급 기회를 사용합니다.
			Reward->bGranted = true;
			Reward->Amounts.AddToSpec(*ChargeSpec.Data);
			AttackerASC->ApplyGameplayEffectSpecToSelf(*ChargeSpec.Data);
		}
	}
	if (bParry && IsValid(Projectile) && !Target->IsDead())
	{
		Projectile->Reflect(*Target);
	}
}
