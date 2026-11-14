// Copyright Woogle. All Rights Reserved.

#include "Animation/AgAnimNotifyState_CameraLookAt.h"

#include "Camera/AgPlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"

void UAgAnimNotifyState_CameraLookAt::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	// 애니메이션 편집기의 미리 보기 월드에는 플레이어 카메라가 없습니다.
	const AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	AAgPlayerCameraManager* CameraManager = Owner ? Cast<AAgPlayerCameraManager>(UGameplayStatics::GetPlayerCameraManager(Owner, 0)) : nullptr;
	if (CameraManager && FVector::Dist(Owner->GetActorLocation(), CameraManager->GetCameraLocation()) <= Radius)
	{
		CameraManager->StartLookAt(*Owner, LookAt);
	}
}

void UAgAnimNotifyState_CameraLookAt::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	const AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	AAgPlayerCameraManager* CameraManager = Owner ? Cast<AAgPlayerCameraManager>(UGameplayStatics::GetPlayerCameraManager(Owner, 0)) : nullptr;
	if (CameraManager)
	{
		CameraManager->StopLookAt(*Owner);
	}
}
