// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/AgTargeting.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AIController.h"
#include "Camera/CameraComponent.h"
#include "Character/AgCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/AgAssetManager.h"
#include "Data/AgAbilityData.h"
#include "Engine/OverlapResult.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GenericTeamAgentInterface.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "SceneView.h"
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

bool AgTargeting::IsVisible(const AActor& Source, const AActor& Target, FName Socket)
{
	const APawn* Pawn = Cast<APawn>(&Source);
	AAIController* AIController = Pawn ? Cast<AAIController>(Pawn->GetController()) : nullptr;
	const UAIPerceptionComponent* Perception = AIController ? AIController->GetPerceptionComponent() : nullptr;
	if (Perception && !Perception->HasActiveStimulus(Target, UAISense::GetSenseID<UAISense_Sight>()))
	{
		return false;
	}
	FVector Start = Source.GetActorLocation();
	if (!Socket.IsNone())
	{
		const ACharacter* Character = Cast<ACharacter>(&Source);
		const USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
		if (!Mesh || !Mesh->DoesSocketExist(Socket))
		{
			return false;
		}
		Start = Mesh->GetSocketLocation(Socket);
	}
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AgVisible), false, &Source);
	QueryParams.AddIgnoredActor(&Target);
	return !Source.GetWorld()->LineTraceTestByObjectType(Start, Target.GetActorLocation(), FCollisionObjectQueryParams(ECC_WorldStatic),
		QueryParams);
}

bool AgTargeting::ProjectToGameScreen(const APawn& Pawn, const FVector& WorldLocation, FVector2D& OutScreenPosition, FVector2D* OutScreenSize)
{
	const APlayerController* Controller = Cast<APlayerController>(Pawn.GetController());
	const ULocalPlayer* LocalPlayer = Controller ? Controller->GetLocalPlayer() : nullptr;
	const UCameraComponent* Camera = Pawn.FindComponentByClass<UCameraComponent>();
	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;
	if (Controller)
	{
		Controller->GetViewportSize(ViewportWidth, ViewportHeight);
	}
	if (!LocalPlayer || !Camera || ViewportWidth <= 0 || ViewportHeight <= 0)
	{
		return false;
	}
	if (OutScreenSize)
	{
		*OutScreenSize = FVector2D(ViewportWidth, ViewportHeight);
	}

	// 엔진 투영(ULocalPlayer::GetProjectionData)은 카메라 매니저의 최종 시야를 쓰므로, 같은 계산을 카메라 컴포넌트의
	// 시야로 합니다. 컴포넌트의 GetCameraView는 회전을 고치는 부작용이 있어 값을 직접 읽습니다.
	FMinimalViewInfo View;
	View.Location = Camera->GetComponentLocation();
	View.Rotation = Camera->GetComponentRotation();
	View.FOV = Camera->FieldOfView;
	View.AspectRatio = Camera->AspectRatio;
	View.bConstrainAspectRatio = Camera->bConstrainAspectRatio;
	View.ProjectionMode = Camera->ProjectionMode;
	const FIntRect ViewRect(0, 0, ViewportWidth, ViewportHeight);
	FSceneViewProjectionData Projection;
	Projection.SetViewRectangle(ViewRect);
	Projection.ViewOrigin = View.Location;
	Projection.ViewRotationMatrix = FInverseRotationMatrix(View.Rotation) *
	                                FMatrix(FPlane(0, 0, 1, 0), FPlane(1, 0, 0, 0), FPlane(0, 1, 0, 0), FPlane(0, 0, 0, 1));
	FMinimalViewInfo::CalculateProjectionMatrixGivenViewRectangle(View, LocalPlayer->AspectRatioAxisConstraint, ViewRect, Projection);
	return FSceneView::ProjectWorldToScreen(WorldLocation, Projection.GetConstrainedViewRect(), Projection.ComputeViewProjectionMatrix(),
		OutScreenPosition);
}

AActor* AgTargeting::FindSideTarget(const FAgTargetingRequest& Request, APawn& Source, const AActor& Current, float Direction)
{
	FVector2D CurrentPosition;
	if (!ProjectToGameScreen(Source, Current.GetActorLocation(), CurrentPosition))
	{
		return nullptr;
	}
	AActor* Nearest = nullptr;
	double NearestOffset = TNumericLimits<double>::Max();
	for (AActor* Candidate : FindTargets(Request, Source))
	{
		FVector2D Position;
		if (Candidate == &Current || !ProjectToGameScreen(Source, Candidate->GetActorLocation(), Position))
		{
			continue;
		}
		const double Offset = (Position.X - CurrentPosition.X) * FMath::Sign(Direction);
		if (Offset > 0.0 && Offset < NearestOffset)
		{
			Nearest = Candidate;
			NearestOffset = Offset;
		}
	}
	return Nearest;
}

void UAgTargetingSelectionTask_Sphere::SelectTargets_Implementation(const FTargetingRequestHandle& TargetingHandle, const FTargetingSourceContext& SourceContext) const
{
	const AActor* Owner = SourceContext.SourceActor.Get();
	const FAgTargetingRequest* Request = UE::TargetingSystem::TTargetingDataStore<FAgTargetingRequest>::Find(TargetingHandle);
	if (Owner && Owner->GetWorld() && Request && Request->Range > 0.f)
	{
		// 엔진 AOE는 모양의 크기를 Task 속성으로만 받아 요청마다 반경을 바꿀 수 없으므로, 같은 겹침 검사를 요청의 Range로 합니다.
		TArray<FOverlapResult> Overlaps;
		const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AgTargetingSphere), false, Owner);
		Owner->GetWorld()->OverlapMultiByObjectType(Overlaps, Owner->GetActorLocation(), FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn),
			FCollisionShape::MakeSphere(Request->Range), QueryParams);
		for (const FOverlapResult& Overlap : Overlaps)
		{
			// 캐릭터 메시도 Pawn 오브젝트 종류라, 캡슐(몸과 회피를 시작한 자리)에 닿은 결과만 씁니다.
			AAgCharacterBase* Candidate = Cast<AAgCharacterBase>(Overlap.GetActor());
			if (Candidate && Cast<UCapsuleComponent>(Overlap.GetComponent()))
			{
				AddTargetActor(TargetingHandle, Candidate);
			}
		}
	}
}

void UAgTargetingSelectionTask_LockOnTarget::SelectTargets_Implementation(const FTargetingRequestHandle& TargetingHandle, const FTargetingSourceContext& SourceContext) const
{
	const AAgCharacterBase* Owner = Cast<AAgCharacterBase>(SourceContext.SourceActor);
	AddTargetActor(TargetingHandle, Owner ? Owner->GetLockOnTarget() : nullptr);
}

bool UAgTargetingFilterTask_Hostile::ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const
{
	const FTargetingSourceContext* Source = FTargetingSourceContext::Find(TargetingHandle);
	const AActor* Owner = Source ? Source->SourceActor.Get() : nullptr;
	if (!Owner)
	{
		return true;
	}
	return FGenericTeamId::GetAttitude(Owner, TargetData.HitResult.GetActor()) != ETeamAttitude::Hostile;
}

bool UAgTargetingFilterTask_Hittable::ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const
{
	const FTargetingSourceContext* Source = FTargetingSourceContext::Find(TargetingHandle);
	const AActor* Owner = Source ? Source->SourceActor.Get() : nullptr;
	const AAgCharacterBase* Candidate = Cast<AAgCharacterBase>(TargetData.HitResult.GetActor());
	return !Owner || !Candidate || !Candidate->CanBeHitBy(*Owner);
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

bool UAgTargetingFilterTask_LockOnTarget::ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const
{
	const FTargetingSourceContext* Source = FTargetingSourceContext::Find(TargetingHandle);
	const AAgCharacterBase* Owner = Source ? Cast<AAgCharacterBase>(Source->SourceActor) : nullptr;
	const AActor* LockOnTarget = Owner ? Owner->GetLockOnTarget() : nullptr;
	return LockOnTarget && TargetData.HitResult.GetActor() != LockOnTarget;
}

bool UAgTargetingFilterTask_OnScreen::ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const
{
	const FTargetingSourceContext* Source = FTargetingSourceContext::Find(TargetingHandle);
	const APawn* Pawn = Source ? Cast<APawn>(Source->SourceActor) : nullptr;
	const AActor* Candidate = TargetData.HitResult.GetActor();
	FVector2D ScreenPosition;
	FVector2D ScreenSize;
	const bool bOnScreen = Pawn && Candidate && AgTargeting::ProjectToGameScreen(*Pawn, Candidate->GetActorLocation(), ScreenPosition, &ScreenSize) &&
	                       ScreenPosition.X >= 0. && ScreenPosition.Y >= 0. && ScreenPosition.X <= ScreenSize.X && ScreenPosition.Y <= ScreenSize.Y;
	return !bOnScreen;
}

bool UAgTargetingFilterTask_Visible::ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const
{
	const FTargetingSourceContext* Source = FTargetingSourceContext::Find(TargetingHandle);
	const AActor* Candidate = TargetData.HitResult.GetActor();
	return !Source || !Source->SourceActor || !Candidate || !AgTargeting::IsVisible(*Source->SourceActor, *Candidate, Socket);
}

float UAgTargetingSortTask_ScreenCenter::GetScoreForTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const
{
	const FTargetingSourceContext* Source = FTargetingSourceContext::Find(TargetingHandle);
	const APawn* Pawn = Source ? Cast<APawn>(Source->SourceActor) : nullptr;
	const AActor* Candidate = TargetData.HitResult.GetActor();
	FVector2D ScreenPosition;
	FVector2D ScreenSize;
	if (!Pawn || !Candidate || !AgTargeting::ProjectToGameScreen(*Pawn, Candidate->GetActorLocation(), ScreenPosition, &ScreenSize))
	{
		return TNumericLimits<float>::Max();
	}
	return static_cast<float>(FVector2D::Distance(ScreenPosition, ScreenSize * 0.5));
}
