// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgTargeting.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Character/AgCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Core/AgAssetManager.h"
#include "Data/AgAbilityData.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GenericTeamAgentInterface.h"
#include "TargetingSystem/TargetingPreset.h"
#include "TargetingSystem/TargetingSubsystem.h"
#include "Types/TargetingSystemDataStores.h"
#include "Types/TargetingSystemTypes.h"

DECLARE_TARGETING_DATA_STORE(FAgTargetingRequest)
DEFINE_TARGETING_DATA_STORE(FAgTargetingRequest)

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
	UE::TargetingSystem::TTargetingDataStore<FAgTargetingRequest>::FindOrAdd(Handle) = Request;

	// 프리셋의 Task는 즉시 실행되므로 결과를 바로 읽고 핸들을 놓습니다.
	Subsystem->ExecuteTargetingRequestWithHandle(Handle);
	Subsystem->GetTargetingResultsActors(Handle, Targets);
	UTargetingSubsystem::ReleaseTargetRequestHandle(Handle);
	return Targets;
}

AActor* AgTargeting::FindTarget(const FAgTargetingRequest& Request, AActor& Source)
{
	const TArray<AActor*> Targets = FindTargets(Request, Source);
	const AAgCharacterBase* Character = Cast<AAgCharacterBase>(&Source);
	AActor* LockOnTarget = Character ? Character->GetLockOnTarget() : nullptr;
	if (LockOnTarget && Targets.Contains(LockOnTarget))
	{
		return LockOnTarget;
	}
	return Targets.IsEmpty() ? nullptr : Targets[0];
}

bool AgTargeting::IsInRange(const AActor& Source, const AActor& Target, float Range)
{
	// 찾을 때와 같은 구 겹침으로 보되, 극한 회피 자리 캡슐은 빼고 몸 캡슐(루트)만 봅니다. 거리로 비교하면 범위 끝에서
	// 찾자마자 풀리기를 되풀이합니다.
	const UPrimitiveComponent* Collision = Cast<UPrimitiveComponent>(Target.GetRootComponent());
	return Range > 0.f && Collision && Collision->OverlapComponent(Source.GetActorLocation(), FQuat::Identity, FCollisionShape::MakeSphere(Range));
}

UAgTargetingSelectionTask_Sphere::UAgTargetingSelectionTask_Sphere(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAgTargetingSelectionTask_Sphere::Execute(const FTargetingRequestHandle& TargetingHandle) const
{
	Super::Execute(TargetingHandle);
	SetTaskAsyncState(TargetingHandle, ETargetingTaskAsyncState::Executing);

	const FTargetingSourceContext* Source = FTargetingSourceContext::Find(TargetingHandle);
	const AActor* Owner = Source ? Source->SourceActor.Get() : nullptr;
	const FAgTargetingRequest* Request = UE::TargetingSystem::TTargetingDataStore<FAgTargetingRequest>::Find(TargetingHandle);
	if (Owner && Owner->GetWorld() && Request && Request->Range > 0.f)
	{
		// 엔진 AOE는 모양의 크기를 Task 속성으로만 받아 요청마다 반경을 바꿀 수 없으므로, 같은 겹침 검사를 요청의 Range로 합니다.
		TArray<FOverlapResult> Overlaps;
		const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AgTargetingSphere), false, Owner);
		Owner->GetWorld()->OverlapMultiByObjectType(Overlaps, Owner->GetActorLocation(), FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn),
			FCollisionShape::MakeSphere(Request->Range), QueryParams);
		FTargetingDefaultResultsSet& Results = FTargetingDefaultResultsSet::FindOrAdd(TargetingHandle);
		for (const FOverlapResult& Overlap : Overlaps)
		{
			// 캐릭터 메시도 Pawn 오브젝트 종류라, 캡슐(몸과 회피를 시작한 자리)에 닿은 결과만 씁니다.
			AAgCharacterBase* Candidate = Cast<AAgCharacterBase>(Overlap.GetActor());
			if (!Candidate || !Cast<UCapsuleComponent>(Overlap.GetComponent()) ||
				Results.TargetResults.ContainsByPredicate([Candidate](const FTargetingDefaultResultData& Result)
			{ return Result.HitResult.GetActor() == Candidate; }))
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
	if (!Owner)
	{
		return true;
	}
	const ETeamAttitude::Type Attitude = FGenericTeamId::GetAttitude(Owner, TargetData.HitResult.GetActor());
	return bIncludeNeutral ? Attitude == ETeamAttitude::Friendly : Attitude != ETeamAttitude::Hostile;
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
	const FAgTargetingRequest* Request = UE::TargetingSystem::TTargetingDataStore<FAgTargetingRequest>::Find(TargetingHandle);
	return !Requirements.RequirementsMet(Tags) || (Request && (!Tags.HasAll(Request->TargetRequiredTags) || Tags.HasAny(Request->TargetBlockedTags)));
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

UAgTargetingSortTask_ScreenCenter::UAgTargetingSortTask_ScreenCenter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

float UAgTargetingSortTask_ScreenCenter::GetScoreForTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const
{
	const FTargetingSourceContext* Source = FTargetingSourceContext::Find(TargetingHandle);
	const APawn* Pawn = Source ? Cast<APawn>(Source->SourceActor) : nullptr;
	const APlayerController* Controller = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	const AActor* Candidate = TargetData.HitResult.GetActor();
	if (!Controller || !Candidate)
	{
		return TNumericLimits<float>::Max();
	}
	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;
	Controller->GetViewportSize(ViewportWidth, ViewportHeight);
	if (ViewportWidth <= 0 || ViewportHeight <= 0)
	{
		return TNumericLimits<float>::Max();
	}
	FVector2D ScreenPosition;
	if (!Controller->ProjectWorldLocationToScreen(Candidate->GetActorLocation(), ScreenPosition))
	{
		return TNumericLimits<float>::Max();
	}
	const FVector2D ScreenCenter(ViewportWidth * 0.5f, ViewportHeight * 0.5f);
	return FVector2D::Distance(ScreenPosition, ScreenCenter);
}
