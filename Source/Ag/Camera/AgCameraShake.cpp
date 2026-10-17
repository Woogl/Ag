// Copyright Woogle. All Rights Reserved.

#include "Camera/AgCameraShake.h"
#include "Data/AgCameraData.h"
#include "Shakes/WaveOscillatorCameraShakePattern.h"

UAgCameraShake::UAgCameraShake(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bSingleInstance = false;
	SetRootShakePattern(CreateDefaultSubobject<UWaveOscillatorCameraShakePattern>(TEXT("WavePattern")));
}

void UAgCameraShake::Initialize(const FAgCameraShakeSettings& Settings, const UAgCameraData& Camera)
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
	Pattern->Y.Frequency = Camera.ShakeYFrequency;
	Pattern->Z.Frequency = Camera.ShakeZFrequency;
	Pattern->Pitch.Frequency = Camera.ShakePitchFrequency;
	Pattern->Yaw.Frequency = Camera.ShakeYawFrequency;
}
