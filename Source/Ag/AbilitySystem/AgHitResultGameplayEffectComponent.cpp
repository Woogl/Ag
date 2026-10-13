// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgHitResultGameplayEffectComponent.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "AbilitySystem/AgEffect_Resource.h"
#include "AbilitySystem/AgEffect_TimedTag.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Ag.h"
#include "Character/AgCharacterBase.h"
#include "Character/AgPlayerCharacter.h"
#include "Combat/AgTimeSubsystem.h"
#include "Core/AgAssetManager.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgCameraData.h"
#include "Data/AgCharacterData.h"
#include "Data/AgCombatRules.h"
#include "GameplayCueFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	/** Context는 이벤트를 받는 쪽이 공격의 출처(공격한 캐릭터, 투사체)를 알아야 할 때 넘깁니다(패리 보상). */
	void SendEvent(AActor* Target, const FGameplayTag& EventTag, AActor* Instigator, const FGameplayEffectContextHandle& Context = FGameplayEffectContextHandle())
	{
		FGameplayEventData Payload;
		Payload.EventTag = EventTag;
		Payload.Instigator = Instigator;
		Payload.Target = Target;
		Payload.ContextHandle = Context;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Target, EventTag, Payload);
	}

	/** 넉업, 넉백 어빌리티를 시작하는 이벤트. 움찔이거나 반응이 없으면 비어 있습니다. */
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

	/**
	 * 피격 반응: 움찔은 하던 동작 위에 덧입히고, 넉업·넉백은 반응 어빌리티를 시작합니다(그로기 중에는 움찔).
	 * Instigator는 밀려나는 기준(공격자 또는 투사체)입니다.
	 */
	void StartHitReaction(AActor* Instigator, AAgCharacterBase* Target, const FGameplayTagContainer& SpecTags)
	{
		const UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
		const FGameplayTag Event = GetReactionEvent(SpecTags);
		if (Event.IsValid() && !TargetASC->HasMatchingGameplayTag(AgGameplayTags::State_Groggy))
		{
			SendEvent(Target, Event, Instigator);
		}
		// 움찔은 이미 재생 중인 넉업, 넉백을 바꾸지 않습니다.
		else if ((Event.IsValid() || SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Reaction_Flinch))
			&& !TargetASC->HasMatchingGameplayTag(AgGameplayTags::State_HitReaction))
		{
			Target->PlayFlinch();
		}
	}

	/** 가드 반응: 움찔은 가드 움찔, 넉업·넉백은 가드 밀림이 됩니다. */
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

	/** 카메라 '상황별 단계'. 처형은 찌르기와 마지막 일격의 단계를 씁니다. */
	EAgCameraShake GetHitShake(const UAgCameraData& Camera, const FGameplayTagContainer& SpecTags, bool bPlayerAttacked)
	{
		const bool bFinisher = SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Finisher);
		if (SpecTags.HasTagExact(AgGameplayTags::Ability_Action_Execution))
		{
			return bFinisher ? Camera.ExecutionBlowShake : Camera.ExecutionStabShake;
		}

		// 넉업·넉백은 움찔보다 크게 흔듭니다.
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
}

void UAgHitResultGameplayEffectComponent::OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
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
		// 극한 회피: 보상은 회피 어빌리티가 회피마다 한 번 줍니다.
		SendEvent(Target, AgGameplayTags::Event_PerfectDodge, Causer);
		return;
	}

	const bool bParry = SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Result_Parry);
	const bool bGuard = SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Result_Guard);
	const bool bGuardBreak = SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Result_GuardBreak);
	const bool bHit = SpecTags.HasTagExact(AgGameplayTags::Effect_Damage_Result_Hit);
	if (!bParry && !bGuard && !bGuardBreak && !bHit)
	{
		// 무적: 아무 일도 일어나지 않습니다.
		return;
	}
	const UAgAttributeSet* TargetStats = Target->GetAttributeSet();
	UE_LOG(LogAg, Verbose, TEXT("Hit: %s -> %s, parry %d, guard %d, guard break %d, HP now %.0f, PP now %.0f, SP now %.0f"),
		*Attacker->GetName(), *Target->GetName(), bParry, bGuard, bGuardBreak, TargetStats->GetHP(), TargetStats->GetPP(), TargetStats->GetSP());

	// 사망·그로기 판정(어트리뷰트 세트)이 피격 반응보다 먼저입니다. 이번 타격으로 죽었거나 그로기가 됐으면 반응하지 않습니다.
	const bool bBecameGroggy = !GESpec.CapturedTargetTags.GetActorTags().HasTag(AgGameplayTags::State_Groggy)
		&& TargetASC->HasMatchingGameplayTag(AgGameplayTags::State_Groggy);
	if (!Target->IsDead() && !bBecameGroggy)
	{
		if (bParry)
		{
			// 패리 모션과 패리 보상(MP·UP 충전, 공격한 쪽의 PP 감소)은 패리 구간을 연 가드 어빌리티가 맡습니다.
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

	// 막힘: 가드·가드 브레이크·패리면 공격자에게 알립니다.
	if (!bHit)
	{
		SendEvent(Attacker, AgGameplayTags::Event_AttackBlocked, Target);
	}

	// 적중 후 처리: 결과마다 이펙트와 효과음(가드 브레이크는 가드 큐, 궁극기 마무리 적중은 피격 큐 대신 궁극기 마무리 큐),
	// 카메라 셰이크, 대미지 숫자, 히트스톱, 슬로우모션(궁극기 마무리), MP·UP 충전.
	const bool bUltimate = SpecTags.HasTagExact(AgGameplayTags::Ability_Action_Ultimate);
	FGameplayCueParameters CueParameters;
	CueParameters.Location = Context.GetOrigin();
	CueParameters.Instigator = Attacker;
	CueParameters.EffectCauser = Causer;
	const FGameplayTag ResultCue = bParry ? AgGameplayTags::GameplayCue_Parry
		: (bGuard || bGuardBreak) ? AgGameplayTags::GameplayCue_Guard
		: bUltimate ? AgGameplayTags::GameplayCue_UltimateFinish
		: AgGameplayTags::GameplayCue_Hit;
	TargetASC->ExecuteGameplayCue(ResultCue, CueParameters);
	// 엔진 Burst 큐는 큐를 실행한 캐릭터가 로컬 플레이어일 때 그 카메라를 흔들므로, 셰이크 큐는 로컬 플레이어 캐릭터에서 실행합니다.
	const EAgCameraShake Shake = GetHitShake(UAgAssetManager::Get().GetCameraData(), SpecTags, Attacker->IsA<AAgPlayerCharacter>());
	UGameplayCueFunctionLibrary::ExecuteGameplayCueOnActor(UGameplayStatics::GetPlayerPawn(Target, 0), UAgCameraData::GetShakeCue(Shake), FGameplayCueParameters());
	if (const FGameplayEffectModifiedAttribute* HPChange = std::as_const(GESpec).GetModifiedAttribute(UAgAttributeSet::GetHPAttribute());
		HPChange && HPChange->TotalMagnitude < 0.f)
	{
		// 숫자는 깎으려던 HP 감소량이라, 남은 HP보다 커도 그대로 띄웁니다.
		CueParameters.RawMagnitude = -HPChange->TotalMagnitude;
		TargetASC->ExecuteGameplayCue(AgGameplayTags::GameplayCue_DamageNumber, CueParameters);
	}

	// 히트스톱은 캐릭터마다 태그로 겁니다. 투사체는 쏜 캐릭터를 멈추지 않고, 처형에는 히트스톱이 없습니다.
	if (!SpecTags.HasTagExact(AgGameplayTags::Ability_Action_Execution))
	{
		const float HitstopTime = UAgAssetManager::Get().GetCombatRules().HitstopTime;
		UAgEffect_TimedTag::Apply(TargetASC, AgGameplayTags::State_Hitstop, HitstopTime);
		if (Causer == Attacker)
		{
			UAgEffect_TimedTag::Apply(Attacker->GetAbilitySystemComponent(), AgGameplayTags::State_Hitstop, HitstopTime);
		}
	}

	// 적이 궁극기 마무리에 맞는 순간을 슬로우모션으로 강조합니다. 여러 적이 맞아도 같은 값으로 다시 시작할 뿐입니다.
	if (bHit && bUltimate)
	{
		const UAgPlayerData* Data = CastChecked<UAgPlayerData>(Attacker->GetCharacterData());
		Target->GetWorld()->GetSubsystem<UAgTimeSubsystem>()->StartSlowMotion(Data->UltimateSlowMotionSpeed, Data->UltimateSlowMotionDuration);
	}

	// 일반 피격이면 공격한 쪽이 공격 데이터의 MP·UP를 얻습니다.
	const float MPCharge = GESpec.GetSetByCallerMagnitude(AgGameplayTags::SetByCaller_MP);
	const float UPCharge = GESpec.GetSetByCallerMagnitude(AgGameplayTags::SetByCaller_UP);
	if (bHit && (MPCharge > 0.f || UPCharge > 0.f))
	{
		UAbilitySystemComponent* AttackerASC = Attacker->GetAbilitySystemComponent();
		const FGameplayEffectSpecHandle Charge = AttackerASC->MakeOutgoingSpec(UAgEffect_Resource::StaticClass(), 1.f, AttackerASC->MakeEffectContext());
		UAgEffect_Resource::SetAmounts(*Charge.Data, 0.f, MPCharge, UPCharge);
		AttackerASC->ApplyGameplayEffectSpecToSelf(*Charge.Data);
	}
}
