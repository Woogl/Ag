// Copyright Woogle. All Rights Reserved.

#include "Camera/AgFieldOfViewCameraModifier.h"

UAgFieldOfViewCameraModifier::UAgFieldOfViewCameraModifier(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 처형 카메라(64)는 시야각을 구도 값으로 덮어쓰므로 그 뒤에, 셰이크 모디파이어(127)보다는 먼저 더합니다.
	Priority = 96;
}

void UAgFieldOfViewCameraModifier::Play(const FAgFieldOfViewEffect& Effect, float Scale)
{
	Start(Effect, Scale, false);
}

void UAgFieldOfViewCameraModifier::Hold(const FAgFieldOfViewEffect& Effect)
{
	Start(Effect, 1.f, true);
}

void UAgFieldOfViewCameraModifier::Release(const FAgFieldOfViewEffect& Effect)
{
	for (FActiveEffect& Active : ActiveEffects)
	{
		if (Active.Source == &Effect && Active.bHold)
		{
			Active.bHold = false;
			Active.Phase = EPhase::BlendOut;
		}
	}
}

void UAgFieldOfViewCameraModifier::StopAll()
{
	ActiveEffects.Reset();
}

void UAgFieldOfViewCameraModifier::ModifyCamera(float DeltaTime, FVector ViewLocation, FRotator ViewRotation, float FOV, FVector& NewViewLocation, FRotator& NewViewRotation, float& NewFOV)
{
	float Offset = 0.f;
	for (int32 Index = ActiveEffects.Num() - 1; Index >= 0; --Index)
	{
		FActiveEffect& Active = ActiveEffects[Index];
		switch (Active.Phase)
		{
			case EPhase::BlendIn:
				Active.Weight = Active.Effect.BlendInTime > 0.f ? FMath::Min(Active.Weight + DeltaTime / Active.Effect.BlendInTime, 1.f) : 1.f;
				if (Active.Weight >= 1.f)
				{
					Active.Phase = EPhase::Hold;
				}
				break;
			case EPhase::Hold:
				if (!Active.bHold)
				{
					Active.HoldRemaining -= DeltaTime;
					if (Active.HoldRemaining <= 0.f)
					{
						Active.Phase = EPhase::BlendOut;
					}
				}
				break;
			case EPhase::BlendOut:
				Active.Weight = Active.Effect.BlendOutTime > 0.f ? Active.Weight - DeltaTime / Active.Effect.BlendOutTime : 0.f;
				break;
		}
		if (Active.Phase == EPhase::BlendOut && Active.Weight <= 0.f)
		{
			ActiveEffects.RemoveAtSwap(Index);
			continue;
		}
		// 시간에 비례하는 세기를 그대로 쓰면 변화가 시작하고 멈추는 순간이 튀므로 이즈 인·아웃을 겁니다.
		Offset += Active.Effect.Offset * Active.Scale * FMath::InterpEaseInOut(0.f, 1.f, Active.Weight, 2.f);
	}
	NewFOV = FOV + Offset;
}

void UAgFieldOfViewCameraModifier::Start(const FAgFieldOfViewEffect& Effect, float Scale, bool bHold)
{
	if (Effect.Offset == 0.f || Scale <= 0.f)
	{
		return;
	}
	// 재생 중인 같은 연출은 지금 세기에서 들어가기부터 다시 시작합니다.
	FActiveEffect* Active = ActiveEffects.FindByPredicate([&Effect](const FActiveEffect& Each) { return Each.Source == &Effect; });
	if (!Active)
	{
		Active = &ActiveEffects.AddDefaulted_GetRef();
		Active->Source = &Effect;
	}
	Active->Effect = Effect;
	Active->Scale = Scale;
	Active->Phase = EPhase::BlendIn;
	Active->HoldRemaining = Effect.HoldTime;
	Active->bHold = bHold;
}
