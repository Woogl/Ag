// Copyright Woogle. All Rights Reserved.

#include "Combat/AgGameplayCue_UltimateSlashes.h"
#include "Character/AgCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/AgCharacterData.h"

AAgGameplayCue_UltimateSlashes::AAgGameplayCue_UltimateSlashes()
{
	bAutoDestroyOnRemove = true;

	SlashTemplate = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SlashTemplate"));
	SlashTemplate->SetupAttachment(RootComponent);
	SlashTemplate->SetVisibility(false);
	SlashTemplate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SlashTemplate->SetGenerateOverlapEvents(false);
}

bool AAgGameplayCue_UltimateSlashes::OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters)
{
	const AAgCharacterBase* Character = Cast<AAgCharacterBase>(Target);
	const UAgPlayerData* Data = Character ? Cast<UAgPlayerData>(Character->GetCharacterData()) : nullptr;
	if (!Data)
	{
		return false;
	}
	// 카메라 쪽을 향하는 면 안에 베기 선을 배치합니다. 각도는 표현 알고리즘의 범위입니다.
	const float ViewYaw = Character->GetControlRotation().Yaw;
	const float Yaw = ViewYaw + FMath::FRandRange(-35.f, 35.f);
	const float Distance = FMath::FRandRange(Data->UltimateSlashDistance.Min, Data->UltimateSlashDistance.Max);
	const float Height = FMath::FRandRange(Data->UltimateSlashHeight.Min, Data->UltimateSlashHeight.Max);
	const FVector Feet = Character->GetActorLocation() - FVector(0.f, 0.f, Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	const float Angle = FMath::DegreesToRadians(FMath::FRandRange(0.f, 180.f));
	const FVector Location = Feet + FRotator(0.f, Yaw, 0.f).Vector() * Distance + FVector(0.f, 0.f, Height);
	const FVector Direction = FRotator(0.f, ViewYaw + 90.f, 0.f).Vector() * FMath::Cos(Angle) + FVector::UpVector * FMath::Sin(Angle);

	// 견본을 복제해 긴 쪽(Y축)이 방향을 향하게 놓습니다. 큐 액터를 따라 움직이지 않습니다.
	UStaticMeshComponent* Slash = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient, SlashTemplate);
	Slash->CreationMethod = EComponentCreationMethod::Instance;
	Slash->SetupAttachment(nullptr);
	Slash->SetWorldTransform(FTransform(FRotationMatrix::MakeFromY(Direction).Rotator(), Location, SlashTemplate->GetRelativeScale3D()));
	Slash->SetVisibility(true);
	Slash->RegisterComponent();
	Slashes.Add(Slash);
	return true;
}

void AAgGameplayCue_UltimateSlashes::ClearSlashes()
{
	for (UStaticMeshComponent* Slash : Slashes)
	{
		if (Slash)
		{
			Slash->DestroyComponent();
		}
	}
	Slashes.Reset();
}

bool AAgGameplayCue_UltimateSlashes::Recycle()
{
	ClearSlashes();
	return Super::Recycle();
}

bool AAgGameplayCue_UltimateSlashes::OnRemove_Implementation(AActor* Target, const FGameplayCueParameters& Parameters)
{
	ClearSlashes();
	return true;
}

void AAgGameplayCue_UltimateSlashes::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearSlashes();
	Super::EndPlay(EndPlayReason);
}
