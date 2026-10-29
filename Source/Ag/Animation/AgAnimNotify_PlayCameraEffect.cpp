// Copyright Woogle. All Rights Reserved.

#include "Animation/AgAnimNotify_PlayCameraEffect.h"

#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

FString UAgAnimNotify_PlayCameraEffect::GetNotifyName_Implementation() const
{
	TArray<FString> Names;
	for (const TSubclassOf<UCameraShakeBase>& Shake : CameraShakes)
	{
		if (Shake)
		{
			FString Name = Shake->GetName();
			Name.RemoveFromEnd(TEXT("_C"));
			Names.Add(MoveTemp(Name));
		}
	}
	if (FieldOfView.Offset != 0.f)
	{
		Names.Add(FString::Printf(TEXT("FOV %+g"), FieldOfView.Offset));
	}
	return Names.IsEmpty() ? Super::GetNotifyName_Implementation() : FString::Join(Names, TEXT(", "));
}

void UAgAnimNotify_PlayCameraEffect::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	const AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	UWorld* World = Owner ? Owner->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}
	const FVector Epicenter = Owner->GetActorLocation();
	for (const TSubclassOf<UCameraShakeBase>& Shake : CameraShakes)
	{
		UGameplayStatics::PlayWorldCameraShake(Owner, Shake, Epicenter, InnerRadius, OuterRadius);
	}
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PlayerController = It->Get();
		if (!PlayerController || !PlayerController->IsLocalController() || !PlayerController->PlayerCameraManager)
		{
			continue;
		}
		APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager;
		UCameraModifier* Modifier = CameraManager->FindCameraModifierByClass(UAgFieldOfViewCameraModifier::StaticClass());
		if (UAgFieldOfViewCameraModifier* FieldOfViewCamera = Cast<UAgFieldOfViewCameraModifier>(Modifier))
		{
			// 엔진의 CalcRadialShakeScale은 protected라, 셰이크와 같은 감쇠(PlayWorldCameraShake 기본 감쇠 지수 1)를 직접 구합니다.
			const float Distance = FVector::Dist(Epicenter, CameraManager->GetCameraLocation());
			const float Scale = FMath::GetMappedRangeValueClamped(FVector2f(InnerRadius, OuterRadius), FVector2f(1.f, 0.f), Distance);
			FieldOfViewCamera->Play(FieldOfView, Scale);
		}
	}
}
