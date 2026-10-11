// Copyright Woogle. All Rights Reserved.

#include "Combat/AgGameplayCue_UltimateSlashes.h"

#include "Components/StaticMeshComponent.h"

AAgGameplayCue_UltimateSlashes::AAgGameplayCue_UltimateSlashes()
{
	bAutoDestroyOnRemove = true;

	SlashTemplate = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SlashTemplate"));
	SlashTemplate->SetupAttachment(RootComponent);
	SlashTemplate->SetVisibility(false);
	SlashTemplate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SlashTemplate->SetGenerateOverlapEvents(false);
}

bool AAgGameplayCue_UltimateSlashes::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	// 견본을 복제해 넘어온 위치에, 긴 쪽(Y축)이 넘어온 방향을 향하게 놓습니다. 큐 액터를 따라 움직이지 않습니다.
	UStaticMeshComponent* Slash = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient, SlashTemplate);
	Slash->SetupAttachment(nullptr);
	Slash->SetRelativeTransform(FTransform(FRotationMatrix::MakeFromY(Parameters.Normal).Rotator(), Parameters.Location, SlashTemplate->GetRelativeScale3D()));
	Slash->SetVisibility(true);
	Slash->RegisterComponent();
	Slashes.Add(Slash);
	return true;
}

bool AAgGameplayCue_UltimateSlashes::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	for (UStaticMeshComponent* Slash : Slashes)
	{
		if (Slash)
		{
			Slash->DestroyComponent();
		}
	}
	Slashes.Reset();
	return true;
}
