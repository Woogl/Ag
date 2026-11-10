// Copyright Woogle. All Rights Reserved.

#include "Game/AgCheatManager.h"

#include "AbilitySystem/AgAttributeSet.h"
#include "Ag.h"
#include "BrainComponent.h"
#include "Character/AgBossCharacter.h"
#include "Character/AgEnemyCharacter.h"
#include "Character/AgPlayerCharacter.h"
#include "Core/AgGameplayTags.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/ImmunityGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"

namespace AgCheatManagerPrivate
{
	void Report(const UCheatManager& Cheats, const FString& Message)
	{
		UE_LOG(LogAg, Display, TEXT("%s"), *Message);
		Cheats.GetOuterAPlayerController()->ClientMessage(Message);
	}

	/**
	 * 인자가 없으면 지금 상태를 뒤집고, 0·1이면 그 상태로 둡니다. 인자를 문자열로 받는 까닭은, 엔진이 문자열이 아닌
	 * 인자가 비면 에디터 밖에서 명령을 실행하지 않기 때문입니다.
	 */
	TOptional<bool> ParseSwitch(const UCheatManager& Cheats, const TCHAR* Command, const FString& OnOff, bool bCurrent)
	{
		if (OnOff.IsEmpty())
		{
			return !bCurrent;
		}
		if (OnOff == TEXT("0") || OnOff == TEXT("1"))
		{
			return OnOff == TEXT("1");
		}
		Report(Cheats, FString::Printf(TEXT("%s: use 1, 0, or no argument to toggle"), Command));
		return {};
	}

	void ReportSwitch(const UCheatManager& Cheats, const TCHAR* Command, bool bOn)
	{
		Report(Cheats, FString::Printf(TEXT("%s: %s"), Command, bOn ? TEXT("on") : TEXT("off")));
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

	constexpr EAgCheatSwitch AllSwitches[] = {EAgCheatSwitch::Invincible, EAgCheatSwitch::NoCooldown, EAgCheatSwitch::InfiniteMP,
		EAgCheatSwitch::InfiniteUP};

	struct FAgLockedResource
	{
		FGameplayAttribute Attribute;
		FGameplayAttribute MaxAttribute;
	};

	FAgLockedResource GetLockedResource(EAgCheatSwitch Switch)
	{
		switch (Switch)
		{
			case EAgCheatSwitch::InfiniteMP:
				return {UAgAttributeSet::GetMPAttribute(), UAgAttributeSet::GetMaxMPAttribute()};
			case EAgCheatSwitch::InfiniteUP:
				return {UAgAttributeSet::GetUPAttribute(), UAgAttributeSet::GetMaxUPAttribute()};
			default:
				return {};
		}
	}

	UGameplayEffect* MakeSwitchEffect(EAgCheatSwitch Switch)
	{
		UGameplayEffect* Effect = NewObject<UGameplayEffect>(GetTransientPackage());
		Effect->DurationPolicy = EGameplayEffectDurationType::Infinite;
		if (Switch == EAgCheatSwitch::Invincible)
		{
			// 회피·처형도 같은 태그를 붙였다 떼지만, 태그는 개수로 쌓이므로 그 어빌리티가 떼도 이 효과의 무적은 남습니다.
			FInheritedTagContainer Tags;
			Tags.AddTag(AgGameplayTags::State_Invincible);
			Effect->AddComponent<UTargetTagsGameplayEffectComponent>().SetAndApplyTargetTagChanges(Tags);
		}
		else if (Switch == EAgCheatSwitch::NoCooldown)
		{
			// 엔진 AbilitySystem.IgnoreCooldowns는 모든 캐릭터에 걸리고 쿨다운 효과는 그대로 걸어 충전 줄이 계속 쌓이므로,
			// 플레이어에게 쿨다운 효과 면역을 겁니다.
			const FGameplayTagContainer CooldownTag(AgGameplayTags::Effect_Cooldown);
			Effect->AddComponent<UImmunityGameplayEffectComponent>().ImmunityQueries.Add(FGameplayEffectQuery::MakeQuery_MatchAnyEffectTags(CooldownTag));
		}
		else
		{
			const FAgLockedResource Resource = GetLockedResource(Switch);
			FGameplayModifierInfo& Modifier = Effect->Modifiers.AddDefaulted_GetRef();
			Modifier.Attribute = Resource.Attribute;
			Modifier.ModifierOp = EGameplayModOp::Override;
			FAttributeBasedFloat Max;
			Max.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(Resource.MaxAttribute, EGameplayEffectAttributeCaptureSource::Target, false);
			Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(Max);
		}
		return Effect;
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

	/** 뷰포트가 장치 입력을 넘기는 컨트롤러에 넣어, 키 매핑·트리거·바인딩과 처리 순서를 사람의 입력과 같게 합니다. */
	void InjectKey(const UCheatManager& Cheats, const FKey& Key, EInputEvent Event, float Value)
	{
		APlayerController* PlayerController = Cheats.GetOuterAPlayerController();
		const FInputDeviceId Device =
			IPlatformInputDeviceMapper::Get().GetPrimaryInputDeviceForUser(PlayerController->GetPlatformUserId());
		PlayerController->InputKey(FInputKeyEventArgs::CreateSimulated(Key, Event, Value, -1, Device));
	}

	void InjectNamedKey(const UCheatManager& Cheats, const TCHAR* Command, FName KeyName, EInputEvent Event, float Value)
	{
		const FKey Key(KeyName);
		if (!Key.IsValid())
		{
			Report(Cheats, FString::Printf(TEXT("%s: no key '%s'"), Command, *KeyName.ToString()));
			return;
		}
		InjectKey(Cheats, Key, Event, Value);
	}
} // namespace AgCheatManagerPrivate

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

void UAgCheatManager::AgGod(const FString& OnOff)
{
	AAgPlayerCharacter* Player = AgCheatManagerPrivate::GetPlayer(*this);
	if (!Player)
	{
		AgCheatManagerPrivate::Report(*this, TEXT("AgGod: no player character"));
		return;
	}
	const UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();
	bool bAllOn = true;
	for (const EAgCheatSwitch Switch : AgCheatManagerPrivate::AllSwitches)
	{
		bAllOn = bAllOn && IsSwitchOn(*ASC, Switch);
	}
	const TOptional<bool> bOn = AgCheatManagerPrivate::ParseSwitch(*this, TEXT("AgGod"), OnOff, bAllOn);
	if (!bOn.IsSet())
	{
		return;
	}

	for (const EAgCheatSwitch Switch : AgCheatManagerPrivate::AllSwitches)
	{
		SetSwitch(*Player, Switch, *bOn);
	}
	AgCheatManagerPrivate::ReportSwitch(*this, TEXT("AgGod"), *bOn);
}

void UAgCheatManager::AgInvincible(const FString& OnOff)
{
	RunSwitch(TEXT("AgInvincible"), OnOff, EAgCheatSwitch::Invincible);
}

void UAgCheatManager::AgSetHP(float Percent)
{
	AAgPlayerCharacter* Player = AgCheatManagerPrivate::GetPlayer(*this);
	if (!Player)
	{
		AgCheatManagerPrivate::Report(*this, TEXT("AgSetHP: no player character"));
		return;
	}

	const float HP = FMath::RoundToFloat(Player->GetAttributeSet()->GetMaxHP() * Percent / 100.f);
	AgCheatManagerPrivate::ApplyOverride(*Player, *Player, UAgAttributeSet::GetHPAttribute(), HP);
}

void UAgCheatManager::AgSetMP(float Percent)
{
	AAgPlayerCharacter* Player = AgCheatManagerPrivate::GetPlayer(*this);
	if (!Player)
	{
		AgCheatManagerPrivate::Report(*this, TEXT("AgSetMP: no player character"));
		return;
	}

	const float MP = FMath::RoundToFloat(Player->GetAttributeSet()->GetMaxMP() * Percent / 100.f);
	AgCheatManagerPrivate::ApplyOverride(*Player, *Player, UAgAttributeSet::GetMPAttribute(), MP);
}

void UAgCheatManager::AgSetUP(float Percent)
{
	AAgPlayerCharacter* Player = AgCheatManagerPrivate::GetPlayer(*this);
	if (!Player)
	{
		AgCheatManagerPrivate::Report(*this, TEXT("AgSetUP: no player character"));
		return;
	}

	const float UP = FMath::RoundToFloat(Player->GetAttributeSet()->GetMaxUP() * Percent / 100.f);
	AgCheatManagerPrivate::ApplyOverride(*Player, *Player, UAgAttributeSet::GetUPAttribute(), UP);
}

void UAgCheatManager::AgInfiniteMP(const FString& OnOff)
{
	RunSwitch(TEXT("AgInfiniteMP"), OnOff, EAgCheatSwitch::InfiniteMP);
}

void UAgCheatManager::AgInfiniteUP(const FString& OnOff)
{
	RunSwitch(TEXT("AgInfiniteUP"), OnOff, EAgCheatSwitch::InfiniteUP);
}

void UAgCheatManager::AgNoCooldown(const FString& OnOff)
{
	RunSwitch(TEXT("AgNoCooldown"), OnOff, EAgCheatSwitch::NoCooldown);
}

void UAgCheatManager::AgTargetGroggy()
{
	AAgPlayerCharacter* Player = AgCheatManagerPrivate::GetPlayer(*this);
	AAgEnemyCharacter* Enemy = AgCheatManagerPrivate::FindTargetEnemy(Player);
	if (!Enemy || !Enemy->CanLosePP())
	{
		AgCheatManagerPrivate::Report(*this, TEXT("AgTargetGroggy: no target enemy that can lose PP"));
		return;
	}

	AgCheatManagerPrivate::ApplyOverride(*Player, *Enemy, UAgAttributeSet::GetPPAttribute(), 0.f);
}

void UAgCheatManager::AgTargetPattern(FName Id)
{
	AAgEnemyCharacter* Enemy = AgCheatManagerPrivate::FindTargetEnemy(AgCheatManagerPrivate::GetPlayer(*this));
	const FGameplayAbilitySpec* Spec = Enemy ? Enemy->FindAbilitySpec(Id) : nullptr;
	if (!Spec)
	{
		AgCheatManagerPrivate::Report(*this, FString::Printf(TEXT("AgTargetPattern: no target enemy with '%s'"), *Id.ToString()));
		return;
	}

	const FGameplayAbilitySpecHandle Handle = Spec->Handle;
	UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent();
	ASC->RemoveActiveEffectsWithTags(FGameplayTagContainer(AgGameplayTags::Effect_Cooldown));
	if (!ASC->TryActivateAbility(Handle))
	{
		AgCheatManagerPrivate::Report(*this, FString::Printf(TEXT("AgTargetPattern: %s could not start '%s'"), *Enemy->GetName(), *Id.ToString()));
	}
}

void UAgCheatManager::AgTargetHP(float Percent)
{
	AAgPlayerCharacter* Player = AgCheatManagerPrivate::GetPlayer(*this);
	AAgEnemyCharacter* Enemy = AgCheatManagerPrivate::FindTargetEnemy(Player);
	if (!Enemy)
	{
		AgCheatManagerPrivate::Report(*this, TEXT("AgTargetHP: no target enemy"));
		return;
	}

	const float HP = FMath::RoundToFloat(Enemy->GetAttributeSet()->GetMaxHP() * Percent / 100.f);
	AgCheatManagerPrivate::ApplyOverride(*Player, *Enemy, UAgAttributeSet::GetHPAttribute(), HP);
}

void UAgCheatManager::AgStopAI()
{
	AgCheatManagerPrivate::SetEnemyAIRunning(GetWorld(), false);
}

void UAgCheatManager::AgStartAI()
{
	AgCheatManagerPrivate::SetEnemyAIRunning(GetWorld(), true);
}

void UAgCheatManager::AgIgnoreDeviceInput(const FString& OnOff)
{
	const ULocalPlayer* LocalPlayer = GetOuterAPlayerController()->GetLocalPlayer();
	UGameViewportClient* ViewportClient = LocalPlayer ? LocalPlayer->ViewportClient.Get() : nullptr;
	if (!ViewportClient)
	{
		AgCheatManagerPrivate::Report(*this, TEXT("AgIgnoreDeviceInput: no game viewport"));
		return;
	}
	const TOptional<bool> bOn = AgCheatManagerPrivate::ParseSwitch(*this, TEXT("AgIgnoreDeviceInput"), OnOff, ViewportClient->IgnoreInput());
	if (!bOn.IsSet())
	{
		return;
	}

	// 엔진 'UI 전용' 입력 모드처럼 뷰포트에서 장치 입력을 버립니다. 콘솔은 계속 받고, 키 입력 명령은 뷰포트를 거치지
	// 않아 그대로 들어갑니다.
	ViewportClient->SetIgnoreInput(*bOn);
	AgCheatManagerPrivate::ReportSwitch(*this, TEXT("AgIgnoreDeviceInput"), *bOn);
}

void UAgCheatManager::AgPressKey(FName KeyName)
{
	AgCheatManagerPrivate::InjectNamedKey(*this, TEXT("AgPressKey"), KeyName, IE_Pressed, 1.f);
}

void UAgCheatManager::AgReleaseKey(FName KeyName)
{
	AgCheatManagerPrivate::InjectNamedKey(*this, TEXT("AgReleaseKey"), KeyName, IE_Released, 0.f);
}

void UAgCheatManager::AgMoveMouse(float X, float Y)
{
	AgCheatManagerPrivate::InjectKey(*this, EKeys::MouseX, IE_Axis, X);
	AgCheatManagerPrivate::InjectKey(*this, EKeys::MouseY, IE_Axis, Y);
}

void UAgCheatManager::RunSwitch(const TCHAR* Command, const FString& OnOff, EAgCheatSwitch Switch)
{
	AAgPlayerCharacter* Player = AgCheatManagerPrivate::GetPlayer(*this);
	if (!Player)
	{
		AgCheatManagerPrivate::Report(*this, FString::Printf(TEXT("%s: no player character"), Command));
		return;
	}
	const TOptional<bool> bOn = AgCheatManagerPrivate::ParseSwitch(*this, Command, OnOff, IsSwitchOn(*Player->GetAbilitySystemComponent(), Switch));
	if (!bOn.IsSet())
	{
		return;
	}

	SetSwitch(*Player, Switch, *bOn);
	AgCheatManagerPrivate::ReportSwitch(*this, Command, *bOn);
}

bool UAgCheatManager::IsSwitchOn(const UAbilitySystemComponent& ASC, EAgCheatSwitch Switch) const
{
	const FActiveGameplayEffectHandle* Handle = SwitchEffects.Find(Switch);
	return Handle && ASC.GetActiveGameplayEffect(*Handle) != nullptr;
}

void UAgCheatManager::SetSwitch(AAgPlayerCharacter& Player, EAgCheatSwitch Switch, bool bOn)
{
	UAbilitySystemComponent* ASC = Player.GetAbilitySystemComponent();
	if (bOn == IsSwitchOn(*ASC, Switch))
	{
		return;
	}
	if (bOn)
	{
		SwitchEffects.Add(Switch, ASC->ApplyGameplayEffectToSelf(AgCheatManagerPrivate::MakeSwitchEffect(Switch), 1.f, ASC->MakeEffectContext()));
		if (Switch == EAgCheatSwitch::NoCooldown)
		{
			ASC->RemoveActiveEffectsWithTags(FGameplayTagContainer(AgGameplayTags::Effect_Cooldown));
		}
		return;
	}

	ASC->RemoveActiveGameplayEffect(SwitchEffects[Switch]);
	// 덮어쓰기는 현재 값에만 걸려 켜 둔 동안의 비용이 기본값에서 빠져 있으므로, 끈 뒤에도 가득 차 있게 채웁니다.
	const AgCheatManagerPrivate::FAgLockedResource Resource = AgCheatManagerPrivate::GetLockedResource(Switch);
	if (Resource.Attribute.IsValid())
	{
		AgCheatManagerPrivate::ApplyOverride(Player, Player, Resource.Attribute, ASC->GetNumericAttribute(Resource.MaxAttribute));
	}
}
