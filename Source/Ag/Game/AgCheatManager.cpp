// Copyright Woogle. All Rights Reserved.

#include "Game/AgCheatManager.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "Ag.h"
#include "BrainComponent.h"
#include "Character/AgBossCharacter.h"
#include "Character/AgEnemyCharacter.h"
#include "Character/AgPlayerCharacter.h"
#include "Core/AgGameplayTags.h"
#include "EngineUtils.h"
#include "GameplayEffect.h"

namespace
{
	void Report(const UCheatManager& Cheats, const FString& Message)
	{
		UE_LOG(LogAg, Display, TEXT("%s"), *Message);
		Cheats.GetOuterAPlayerController()->ClientMessage(Message);
	}

	AAgPlayerCharacter* GetPlayer(const UCheatManager& Cheats)
	{
		return Cast<AAgPlayerCharacter>(Cheats.GetOuterAPlayerController()->GetPawn());
	}

	AAgEnemyCharacter* FindTargetEnemy(const AAgPlayerCharacter* Player)
	{
		if (!Player)
		{
			return nullptr;
		}
		if (AAgEnemyCharacter* LockOnTarget = Cast<AAgEnemyCharacter>(Player->GetLockOnTarget()))
		{
			return LockOnTarget;
		}

		AAgEnemyCharacter* Nearest = nullptr;
		double NearestDistanceSq = TNumericLimits<double>::Max();
		for (TActorIterator<AAgEnemyCharacter> It(Player->GetWorld()); It; ++It)
		{
			const double DistanceSq = FVector::DistSquared(It->GetActorLocation(), Player->GetActorLocation());
			if (!It->IsDead() && DistanceSq < NearestDistanceSq)
			{
				Nearest = *It;
				NearestDistanceSq = DistanceSq;
			}
		}
		return Nearest;
	}

	/** 어트리뷰트를 직접 쓰면 사망·그로기 판정을 건너뛰므로, 플레이어가 건 즉시 효과로 바꿉니다. */
	void ApplyOverride(AAgCharacterBase& Source, AAgCharacterBase& Target, const FGameplayAttribute& Attribute, float Value)
	{
		UGameplayEffect* Effect = NewObject<UGameplayEffect>(GetTransientPackage());
		FGameplayModifierInfo& Modifier = Effect->Modifiers.AddDefaulted_GetRef();
		Modifier.Attribute = Attribute;
		Modifier.ModifierOp = EGameplayModOp::Override;
		Modifier.ModifierMagnitude = FScalableFloat(Value);
		UAbilitySystemComponent* SourceASC = Source.GetAbilitySystemComponent();
		SourceASC->ApplyGameplayEffectToTarget(Effect, Target.GetAbilitySystemComponent(), 1.f, SourceASC->MakeEffectContext());
	}

	void SetEnemyAIRunning(UWorld* World, bool bRunning)
	{
		for (TActorIterator<AAgEnemyCharacter> It(World); It; ++It)
		{
			const AController* Controller = It->GetController();
			UBrainComponent* Brain = Controller ? Controller->FindComponentByClass<UBrainComponent>() : nullptr;
			if (!Brain || It->IsDead() || Brain->IsRunning() == bRunning)
			{
				continue;
			}
			if (bRunning)
			{
				Brain->StartLogic();
			}
			else
			{
				Brain->StopLogic(TEXT("Cheat"));
			}
		}
	}
} // namespace

void UAgCheatManager::AgKillPlayer()
{
	for (TActorIterator<AAgPlayerCharacter> It(GetWorld()); It; ++It)
	{
		It->Kill();
	}
}

void UAgCheatManager::AgKillBoss()
{
	for (TActorIterator<AAgBossCharacter> It(GetWorld()); It; ++It)
	{
		It->Kill();
	}
}

void UAgCheatManager::AgKillEnemies()
{
	for (TActorIterator<AAgEnemyCharacter> It(GetWorld()); It; ++It)
	{
		if (!It->IsA<AAgBossCharacter>())
		{
			It->Kill();
		}
	}
}

void UAgCheatManager::AgGod()
{
	AAgPlayerCharacter* Player = GetPlayer(*this);
	if (!Player)
	{
		Report(*this, TEXT("AgGod: no player character"));
		return;
	}

	bGod = !bGod;
	// 회피·처형도 같은 태그를 붙였다 떼므로, 개수로 더하고 빼야 그 어빌리티의 무적과 섞이지 않습니다.
	UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();
	if (bGod)
	{
		ASC->AddLooseGameplayTag(AgGameplayTags::State_Invincible);
	}
	else
	{
		ASC->RemoveLooseGameplayTag(AgGameplayTags::State_Invincible);
	}
	Report(*this, bGod ? TEXT("AgGod: on") : TEXT("AgGod: off"));
}

void UAgCheatManager::AgFillMPUP()
{
	AAgPlayerCharacter* Player = GetPlayer(*this);
	if (!Player)
	{
		Report(*this, TEXT("AgFillMPUP: no player character"));
		return;
	}

	const UAgAttributeSet* Stats = Player->GetAttributeSet();
	ApplyOverride(*Player, *Player, UAgAttributeSet::GetMPAttribute(), Stats->GetMaxMP());
	ApplyOverride(*Player, *Player, UAgAttributeSet::GetUPAttribute(), Stats->GetMaxUP());
}

void UAgCheatManager::AgGroggy()
{
	AAgPlayerCharacter* Player = GetPlayer(*this);
	AAgEnemyCharacter* Enemy = FindTargetEnemy(Player);
	if (!Enemy || !Enemy->CanLosePP())
	{
		Report(*this, TEXT("AgGroggy: no target enemy that can lose PP"));
		return;
	}

	ApplyOverride(*Player, *Enemy, UAgAttributeSet::GetPPAttribute(), 0.f);
}

void UAgCheatManager::AgPattern(FName Id)
{
	AAgEnemyCharacter* Enemy = FindTargetEnemy(GetPlayer(*this));
	const FGameplayAbilitySpec* Spec = Enemy ? Enemy->FindAbilitySpec(Id) : nullptr;
	if (!Spec)
	{
		Report(*this, FString::Printf(TEXT("AgPattern: no target enemy with '%s'"), *Id.ToString()));
		return;
	}

	const FGameplayAbilitySpecHandle Handle = Spec->Handle;
	UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent();
	ASC->RemoveActiveEffectsWithTags(FGameplayTagContainer(AgGameplayTags::Effect_Cooldown));
	if (!ASC->TryActivateAbility(Handle))
	{
		Report(*this, FString::Printf(TEXT("AgPattern: %s could not start '%s'"), *Enemy->GetName(), *Id.ToString()));
	}
}

void UAgCheatManager::AgSetHP(float Percent)
{
	AAgPlayerCharacter* Player = GetPlayer(*this);
	AAgEnemyCharacter* Enemy = FindTargetEnemy(Player);
	if (!Enemy)
	{
		Report(*this, TEXT("AgSetHP: no target enemy"));
		return;
	}

	const float HP = FMath::RoundToFloat(Enemy->GetAttributeSet()->GetMaxHP() * Percent / 100.f);
	ApplyOverride(*Player, *Enemy, UAgAttributeSet::GetHPAttribute(), HP);
}

void UAgCheatManager::AgStopAI()
{
	SetEnemyAIRunning(GetWorld(), false);
}

void UAgCheatManager::AgStartAI()
{
	SetEnemyAIRunning(GetWorld(), true);
}
