// Copyright Woogle. All Rights Reserved.

#include "Camera/AgCameraShake.h"
#include "Data/AgCameraData.h"
#include "Shakes/WaveOscillatorCameraShakePattern.h"

namespace
{
	constexpr float ShakePitchFrequency = 25.f;
	constexpr float ShakeYawFrequency = 21.f;
	constexpr float ShakeYFrequency = 27.f;
	constexpr float ShakeZFrequency = 23.f;
}

UAgCameraShake::UAgCameraShake(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bSingleInstance = true;
	SetRootShakePattern(CreateDefaultSubobject<UWaveOscillatorCameraShakePattern>(TEXT("WavePattern")));
}

void UAgCameraShake::Initialize(const FAgCameraShakeSettings& Settings)
{
	UWaveOscillatorCameraShakePattern* Pattern = CastChecked<UWaveOscillatorCameraShakePattern>(GetRootShakePattern());
	Pattern->Duration = Settings.Duration;
	Pattern->BlendInTime = 0.f;
	Pattern->BlendOutTime = Settings.Duration;
	Pattern->X.Amplitude = 0.f;
	Pattern->Roll.Amplitude = 0.f;
	Pattern->FOV.Amplitude = 0.f;
	Pattern->Y.Amplitude = Pattern->Z.Amplitude = Settings.LocationAmplitude;
	Pattern->Pitch.Amplitude = Pattern->Yaw.Amplitude = Settings.RotationAmplitude;
	Pattern->Y.Frequency = ShakeYFrequency;
	Pattern->Z.Frequency = ShakeZFrequency;
	Pattern->Pitch.Frequency = ShakePitchFrequency;
	Pattern->Yaw.Frequency = ShakeYawFrequency;
}
