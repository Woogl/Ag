// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgTargeting.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Character/AgCharacterBase.h"
#include "Core/AgAssetManager.h"
#include "Data/AgAbilityData.h"
#include "EngineUtils.h"
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

TArray<AActor*> AgTargeting::FindTargets(const FAgTargetingRequest& Request, AActor& Source)
{
	TArray<AActor*> Targets;
	if (Request.Range <= 0.f)
	{
		return Targets;
	}
	UTargetingSubsystem* Subsystem = UTargetingSubsystem::Get(Source.GetWorld());
	const UTargetingPreset* Preset = UAgAssetManager::Get().FindTargetingPreset(Request.Preset);
	if (!Preset || !Subsystem)
	{
		return Targets;
	}
	FTargetingSourceContext Context;
	Context.SourceActor = &Source;
	Context.InstigatorActor = &Source;
	FTargetingRequestHandle Handle = UTargetingSubsystem::MakeTargetRequestHandle(Preset, Context);
	FAgTargetingConditions& Conditions = FAgTargetingConditions::FindOrAdd(Handle);
	Conditions.Range = Request.Range;
	Conditions.TagRequirements.RequireTags = Request.TargetRequiredTags;
	Conditions.TagRequirements.IgnoreTags = Request.TargetBlockedTags;

	// 프리셋의 Task는 즉시 실행되므로 결과를 바로 읽고 핸들을 놓습니다.
	Subsystem->ExecuteTargetingRequestWithHandle(Handle);
	Subsystem->GetTargetingResultsActors(Handle, Targets);
	UTargetingSubsystem::ReleaseTargetRequestHandle(Handle);
	return Targets;
}

AActor* AgTargeting::FindTarget(const FAgTargetingRequest& Request, AActor& Source)
{
	const TArray<AActor*> Targets = FindTargets(Request, Source);
	return Targets.IsEmpty() ? nullptr : Targets[0];
}

UAgTargetingSelectionTask_Range::UAgTargetingSelectionTask_Range(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAgTargetingSelectionTask_Range::Execute(const FTargetingRequestHandle& TargetingHandle) const
{
	Super::Execute(TargetingHandle);
	SetTaskAsyncState(TargetingHandle, ETargetingTaskAsyncState::Executing);

	const FTargetingSourceContext* Source = FTargetingSourceContext::Find(TargetingHandle);
	const AActor* Owner = Source ? Source->SourceActor.Get() : nullptr;
	const FAgTargetingConditions* Conditions = FAgTargetingConditions::Find(TargetingHandle);
	if (Owner && Owner->GetWorld() && Conditions && Conditions->Range > 0.f)
	{
		FTargetingDefaultResultsSet& Results = FTargetingDefaultResultsSet::FindOrAdd(TargetingHandle);
		// 엔진 AOE의 구체 반경은 높이 차이까지 제한하므로 캐릭터 중심의 수평 거리로 후보를 모읍니다.
		for (TActorIterator<AAgCharacterBase> It(Owner->GetWorld()); It; ++It)
		{
			AAgCharacterBase* Candidate = *It;
			if (Candidate == Owner || Owner->GetHorizontalDistanceTo(Candidate) > Conditions->Range)
			{
				continue;
			}
			FTargetingDefaultResultData& Result = Results.TargetResults.AddDefaulted_GetRef();
			Result.HitResult.HitObjectHandle = FActorInstanceHandle(Candidate);
			Result.HitResult.Location = Candidate->GetActorLocation();
			Result.HitResult.Distance = FVector::Distance(Owner->GetActorLocation(), Candidate->GetActorLocation());
		}
	}

	SetTaskAsyncState(TargetingHandle, ETargetingTaskAsyncState::Completed);
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
