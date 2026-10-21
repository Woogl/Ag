// Copyright Woogle. All Rights Reserved.

#include "Camera/AgPostProcessComponent.h"

#include "Ag.h"
#include "Components/PostProcessComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	const FName EffectWeightParameter(TEXT("AgEffectWeight"));
	const FName EffectTimeParameter(TEXT("AgEffectTime"));

	bool IsLocalPlayerPawn(const APawn* Pawn)
	{
		const APlayerController* Controller = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
		return Controller && Controller->IsLocalController() && Controller->GetPawn() == Pawn;
	}
}

UAgPostProcessComponent::UAgPostProcessComponent()
{
	bAutoActivate = false;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

bool UAgPostProcessComponent::Start(AActor* Target, float Duration)
{
	Reset();
	APawn* Pawn = Cast<APawn>(Target);
	if (!Material || !IsLocalPlayerPawn(Pawn) || !GetWorld())
	{
		return false;
	}
	FString Error;
	if (!ValidateSettings(Error) || !FMath::IsFinite(Duration) || Duration < 0.f || (Duration > 0.f && Duration < BlendIn + BlendOut))
	{
		UE_LOG(LogAg, Error, TEXT("%s: invalid screen effect (%s, duration %f)"), *GetPathName(), *Error, Duration);
		return false;
	}

	MaterialInstance = UMaterialInstanceDynamic::Create(Material, this);
	TargetPawn = Pawn;
	PlaybackDuration = Duration;
	PlaybackBlendIn = BlendIn;
	PlaybackBlendOut = BlendOut;
	StartTime = GetWorld()->GetAudioTimeSeconds();
	// UE 5.8의 PostProcessComponent 상속에 필요한 함수가 DLL에 공개되어 있지 않아 엔진 컴포넌트를 소유합니다.
	Volume = NewObject<UPostProcessComponent>(GetOwner(), NAME_None, RF_Transient);
	Volume->Settings.WeightedBlendables.Array.Emplace(1.f, MaterialInstance);
	Volume->bUnbound = true;
	Volume->BlendWeight = 1.f;
	UpdateMaterial(StartTime);
	Volume->RegisterComponent();
	SetComponentTickEnabled(true);
	return true;
}

bool UAgPostProcessComponent::Stop()
{
	if (!IsPlaying())
	{
		return false;
	}
	if (bFadingOut)
	{
		return true;
	}
	const double Now = GetWorld()->GetAudioTimeSeconds();
	StopWeight = GetWeight(Now);
	if (PlaybackBlendOut <= 0.f || StopWeight <= 0.f)
	{
		Reset();
		return false;
	}
	StopTime = Now;
	bFadingOut = true;
	return true;
}

void UAgPostProcessComponent::Reset()
{
	SetComponentTickEnabled(false);
	if (Volume)
	{
		Volume->bEnabled = false;
		Volume->DestroyComponent();
		Volume = nullptr;
	}
	MaterialInstance = nullptr;
	TargetPawn.Reset();
	StartTime = StopTime = 0.;
	PlaybackDuration = PlaybackBlendIn = PlaybackBlendOut = StopWeight = 0.f;
	bFadingOut = false;
}

bool UAgPostProcessComponent::IsPlaying() const
{
	return MaterialInstance != nullptr;
}

bool UAgPostProcessComponent::IsFadingOut() const
{
	return bFadingOut;
}

bool UAgPostProcessComponent::HasMaterial() const
{
	return Material != nullptr;
}

bool UAgPostProcessComponent::ValidateSettings(FString& Error) const
{
	if (!FMath::IsFinite(BlendIn) || !FMath::IsFinite(BlendOut) || BlendIn < 0.f || BlendOut < 0.f)
	{
		Error = TEXT("fade times must be finite and non-negative");
		return false;
	}
	if (Material)
	{
		const UMaterial* BaseMaterial = Material->GetMaterial();
		float DefaultWeight = 0.f;
		if (!BaseMaterial || BaseMaterial->MaterialDomain != MD_PostProcess || !Material->GetScalarParameterValue(FMaterialParameterInfo(EffectWeightParameter), DefaultWeight))
		{
			Error = TEXT("material must use the Post Process domain and define AgEffectWeight");
			return false;
		}
		if (BaseMaterial->bIsBlendable)
		{
			Error = TEXT("material Is Blendable must be disabled so cue instances remain independent");
			return false;
		}
	}
	return true;
}

void UAgPostProcessComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!IsPlaying())
	{
		return;
	}
	const double Now = GetWorld()->GetAudioTimeSeconds();
	if (!IsLocalPlayerPawn(TargetPawn.Get()) || (bFadingOut && Now - StopTime >= PlaybackBlendOut) || (PlaybackDuration > 0.f && Now - StartTime >= PlaybackDuration))
	{
		Finish();
		return;
	}
	UpdateMaterial(Now);
}

void UAgPostProcessComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Reset();
	Super::EndPlay(EndPlayReason);
}

float UAgPostProcessComponent::GetWeight(double Now) const
{
	if (bFadingOut)
	{
		return StopWeight * FMath::Clamp(1.f - static_cast<float>(Now - StopTime) / PlaybackBlendOut, 0.f, 1.f);
	}
	const float Elapsed = static_cast<float>(Now - StartTime);
	float Weight = PlaybackBlendIn > 0.f ? FMath::Clamp(Elapsed / PlaybackBlendIn, 0.f, 1.f) : 1.f;
	if (PlaybackDuration > 0.f && PlaybackBlendOut > 0.f)
	{
		Weight = FMath::Min(Weight, FMath::Clamp((PlaybackDuration - Elapsed) / PlaybackBlendOut, 0.f, 1.f));
	}
	return Weight;
}

void UAgPostProcessComponent::UpdateMaterial(double Now)
{
	const float Weight = GetWeight(Now);
	MaterialInstance->SetScalarParameterValue(EffectWeightParameter, Weight);
	MaterialInstance->SetScalarParameterValue(EffectTimeParameter, static_cast<float>(Now - StartTime));
	Volume->bEnabled = Weight > 0.f;
}

void UAgPostProcessComponent::Finish()
{
	Reset();
	OnFinished.ExecuteIfBound();
}
