// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgTargeting.h"

#include "AbilitySystemComponent.h"
#include "AI/AgBossAIController.h"
#include "Character/AgCharacterBase.h"
#include "Data/AgAbilityData.h"
#include "GameFramework/PlayerController.h"
#include "TargetingSystem/TargetingPreset.h"
#include "TargetingSystem/TargetingSubsystem.h"
#include "Types/TargetingSystemDataStores.h"
#include "Types/TargetingSystemTypes.h"

namespace UE::TargetingSystem
{
	// 엔진 데이터(FTargetingSourceContext 등)와 같은 방식의 요청 핸들별 저장소. 핸들을 놓으면 저장소가 그 항목을 지웁니다.
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
	if (!Targeting.Preset || !Subsystem)
	{
		return nullptr;
	}
	FTargetingSourceContext Context;
	Context.SourceActor = &Source;
	Context.InstigatorActor = &Source;
	FTargetingRequestHandle Request = UTargetingSubsystem::MakeTargetRequestHandle(Targeting.Preset, Context);
	FAgTargetingConditions& Conditions = FAgTargetingConditions::FindOrAdd(Request);
	Conditions.Range = Targeting.bHasRange ? Targeting.Range : -1.f;
	Conditions.RequiredTags = Targeting.TargetRequiredTags;
	Conditions.BlockedTags = Targeting.TargetBlockedTags;

	// 프리셋의 Task는 즉시 실행되므로 결과를 바로 읽고 핸들을 놓습니다.
	Subsystem->ExecuteTargetingRequestWithHandle(Request);
	TArray<AActor*> Targets;
	Subsystem->GetTargetingResultsActors(Request, Targets);
	UTargetingSubsystem::ReleaseTargetRequestHandle(Request);
	return Targets.IsEmpty() ? nullptr : Targets[0];
}

UAgTargetingFilterTask_ActionTarget::UAgTargetingFilterTask_ActionTarget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool UAgTargetingFilterTask_ActionTarget::ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const
{
	const FTargetingSourceContext* Source = FTargetingSourceContext::Find(TargetingHandle);
	const AAgCharacterBase* Owner = Source ? Cast<AAgCharacterBase>(Source->SourceActor) : nullptr;
	const AAgCharacterBase* Candidate = Cast<AAgCharacterBase>(TargetData.HitResult.GetActor());
	if (!Owner || !Candidate || Candidate == Owner || Candidate->IsDead() ||
		Owner->GetTeamAttitudeTowards(*Candidate) != ETeamAttitude::Hostile)
	{
		return true;
	}
	const FAgTargetingConditions* Conditions = FAgTargetingConditions::Find(TargetingHandle);
	if (!Conditions)
	{
		return false;
	}
	const UAbilitySystemComponent* ASC = Candidate->GetAbilitySystemComponent();
	return (Conditions->Range >= 0.f && Owner->GetHorizontalDistanceTo(Candidate) > Conditions->Range) ||
	       !ASC->HasAllMatchingGameplayTags(Conditions->RequiredTags) || ASC->HasAnyMatchingGameplayTags(Conditions->BlockedTags);
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

UAgTargetingSortTask_HorizontalDistance::UAgTargetingSortTask_HorizontalDistance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

float UAgTargetingSortTask_HorizontalDistance::GetScoreForTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const
{
	// 기획서의 거리는 수평 거리라, 엔진 정렬(3D 거리) 대신 이 정렬을 씁니다. 점수가 작을수록 앞입니다.
	const FTargetingSourceContext* Source = FTargetingSourceContext::Find(TargetingHandle);
	const AActor* Owner = Source ? Source->SourceActor.Get() : nullptr;
	const AActor* Candidate = TargetData.HitResult.GetActor();
	return Owner && Candidate ? Owner->GetHorizontalDistanceTo(Candidate) : 0.f;
}

UAgTargetingSelectionTask_AITarget::UAgTargetingSelectionTask_AITarget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAgTargetingSelectionTask_AITarget::Execute(const FTargetingRequestHandle& TargetingHandle) const
{
	Super::Execute(TargetingHandle);
	SetTaskAsyncState(TargetingHandle, ETargetingTaskAsyncState::Executing);

	const FTargetingSourceContext* Source = FTargetingSourceContext::Find(TargetingHandle);
	const APawn* Pawn = Source ? Cast<APawn>(Source->SourceActor) : nullptr;
	const AAgBossAIController* Controller = Pawn ? Cast<AAgBossAIController>(Pawn->GetController()) : nullptr;
	if (AActor* Target = Controller ? Controller->GetTarget() : nullptr)
	{
		FTargetingDefaultResultData& Result = FTargetingDefaultResultsSet::FindOrAdd(TargetingHandle).TargetResults.AddDefaulted_GetRef();
		Result.HitResult.HitObjectHandle = FActorInstanceHandle(Target);
		Result.HitResult.Location = Target->GetActorLocation();
		Result.HitResult.ImpactPoint = Result.HitResult.Location;
	}

	SetTaskAsyncState(TargetingHandle, ETargetingTaskAsyncState::Completed);
}
