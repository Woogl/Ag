// Copyright Woogle. All Rights Reserved.

#include "Animation/AgAnimNotify_PlayCameraEffect.h"

#include "Camera/AgFieldOfViewCameraModifier.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
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
		APlayerCameraManager::PlayWorldCameraShake(World, Shake, Epicenter, InnerRadius, OuterRadius, 1.f);
	}
	// 애니메이션 편집기의 미리 보기 월드에는 플레이어 카메라가 없습니다.
	APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(World, 0);
	UCameraModifier* Modifier = CameraManager ? CameraManager->FindCameraModifierByClass(UAgFieldOfViewCameraModifier::StaticClass()) : nullptr;
	if (UAgFieldOfViewCameraModifier* FieldOfViewCamera = Cast<UAgFieldOfViewCameraModifier>(Modifier))
	{
		// 엔진의 CalcRadialShakeScale은 protected라, 셰이크와 같은 감쇠(위에서 넘긴 감쇠 지수 1)를 직접 구합니다.
		const float Distance = FVector::Dist(Epicenter, CameraManager->GetCameraLocation());
		const float Scale = FMath::GetMappedRangeValueClamped(FVector2f(InnerRadius, OuterRadius), FVector2f(1.f, 0.f), Distance);
		FieldOfViewCamera->Play(FieldOfView, Scale);
	}
}
