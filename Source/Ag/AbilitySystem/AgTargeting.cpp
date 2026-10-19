// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgTargeting.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Core/AgAssetManager.h"
#include "Data/AgAbilityData.h"
#include "GameFramework/PlayerController.h"
#include "GenericTeamAgentInterface.h"
#include "TargetingSystem/TargetingPreset.h"
#include "TargetingSystem/TargetingSubsystem.h"
#include "Types/TargetingSystemDataStores.h"
#include "Types/TargetingSystemTypes.h"

namespace UE::TargetingSystem
{
	extern TTargetingDataStore<FAgTargetingConditions> GAgTargetingConditionsDataStore;

	template <>
	FAgTargetingConditions& TTargetingDataStore<FAgTargetingConditions>::FindOrAdd(FTargetingRequestHandle Handle)
	{
		return GAgTargetingConditionsDataStore.Items.FindOrAdd(Handle);
	}

	template <>
	FAgTargetingConditions* TTargetingDataStore<FAgTargetingConditions>::Find(FTargetingRequestHandle Handle)
	{
		return GAgTargetingConditionsDataStore.Items.Find(Handle);
	}

	template <>
	void TTargetingDataStore<FAgTargetingConditions>::OnTargetingRequestHandleReleased(FTargetingRequestHandle Handle)
	{
		GAgTargetingConditionsDataStore.Items.Remove(Handle);
	}

	TTargetingDataStore<FAgTargetingConditions> GAgTargetingConditionsDataStore;
}

FAgTargetingConditions& FAgTargetingConditions::FindOrAdd(FTargetingRequestHandle Handle)
{
	return UE::TargetingSystem::GAgTargetingConditionsDataStore.FindOrAdd(Handle);
}

FAgTargetingConditions* FAgTargetingConditions::Find(FTargetingRequestHandle Handle)
{
	return UE::TargetingSystem::GAgTargetingConditionsDataStore.Find(Handle);
}

AActor* AgTargeting::FindTarget(const FAgAbilityTargeting& Targeting, AActor& Source)
{
	UTargetingSubsystem* Subsystem = UTargetingSubsystem::Get(Source.GetWorld());
	const UTargetingPreset* Preset = UAgAssetManager::Get().FindTargetingPreset(Targeting.Preset);
	if (!Preset || !Subsystem)
	{
		return nullptr;
	}
	FTargetingSourceContext Context;
	Context.SourceActor = &Source;
	Context.InstigatorActor = &Source;
	FTargetingRequestHandle Request = UTargetingSubsystem::MakeTargetRequestHandle(Preset, Context);
	FAgTargetingConditions& Conditions = FAgTargetingConditions::FindOrAdd(Request);
	Conditions.Range = Targeting.Range.Get(-1.f);
	Conditions.TagRequirements.RequireTags = Targeting.TargetRequiredTags;
	Conditions.TagRequirements.IgnoreTags = Targeting.TargetBlockedTags;

	// 프리셋의 Task는 즉시 실행되므로 결과를 바로 읽고 핸들을 놓습니다.
	Subsystem->ExecuteTargetingRequestWithHandle(Request);
	TArray<AActor*> Targets;
	Subsystem->GetTargetingResultsActors(Request, Targets);
	UTargetingSubsystem::ReleaseTargetRequestHandle(Request);
	return Targets.IsEmpty() ? nullptr : Targets[0];
}

UAgTargetingFilterTask_Hostile::UAgTargetingFilterTask_Hostile(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool UAgTargetingFilterTask_Hostile::ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const
{
	const FTargetingSourceContext* Source = FTargetingSourceContext::Find(TargetingHandle);
	const AActor* Owner = Source ? Source->SourceActor.Get() : nullptr;
	return !Owner || FGenericTeamId::GetAttitude(Owner, TargetData.HitResult.GetActor()) != ETeamAttitude::Hostile;
}

UAgTargetingFilterTask_TagRequirements::UAgTargetingFilterTask_TagRequirements(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool UAgTargetingFilterTask_TagRequirements::ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const
{
	const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetData.HitResult.GetActor());
	if (!ASC)
	{
		return true;
	}
	const FGameplayTagContainer& Tags = ASC->GetOwnedGameplayTags();
	const FAgTargetingConditions* Conditions = FAgTargetingConditions::Find(TargetingHandle);
	return !Requirements.RequirementsMet(Tags) || (Conditions && !Conditions->TagRequirements.RequirementsMet(Tags));
}

UAgTargetingFilterTask_Distance::UAgTargetingFilterTask_Distance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool UAgTargetingFilterTask_Distance::ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const
{
	const FTargetingSourceContext* Source = FTargetingSourceContext::Find(TargetingHandle);
	const AActor* Owner = Source ? Source->SourceActor.Get() : nullptr;
	const AActor* Candidate = TargetData.HitResult.GetActor();
	const FAgTargetingConditions* Conditions = FAgTargetingConditions::Find(TargetingHandle);
	return !Owner || !Candidate || (Conditions && Conditions->Range >= 0.f && Owner->GetHorizontalDistanceTo(Candidate) > Conditions->Range);
}

UAgTargetingFilterTask_OnScreen::UAgTargetingFilterTask_OnScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool UAgTargetingFilterTask_OnScreen::ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const
{
	const FTargetingSourceContext* Source = FTargetingSourceContext::Find(TargetingHandle);
	const APawn* Pawn = Source ? Cast<APawn>(Source->SourceActor) : nullptr;
	const APlayerController* Controller = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	const AActor* Candidate = TargetData.HitResult.GetActor();
	if (!Controller || !Candidate)
	{
		return true;
	}
	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;
	Controller->GetViewportSize(ViewportWidth, ViewportHeight);
	FVector2D ScreenPosition;
	const bool bOnScreen = Controller->ProjectWorldLocationToScreen(Candidate->GetActorLocation(), ScreenPosition) &&
	                       ScreenPosition.X >= 0.f && ScreenPosition.Y >= 0.f && ScreenPosition.X <= ViewportWidth &&
	                       ScreenPosition.Y <= ViewportHeight;
	return !bOnScreen;
}
