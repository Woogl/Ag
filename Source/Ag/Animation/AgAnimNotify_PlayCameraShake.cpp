// Copyright Woogle. All Rights Reserved.

#include "Animation/AgAnimNotify_PlayCameraShake.h"

#include "Camera/CameraShakeBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"

FString UAgAnimNotify_PlayCameraShake::GetNotifyName_Implementation() const
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
	return Names.IsEmpty() ? Super::GetNotifyName_Implementation() : FString::Join(Names, TEXT(", "));
}

void UAgAnimNotify_PlayCameraShake::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	const AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner)
	{
		return;
	}
	for (const TSubclassOf<UCameraShakeBase>& Shake : CameraShakes)
	{
		UGameplayStatics::PlayWorldCameraShake(Owner, Shake, Owner->GetActorLocation(), InnerRadius, OuterRadius);
	}
}
